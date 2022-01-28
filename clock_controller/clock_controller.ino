/*
clock_controller.cpp 

Version: 1.0
(c) 2020 Wojtek Kosak

This program is free software: you can redistribute it and/or modify
it under the terms of the version 3 GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _delay_ms
#define _delay_ms delay
#endif

#include <Wire.h>
#include <RtcDS3231.h>

RtcDS3231<TwoWire> rtcObject(Wire);

#include "font.h"
#include "config.h"

void do_trick_1();
void do_trick_2();
void do_trick_3();
void do_trick_4();
void do_trick_5();
void show_temp(int16_t spd); 


uint16_t set_positions[8][3][2];
byte set_directions[8][3][2];
int16_t get_positions[8][3][2];
uint16_t set_speeds[8][3];



uint32_t previousMillis = 0;  
uint16_t interval = 1000; 

#ifndef __AVR__
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server;

const long utcOffsetInSeconds = 3600;


void synchronizeClock() {
   WiFiUDP ntpUDP;
   NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
   timeClient.begin();
   delay(500);
   timeClient.update();
   RtcDateTime currentTime = rtcObject.GetDateTime();
   RtcDateTime newTime = RtcDateTime(currentTime.Year(), currentTime.Month(), currentTime.Day(), 
     timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds()); 
   rtcObject.SetDateTime(newTime);   
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname("clock");
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  server.on("/",[](){server.send(200,"text/plain","Hello on the clock\n");});
  server.on("/trick1", [](){
     server.send(200, "text/plain", "start_trick_1\n");
     do_trick_2();
  }); 
  server.on("/trick2", [](){
     server.send(200, "text/plain", "start_trick_2\n");
     do_trick_2();
  });
  server.on("/synchro", [](){
     server.send(200, "text/plain", "synchro\n");
     synchronizeClock();
  });   
  server.on("/temp", [](){
     server.send(200, "text/plain", "temp\n");
     show_temp(900);
     interval=25000;
  });   
  
  server.begin(); 

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  
}
#else
void initWiFi() {
}

#endif

void send_positions() {
  uint16_t sendstop[2] = {0, 0};
  byte dirs[2] = {0, 0};
  int16_t xpos, ypos;
  int16_t i;
  for (xpos = 0; xpos < 8; xpos++) {
    for (ypos = 0; ypos < 3; ypos++) {
      sendstop[0] = set_positions[xpos][ypos][0];
      sendstop[1] = set_positions[xpos][ypos][1];
      dirs[0] = set_directions[xpos][ypos][0];
      dirs[1] = set_directions[xpos][ypos][1];
      int16_t offset;
      offset = offsets[xpos / 2][ypos][xpos % 2];
      sendstop[0]=(sendstop[0]+offset)%10800;
      sendstop[1]=(sendstop[1]+offset)%10800;
        
      Wire.beginTransmission(clock_address[xpos / 2][ypos][xpos % 2]);
      for (i = 0; i < sizeof(sendstop); i++) {
        byte *p = (byte *)sendstop + i;
        Wire.write(*p);
      }
      for (i = 0; i < sizeof(dirs); i++) {
        byte *p = (byte *)dirs + i;
        Wire.write(*p);
      }    
      unsigned int16_t q = set_speeds[xpos][ypos];
      for (i = 0; i < sizeof(q); i++) {
        byte *p = (byte *)&q + i;
        Wire.write(*p);
      }
      Wire.endTransmission();
    }
  }
}
void wait_for_positions(int16_t timeout) {
  _delay_ms(timeout*10); //WKG
  return;
  
  uint16_t positions[2] = {0, 0};
  uint16_t position_sent[2] = {0, 0};
  int16_t towait=timeout*100;
  byte *d;
  int16_t xpos, ypos;
  while (1) {
    for (xpos = 0; xpos < 8; xpos++) {
      for (ypos = 0; ypos < 3; ypos++) {
        Wire.requestFrom(clock_address[xpos / 2][ypos][xpos % 2], 4);
        d = (byte *)&positions;
        while (Wire.available()) {
          *(d++) = Wire.read();
        }
        Serial.print("position (");
        Serial.print(clock_address[xpos / 2][ypos][xpos % 2]);
        Serial.print("): ");
        Serial.print(positions[0]);
        Serial.print(",");
        Serial.println(positions[1]);
        get_positions[xpos][ypos][0] = positions[0];
        get_positions[xpos][ypos][1] = positions[1];       
      }
    }

    int16_t m = 0;
    int16_t offset = 0;
 
    for (xpos = 0; xpos < 8; xpos++) {
      for (ypos = 0; ypos < 3; ypos++) {
        offset = offsets[xpos / 2][ypos][xpos % 2];
        position_sent[0] = set_positions[xpos][ypos][0];
        position_sent[1] = set_positions[xpos][ypos][1];
        offset = offsets[xpos / 2][ypos][xpos % 2];
        position_sent[0]=(position_sent[0]+offset)%10800;
        position_sent[1]=(position_sent[1]+offset)%10800;
        if (get_positions[xpos][ypos][0] != position_sent[0] || get_positions[xpos][ypos][1] != set_positions[xpos][ypos][1]) {
          m++;
          break;
        }
      }
    }
    if (!m) break;
    _delay_ms(5);
    //check this timeout
    //if (!towait) break;
    towait--;
  }
}


void show_temp(int16_t spd) {
  byte digital[4] = {0, 0, 0, 0};
  byte digit;
  byte ypos;
  byte xpos;

  RtcTemperature tempObj = rtcObject.GetTemperature();
  byte temp = (byte)tempObj.AsFloatDegC();
  digital[0] = temp / 10;
  digital[1] = temp % 10;
  digital[2] = 'o';
  digital[3] = 'C';

  for (digit = 0; digit < 4; digit++) {
    for (ypos = 0; ypos < 3; ypos++) {
      for (xpos = 0; xpos < 2; xpos++) {
        int16_t i;
        int16_t j;
        getdigit(digital[digit], 2 - ypos, xpos, &set_positions[digit*2+xpos][ypos][0], &set_positions[digit*2+xpos][ypos][1]);
        set_speeds[digit*2+xpos][ypos]=spd;

        set_directions[xpos][ypos][0] = random(0,2)==0?1:2;
        set_directions[xpos][ypos][1] = random(0,2)==0?1:2;

      }
    }
  }
  send_positions();
}


void do_trick_1() {
  int16_t x, y, i ;
  for (x = 0; x < 8; x++) {
    set_speeds[x][0] = set_speeds[x][1] = set_speeds[x][2] = 900;
    set_positions[x][0][0] = stoppositions[3]; set_positions[x][0][1] = stoppositions[1];
    set_positions[x][2][0] = stoppositions[3]; set_positions[x][2][1] = stoppositions[1];
    set_positions[x][1][0] = stoppositions[3]; set_positions[x][1][1] = stoppositions[1];
  }
  send_positions(); wait_for_positions(50);
  int16_t a = 10;
  for (i = 0; i < 10; i++) {
    for (x = 0; x < 8; x++) {
      if (x % 2) {
        set_positions[x][0][0] = stoppositions[3] + i * a; set_positions[x][0][1] = stoppositions[1] + i * a; set_directions[x][0][0] = 1; set_directions[x][0][1] = 1;
        set_positions[x][1][0] = stoppositions[3] - i * a; set_positions[x][1][1] = stoppositions[1] - i * a; set_directions[x][1][0] = 2; set_directions[x][1][1] = 2;
        set_positions[x][2][0] = stoppositions[3] + i * a; set_positions[x][2][1] = stoppositions[1] + i * a; set_directions[x][2][0] = 1; set_directions[x][2][1] = 1;
      } else {
        set_positions[x][0][0] = stoppositions[3] - i * a; set_positions[x][0][1] = stoppositions[1] - i * a; set_directions[x][0][0] = 2; set_directions[x][0][1] = 2;
        set_positions[x][1][0] = stoppositions[3] + i * a; set_positions[x][1][1] = stoppositions[1] + i * a; set_directions[x][1][0] = 1; set_directions[x][1][1] = 1;
        set_positions[x][2][0] = stoppositions[3] - i * a; set_positions[x][2][1] = stoppositions[1] - i * a; set_directions[x][2][0] = 2; set_directions[x][2][1] = 2;
      }
    }
    send_positions(); wait_for_positions(50);
    for (x = 0; x < 8; x++) {
      if (x % 2) {
        set_positions[x][0][0] = stoppositions[3] - i * a; set_positions[x][0][1] = stoppositions[1] - i * a; set_directions[x][0][0] = 2; set_directions[x][0][1] = 2;
        set_positions[x][1][0] = stoppositions[3] + i * a; set_positions[x][1][1] = stoppositions[1] + i * a; set_directions[x][1][0] = 1; set_directions[x][1][1] = 1;
        set_positions[x][2][0] = stoppositions[3] - i * a; set_positions[x][2][1] = stoppositions[1] - i * a; set_directions[x][2][0] = 2; set_directions[x][2][1] = 2;

      } else {
        set_positions[x][0][0] = stoppositions[3] + i * a; set_positions[x][0][1] = stoppositions[1] + i * a; set_directions[x][0][0] = 1; set_directions[x][0][1] = 1;
        set_positions[x][1][0] = stoppositions[3] - i * a; set_positions[x][1][1] = stoppositions[1] - i * a; set_directions[x][1][0] = 2; set_directions[x][1][1] = 2;
        set_positions[x][2][0] = stoppositions[3] + i * a; set_positions[x][2][1] = stoppositions[1] + i * a; set_directions[x][2][0] = 1; set_directions[x][2][1] = 1;
      }
    }
    send_positions(); wait_for_positions(50);
  }
  for (i = 0; i < 10; i++) {
    for (x = 0; x < 8; x++) {
      set_positions[x][0][0] = random(0, 10799);
      set_positions[x][0][1] = random(0, 10799);
      set_positions[x][1][0] = random(0, 10799);
      set_positions[x][1][1] = random(0, 10799);
      set_positions[x][2][0] = random(0, 10799);
      set_positions[x][2][1] = random(0, 10799);
    }
  }
  send_positions(); wait_for_positions(50);
  _delay_ms(2000);
}
void do_trick_2() {
  int16_t x, y, i;
  for (x = 0; x < 8; x++) {
    set_speeds[x][0] = set_speeds[x][1] = set_speeds[x][2] = 900;
  }
  for (x = 0; x < 4; x++) {
    set_positions[x][0][0] = stoppositions[1]; set_positions[x][0][1] = stoppositions[1];
    set_positions[x][2][0] = stoppositions[1]; set_positions[x][2][1] = stoppositions[1];
    set_positions[x][1][0] = stoppositions[1]; set_positions[x][1][1] = stoppositions[1];
  }
  for (x = 4; x < 8; x++) {
    set_positions[x][0][0] = stoppositions[3]; set_positions[x][0][1] = stoppositions[3];
    set_positions[x][2][0] = stoppositions[3]; set_positions[x][2][1] = stoppositions[3];
    set_positions[x][1][0] = stoppositions[3]; set_positions[x][1][1] = stoppositions[3];
  }
  send_positions(); wait_for_positions(50);
  for (x = 0; x < 4; x++) {
    set_directions[x][0][0] = 2; set_directions[x][0][1] = 2;
    set_directions[x][1][0] = 1; set_directions[x][1][1] = 2;
    set_directions[x][2][0] = 1; set_directions[x][2][1] = 1;
    set_directions[x + 4][0][0] = 1; set_directions[x + 4][0][1] = 1;
    set_directions[x + 4][1][0] = 2; set_directions[x + 4][1][1] = 1;
    set_directions[x + 4][2][0] = 2; set_directions[x + 4][2][1] = 2;
  }
  for (y = 0; y < 3; y++) {
    set_speeds[0][y] = set_speeds[7][y] = 3000;
    set_speeds[1][y] = set_speeds[6][y] = 2900;
    set_speeds[2][y] = set_speeds[5][y] = 2700;
    set_speeds[3][y] = set_speeds[4][y] = 2300;
  }
  send_positions();
  for (x = 0; x < 8; x++) {
    set_positions[x][0][0] = stoppositions[0]; set_positions[x][0][1] = stoppositions[0];
    set_positions[x][1][0] = stoppositions[2]; set_positions[x][1][1] = stoppositions[0];
    set_positions[x][2][0] = stoppositions[2]; set_positions[x][2][1] = stoppositions[2];
  }

  send_positions(); wait_for_positions(50);

  for (x = 0; x < 8; x++) {
    set_speeds[x][0] = set_speeds[x][1] = set_speeds[x][2] = 900;
  }
  send_positions();
  _delay_ms(2000);

}
void do_trick_3() {
  int16_t x, y, i ;
  for (x = 0; x < 8; x++) {
    set_speeds[x][0] = set_speeds[x][1] = set_speeds[x][2] = 900;
    set_positions[x][0][0] = stoppositions[3]; set_positions[x][0][1] = stoppositions[1];
    set_positions[x][2][0] = stoppositions[3]; set_positions[x][2][1] = stoppositions[1];
    set_positions[x][1][0] = stoppositions[3]; set_positions[x][1][1] = stoppositions[1];
  }
  send_positions(); wait_for_positions(50);
  for (x = 0; x < 8; x++) {
    set_speeds[x][0] = set_speeds[x][1] = set_speeds[x][2] = 150;
  }
  send_positions();
  for (x = 0; x < 8; x++) {
    set_positions[x][0][0] = stoppositions[1];
    set_directions[x][0][0] = 1;
    set_positions[x][2][0] = stoppositions[1];
    set_directions[x][2][0] = 1;
    set_positions[7 - x][1][1] = stoppositions[3];
    set_directions[7 - x][1][1] = 2;
    send_positions(); wait_for_positions(50);
  }
  for (x = 0; x < 8; x++) {
    set_speeds[x][0] = set_speeds[x][1] = set_speeds[x][2] = 900;
  }
  send_positions();
}
void do_trick_4() {
  int16_t x, y, i ;
  for (y = 0; y < 3; y++) {
    set_speeds[0][y] = set_speeds[1][y] = set_speeds[2][y] = set_speeds[3][y] = 900;
    set_speeds[4][y] = set_speeds[5][y] = set_speeds[6][y] = set_speeds[7][y] = 900;
  }
  i = 0;
  for (x = 3; x >= 0; x--) {
    set_positions[x][1][0] = stoppositions[1] - i * 200; set_positions[x][1][1] = stoppositions[1] + i * 200;
    set_positions[7 - x][1][0] = stoppositions[3] - i * 200; set_positions[7 - x][1][1] = stoppositions[3] + i * 200;
    set_positions[x][0][0] = stoppositions[1] - i * 200; set_positions[x][0][1] = stoppositions[1] - i * 200;
    set_positions[7 - x][0][0] = stoppositions[3] + i * 200; set_positions[7 - x][0][1] = stoppositions[3] + i * 200;
    set_positions[x][2][0] = stoppositions[1] + i * 200; set_positions[x][2][1] = stoppositions[1] + i * 200;
    set_positions[7 - x][2][0] = stoppositions[3] - i * 200; set_positions[7 - x][2][1] = stoppositions[3] - i * 200;
    i += 2;
  }
  send_positions(); wait_for_positions(50);
  _delay_ms(2000);
  for (x = 0; x < 4; x++) {
    for (y = 0; y < 3; y++) {
      set_positions[x][y][0] = 50000; set_positions[x][y][1] = 50000;
      set_positions[7 - x][y][0] = 50000; set_positions[7 - x][y][1] = 50000;
      set_directions[x][y][0] = 2; set_directions[x][y][1] = 1;
      set_directions[7 - x][y][0] = 2; set_directions[7 - x][y][1] = 1;
    }
    send_positions();
    _delay_ms(600);
  }

  _delay_ms(7000);

}
void do_trick_5() {
  int16_t x, y, i ;
  const int16_t sp[8]={0, 1349, 2699,4049, 5399, 6749, 8099, 9449};
  int16_t spx[8][2];
  
  for (y = 0; y < 3; y++) {
    set_speeds[0][y] = set_speeds[1][y] = set_speeds[2][y] = set_speeds[3][y] = 900;
    set_speeds[4][y] = set_speeds[5][y] = set_speeds[6][y] = set_speeds[7][y] = 900;
  }
  for (x = 0; x< 8;x++) {
    spx[x][0]=1;spx[x][1]=5;   
    set_positions[x][0][0]=sp[spx[x][0]];set_positions[x][0][1]=sp[spx[x][1]];
    set_positions[x][1][0]=sp[spx[x][0]];set_positions[x][1][1]=sp[spx[x][1]];
    set_positions[x][2][0]=sp[spx[x][0]];set_positions[x][2][1]=sp[spx[x][1]];
    set_directions[x][0][0]=1;set_directions[x][0][1]=1; 
    set_directions[x][1][0]=1;set_directions[x][1][1]=1;
    set_directions[x][2][0]=1;set_directions[x][2][1]=1;  
  }
  send_positions(); wait_for_positions(50);
  for (y = 0; y < 3; y++) {
    set_speeds[0][y] = set_speeds[1][y] = set_speeds[2][y] = set_speeds[3][y] = 500;
    set_speeds[4][y] = set_speeds[5][y] = set_speeds[6][y] = set_speeds[7][y] = 500;
  }
  i=0;
  while (1) {
    for (x=0;x<8;x++) {
      if (x<=i && i-x<3) {
        if (i%2) spx[x][0]=(spx[x][0]+1)%8; else spx[x][1]=(spx[x][1]+1)%8;
        set_positions[x][0][0]=sp[spx[x][0]];set_positions[x][0][1]=sp[spx[x][1]];
        set_positions[x][1][0]=sp[spx[x][0]];set_positions[x][1][1]=sp[spx[x][1]];
        set_positions[x][2][0]=sp[spx[x][0]];set_positions[x][2][1]=sp[spx[x][1]];
      }
    }
    send_positions(); wait_for_positions(50);
    i++;
    if (i==20) break;
  }

  i=0;
  while (1) {
    for (x=0;x<8;x++) {
      if (x<=i && i-x<3) {
        if (i%2) spx[x][0]=(spx[x][0]+1)%8; else spx[x][1]=(spx[x][1]+1)%8;
        set_positions[x][0][0]=sp[spx[x][0]];set_positions[x][0][1]=sp[spx[x][1]];
        set_positions[x][1][0]=sp[spx[x][0]];set_positions[x][1][1]=sp[spx[x][1]];
        set_positions[x][2][0]=sp[spx[x][0]];set_positions[x][2][1]=sp[spx[x][1]];
      }
    }
    send_positions(); wait_for_positions(50);
    i++;
    if (i==20) break;
  }
  
}

void do_trick_6() {
  int16_t x, y, i, j,digit;
  
  for (i = 0; i < 2; i++) {
    for (x = 0; x < 8; x++) {
      for (y = 0; y < 3; y++) {
        set_positions[x][y][i] = random(0, 10799);
        set_directions[x][y][i] = random(0,2)==0?1:2;
      }
    }
  }
  send_positions(); wait_for_positions(50);

  byte digital[4] = {0, 0, 0, 0};

  /*
  dt = clockds.getDateTime();
  */
  
  //digital[2] = dt.minute / 10;
  //digital[3] = dt.minute % 10;
  //digital[0] = dt.hour / 10;
  //digital[1] = dt.hour % 10;

  for (digit = 0; digit < 4; digit++) {
    for (y = 0; y < 3; y++) {
      for (x = 0; x < 2; x++) {
        int16_t i;
        int16_t j;
        getdigit(digital[digit], 2 - y, x, &set_positions[digit*2+x][y][0], &set_positions[digit*2+x][y][1]);
        set_speeds[digit*2+x][y]=800;  
      }
    }
  }
  send_positions(); 
  /*
  for (j=0;j<10;j++) {
    for (i = 0; i < 2; i++) {
      for (x = 0; x < 8; x++) {
        for (y = 0; y < 3; y++) {
          if (random(0,2)==1) { set_directions[x][y][i]=2; } else { set_directions[x][y][i]=1;};
        }
      }
    }
    send_positions();
    _delay_ms(100);
  }
  */
  for (j=0;j<200;j++) {
    x=random(0,9);
    y=random(0,4);
    i=random(0,2);
    if (set_directions[x][y][i]==1) set_directions[x][y][i]=2; else set_directions[x][y][i]=1;
    send_positions();
    _delay_ms(5);
  }
  
  
}

