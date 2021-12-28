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

#include "DS3231.h"
#include <Wire.h>
#include "font.h"


static DS3231 clockds;
unsigned int set_positions[8][3][2];
byte set_directions[8][3][2];
int get_positions[8][3][2];
unsigned int set_speeds[8][3];
static RTCDateTime dt;

const int stoppositions[5]={0, 2699,5399, 8099, 9449/*1349*/};

byte clock_address[4][3][2] = {
  { {21, 22},
    {11, 12},
    {1, 2}
  },

  { {23, 24},
    {13, 14},
    {3, 4}
  },

  { {25, 26},
    {15, 16},
    {5, 6}
  },

  { {27, 28},
    {17, 18},
    {7, 8}
  }
};

int offsets[4][3][2] = {
  { {0, 0},
    {0, 0},
    {0, 5399}
  },

  { {0, 0},
    {0, 0},
    {0, 0}
  },

  { {0, 0},
    {0, 0},
    {0, 0}
  },

  { {0, 0},
    {0, 0},
    {0, 0}
  }
};


#ifndef __AVR__
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char *ssid     = "xxxx";
const char *password = "xxxxxx";

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
}
#else
void initWiFi() {
}
}
#endif

void send_positions() {
  unsigned int sendstop[2] = {0, 0};
  byte dirs[2] = {0, 0};
  int xpos, ypos;
  int i;
  for (xpos = 0; xpos < 8; xpos++) {
    for (ypos = 0; ypos < 3; ypos++) {
      sendstop[0] = set_positions[xpos][ypos][0];
      sendstop[1] = set_positions[xpos][ypos][1];
      dirs[0] = set_directions[xpos][ypos][0];
      dirs[1] = set_directions[xpos][ypos][1];
      int offset;
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
      unsigned int q = set_speeds[xpos][ypos];
      for (i = 0; i < sizeof(q); i++) {
        byte *p = (byte *)&q + i;
        Wire.write(*p);
      }
      Wire.endTransmission();
    }
  }
}
void wait_for_positions(int timeout) {
  unsigned int positions[2] = {0, 0};
  unsigned int position_sent[2] = {0, 0};
  int towait=timeout*100;
  byte *d;
  int xpos, ypos;
  while (1) {
    for (xpos = 0; xpos < 8; xpos++) {
      for (ypos = 0; ypos < 3; ypos++) {
        Wire.requestFrom(clock_address[xpos / 2][ypos][xpos % 2], 4);
        d = (byte *)&positions;
        while (Wire.available()) {
          *(d++) = Wire.read();
        }
        get_positions[xpos][ypos][0] = positions[0];
        get_positions[xpos][ypos][1] = positions[1];       
      }
    }

    int m = 0;
    int offset = 0;
 
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

void show_temp(int spd) {
  byte temp = clockds.readTemperature();
  byte digital[4] = {0, 0, 0, 0};
  byte digit;
  byte ypos;
  byte xpos;
  
  digital[0] = temp / 10;
  digital[1] = temp % 10;
  digital[2] = 'o';
  digital[3] = 'C';

  for (digit = 0; digit < 4; digit++) {
    for (ypos = 0; ypos < 3; ypos++) {
      for (xpos = 0; xpos < 2; xpos++) {
        int i;
        int j;
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
  int x, y, i ;
  for (x = 0; x < 8; x++) {
    set_speeds[x][0] = set_speeds[x][1] = set_speeds[x][2] = 900;
    set_positions[x][0][0] = stoppositions[3]; set_positions[x][0][1] = stoppositions[1];
    set_positions[x][2][0] = stoppositions[3]; set_positions[x][2][1] = stoppositions[1];
    set_positions[x][1][0] = stoppositions[3]; set_positions[x][1][1] = stoppositions[1];
  }
  send_positions(); wait_for_positions(50);
  int a = 10;
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
  int x, y, i;
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
  int x, y, i ;
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
  int x, y, i ;
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
  int x, y, i ;
  const int sp[8]={0, 1349, 2699,4049, 5399, 6749, 8099, 9449};
  int spx[8][2];
  
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
  int x, y, i, j,digit;
  
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

  dt = clockds.getDateTime();
  digital[2] = dt.minute / 10;
  digital[3] = dt.minute % 10;
  digital[0] = dt.hour / 10;
  digital[1] = dt.hour % 10;

  for (digit = 0; digit < 4; digit++) {
    for (y = 0; y < 3; y++) {
      for (x = 0; x < 2; x++) {
        int i;
        int j;
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


void show_time_speed(int spd) {
  byte digital[4] = {0, 0, 0, 0};
  byte digit;
  byte ypos;
  byte xpos;

  dt = clockds.getDateTime();
  digital[2] = dt.minute / 10;
  digital[3] = dt.minute % 10;
  digital[0] = dt.hour / 10;
  digital[1] = dt.hour % 10;

  for (digit = 0; digit < 4; digit++) {
    for (ypos = 0; ypos < 3; ypos++) {
      for (xpos = 0; xpos < 2; xpos++) {
        int i;
        int j;
        getdigit(digital[digit], 2 - ypos, xpos, &set_positions[digit*2+xpos][ypos][0], &set_positions[digit*2+xpos][ypos][1]);
        set_speeds[digit*2+xpos][ypos]=spd;
        
        set_directions[xpos][ypos][0] = random(0,2)==0?1:2;
        set_directions[xpos][ypos][1] = random(0,2)==0?1:2;
        
      }
    }
  }
  send_positions();
 
}




void setup() {
  Wire.begin();
  Serial.begin(9600);
  initWiFi();
  
}
int temp_delay=20;
int trickcount=100;

void loop() {
  // put your main code here, to run repeatedly:
  show_time_speed(900);

  temp_delay--;
  if (temp_delay==0) {
    temp_delay=20;
    show_temp(900);
    _delay_ms(15000);
  }  
  _delay_ms(1000);

  if (--trickcount == 0) {
    trickcount = 200 + random(0, 50);
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