uint32_t previousMillisHandsRandom = 0;

void show_time_speed(int16_t spd) {
  byte digital[4] = {0, 0, 0, 0};
  byte digit;
  byte ypos;
  byte xpos;
  byte doRandom=0;
  Serial.println("show time start");

  RtcDateTime currentTime = rtcObject.GetDateTime(); 
  

  digital[2] = currentTime.Minute() / 10;
  digital[3] = currentTime.Minute() % 10;
  digital[0] = currentTime.Hour() / 10;
  digital[1] = currentTime.Hour() % 10;
  Serial.print(digital[0]);
  Serial.print(digital[1]);
  Serial.print(digital[2]);
  Serial.println(digital[3]);
  
  uint32_t currentMillis = millis();
  if (currentMillis - previousMillisHandsRandom >= 20000) {
      previousMillisHandsRandom = currentMillis;  
      doRandom=1;   
  } 

  for (digit = 0; digit < 4; digit++) {
    for (ypos = 0; ypos < 3; ypos++) {
      for (xpos = 0; xpos < 2; xpos++) {
        int16_t i;
        int16_t j;
        getdigit(digital[digit], 2 - ypos, xpos, &set_positions[digit*2+xpos][ypos][0], &set_positions[digit*2+xpos][ypos][1]);
        set_speeds[digit*2+xpos][ypos]=spd;

        if (doRandom) {
          set_directions[xpos][ypos][0] = random(0,2)==0?1:2;
          set_directions[xpos][ypos][1] = random(0,2)==0?1:2;
        }
        
      }
    }
  }
  send_positions();
 
}




void setup() {
  Wire.begin();
  Wire.setClock(10000L);
  //rtcObject.Begin(); 
  Serial.begin(9600);
  initWiFi();
  
}
int16_t temp_delay=20;
int16_t trickcount=100;

void loop() {
#ifndef __AVR__
  ArduinoOTA.handle();
  server.handleClient();
#endif

  // put your main code here, to run repeatedly:
  uint32_t currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
     previousMillis = currentMillis;
     show_time_speed(900);
     interval=1000;
  
  
 
     temp_delay--;
     if (temp_delay==0) {
       temp_delay=160;
       show_temp(900);
       interval=20000;
     }  
     
  
    if (--trickcount == 0) {
      trickcount = 600 + random(0, 250);
      switch (random(0, 6)) {
        case 0:
          do_trick_1();
          break;
        case 1:
          do_trick_2();
          break;
        case 2:
          do_trick_3();
          break;
        case 3:
          do_trick_4();
          break;
        case 4:
          do_trick_5();
          break;
        case 5:
          do_trick_6();
          break;
      }
    }
  }
}
