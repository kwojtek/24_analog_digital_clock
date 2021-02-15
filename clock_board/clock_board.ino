/*
clock_board.cpp 

software for clock boards. 
https://hackaday.io/project/163582-digital-clock-made-of-analog-clocks

!!!!! Use with atmega328pb (pb - it is very important) with internal clock speed 8Mhz !!!!


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

#include <Wire.h>

#include <string.h>
#include <EEPROM.h>


int handdirection[2]={1,-1};
static int direction[2][2]={{1,-1},{-1,-1}};
int values[2][2]={{0,63},{0,63}};
volatile unsigned int current_step[2]={0,0};
const int stoppositions[5]={0, 2699,5399, 8099, 9449};
byte motorPinOut[2][4]={{9,10,5,6},{11,3,0,2}};
byte motorSensor[2]={1,2};
byte started=0;
int need_zero[2]={0,0};

int config_address=0;
int  config_zero_gauss[2]={0,0};
int rotation=0;
void read_config();

void receivedEvent(int howMany);
void requestEvent();
void zero_hands();

unsigned int stoponposition[2] = {50000, 50000};

void setup() {
  byte x, y;
  ADCSRA = (1 << ADEN) | (1 << ADPS1) | (1 << ADPS2);
  Serial.begin(9600);
  _delay_ms(3000);
  if (Serial.available() >= 2 ) {
    char bytes[2];
    bytes[0] = Serial.read();
    bytes[1] = Serial.read();
    while (Serial.available() > 0) {
      Serial.read();
    }
    if (bytes[0] == 'H' && bytes[1] == 'I') {
      Serial.println("Welcome in the administration panel");
      adminPanel();
    }
  }

  read_config();
  Serial.end();
  randomSeed(analogRead(0));
  setup_timers();

  PCICR = (1<<PCIE1);
  PCMSK1 = (1<<PCINT8) | (1<<PCINT11);

  sei();
  pinMode(A0,INPUT);
  pinMode(A3,INPUT);

  zero_hands();
  current_step[0] = rotation;
  current_step[1] = rotation;  
  started = 1;   
  Wire.begin(config_address); //if I'm slave
  Wire.onReceive(receivedEvent);
  Wire.onRequest(requestEvent);
  need_zero[0]=need_zero[1]=1;
}



void setup_timers() {
  DDRD |= (1 << PD5) | (1 << PD6);
  TCCR0A = TCCR0B = 0;
  TCCR0A |= (1 << WGM00) | (1 << COM0A1) | (1 << COM0B1) ;
  TCCR0B |= (1 << CS00);

  DDRB |= (1 << PB1) | (1 << PB2);
  TCCR1A = TCCR1B = 0;
  TCCR1A |= (1 << WGM10) | (1 << COM1A1) | (1 << COM1B1) ;
  TCCR1B |= (1 << CS10);

  DDRD |= (1 << PD0) | (1 << PD2);
  TCCR3A = TCCR3B = 0;
  TCCR3A |= (1 << WGM30) | (1 << COM3A1) | (1 << COM3B1) ;
  TCCR3B |= (1 << CS30);

  DDRD |= (1 << PD3);
  DDRB |= (1 << PB3);
  PORTD |= (1 << PD2);
  TCCR2A = TCCR2B = 0;
  TCCR2A |= (1 << WGM20) | (1 << COM2A1) | (1 << COM2B1) ;
  TCCR2B |= (1 << CS20);
  
  TCCR4A = TCCR4B = TCNT4 = 0;
  OCR4A = 1100;
  TCCR4B |= (1 << WGM42) | (1 << CS41);
  TIMSK4 |= (1 << OCIE4A);
}

void loop() {
  delay(10);
  return;
}

char processCharInput(char* cmdBuffer, const char c)
{
  if (c >= 32 && c <= 126) {
    if (strlen(cmdBuffer) < 32) {
      strncat(cmdBuffer, &c, 1);  
    } else {
      return '\n';
    }
  } else if ((c == 8 || c == 127) && cmdBuffer[0] != 0) {
    cmdBuffer[strlen(cmdBuffer) - 1] = 0;
  }
  return c;
}
int readvalAdmin(byte num) {
  ADMUX  = (1 << REFS0) | (num == 1 ? (1 << MUX0) : (1 << MUX1));
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  int a = ADC;
  return a;
}

void adminPanel() {
  static char cmdBuffer[32] = "";
  char c;
  byte *p;
  while (1) {
    while (Serial.available())  {
      c = processCharInput(cmdBuffer, Serial.read());
      if (c == '\n') {

        //Full command received. Do your stuff here!
        if (strcmp("ZERO", cmdBuffer) == 0)  {
          int i;
          int data1[10];
          int data2[10];
          int px[2] = {0, 0};
          for (i = 0; i < 10; i++) {
            data1[i] = readvalAdmin(1);
            data2[i] = readvalAdmin(2);
          }
          for (i = 0; i < 10; i++) {
            px[0] += data1[i];
            px[1] += data2[i];
          }
          px[0] = px[0] / 10;
          px[1] = px[1] / 10;
          p = (byte *)px;
          int offset = 0;
          for (i = 0; i < sizeof(px); i++) {
            EEPROM.write(offset, *p);
            offset++;
            *p++;
          }
          Serial.print("wrote ");
          Serial.print(px[0]);
          Serial.print(" ");
          Serial.print(px[1]);
          Serial.println("");
          Serial.println("\r\nZero set!");
          cmdBuffer[0] = 0;
        } else if (strncmp("ADR", cmdBuffer, 3) == 0) {
          int data;
          int i;
          data = atoi((char *)(cmdBuffer + 3));

          p = (byte *)&data;
          int offset = 4;
          for (i = 0; i < sizeof(data); i++) {
            EEPROM.write(offset, *p);
            offset++;
            *p++;
          }
          Serial.print("setting address ");
          Serial.println(data);
          cmdBuffer[0] = 0;
        } if (strncmp("ROT", cmdBuffer, 3) == 0) {
          int data;
          int i;
          data = atoi((char *)(cmdBuffer + 3));

          p = (byte *)&data;
          int offset = 6;
          for (i = 0; i < sizeof(data); i++) {
            EEPROM.write(offset, *p);
            offset++;
            *p++;
          }
          Serial.print("setting rotation ");
          Serial.println(data);
          cmdBuffer[0] = 0;
        } else {
          Serial.print("unkonown command: ");
          Serial.println(cmdBuffer);
          cmdBuffer[0] = 0;
        }
      }

    }
    _delay_ms(1);
  }
}


ISR(TIMER4_COMPA_vect) {
  if (!started) return;
  cli();
  if (current_step[0] != stoponposition[0] || stoponposition[0] == 50000) {
    run_motor(0);
  }
  if (current_step[1] != stoponposition[1] || stoponposition[1] == 50000) {
    run_motor(1);
  }
  sei();
}



void receivedEvent(int howMany) {
  byte data[10];
  memset(data, 0, 10);
  int p = 0;
  while (Wire.available() > 0) {
    byte x;
    x = Wire.read();
    data[p++] = x;
  }

  if (data[4] == 1)
    set_direction(0, 1);
  else if (data[4] == 2)
    set_direction(0, -1);

  if (data[5] == 1)
    set_direction(1, 1);
  else if (data[5] == 2)
    set_direction(1, -1);

  memcpy((void*)stoponposition, (void*)data, sizeof(stoponposition));
  unsigned int q;
  memcpy((void*)&q, (void*)(data + 6), sizeof(q));

  OCR4A = q;
}

void requestEvent() {
  Wire.write((byte *)current_step, 4);
}


static const int allsin[60]={255,254,249,243,233,221,206,190,171,150,128,104,79,53,27,0,
                                      -27,-53,-79,-104,-128,-150,-171,-190,-206,-221,-233,-243,-249,
                                       -254,-255,-254,-249,-243,-233,-221,-206,-190,-171,-150,-128,-104,
                                       -79,-53,-27,0,27,53,79,104,128,150,171,190,206,221,233,243,249,254};

static unsigned char motors_sin_pos[2][2]={{0,15},{0,15}};

                                   

void set_direction(char n, int dir) {
  if (handdirection[n]==dir) return;
  handdirection[n]=handdirection[n]*(-1);
  direction[n][0]=direction[n][0]*(-1);
  direction[n][1]=direction[n][1]*(-1);
}

int get_direction(char n) {
  return handdirection[n];
}

int readvalRunMotor(byte num) {
  ADMUX  = (1 << REFS0) | (num == 1 ? (1 << MUX0) : (1 << MUX1));
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  int a = ADC;
  return a;
}

void run_motor(char n) {
  static unsigned char motor[4];
  char q;


  if (current_step[n]==rotation && need_zero[n]>0) {
    int zeroval;
    if (n==0) {
      zeroval=readvalRunMotor(motorSensor[1]);
      zeroval=abs(zeroval-config_zero_gauss[1]);
    } else {
      zeroval=readvalRunMotor(motorSensor[0]);
      zeroval=abs(zeroval-config_zero_gauss[0]);
    }
    zeroval=zeroval>>2;
    if (zeroval<4) {
       need_zero[n]=10;
    }
  }
  if (handdirection[n]==1) {
      if (current_step[n]== 10799) {
        current_step[n]=0;
      } else {
        current_step[n]++;
      }
  } else if (handdirection[n]==-1) {
       if (current_step[n]==0) {
        current_step[n]= 10799;
       } else {
        current_step[n]--;
       }
  }
  
  for (q=0;q<2;q++) {
    if (handdirection[n]>0) {
      if (motors_sin_pos[n][q]==59) motors_sin_pos[n][q]=0;
      else motors_sin_pos[n][q]++;
    } else {
      if (motors_sin_pos[n][q]==0) motors_sin_pos[n][q]=59;
      else motors_sin_pos[n][q]--;
    }
    if (allsin[motors_sin_pos[n][q]]>=0) {
      int x;
      x= allsin [ motors_sin_pos[n][q] ];
      motor[!q?0:2]=0;
      motor[!q?1:3]=x;
    } else {
      int x;
      x= allsin [ motors_sin_pos[n][q] ] * (-1);
      motor[!q?0:2]=x;
      motor[!q?1:3]=0;
    }
  }

  if (n==0) {
    OCR0A=motor[0];
    OCR0B=motor[1];
    OCR1A=motor[2];
    OCR1B=motor[3];
  } else {
    OCR2A=motor[0];
    OCR2B=motor[1];
    OCR3A=motor[2];
    OCR3B=motor[3];   
  }
 
}



int readvalConfig(byte num) {
  ADMUX  = (1<<REFS0) | (num==1?(1<<MUX0):(1<<MUX1));
  ADCSRA |= (1<<ADSC);
  while(ADCSRA & (1<<ADSC));
  int a=ADC;
  return a;
}


void read_config() {
  int offset=0;
  int i;
  byte *p;
  
  

  p=(byte *)config_zero_gauss;
  offset=0; 
  for (i=0;i<sizeof(config_zero_gauss);i++) {
    *p=EEPROM.read(offset);
    offset++;
    *p++;
  }

  if (config_zero_gauss[0]==-1 && config_zero_gauss[1]==-1) {
        int i;
        int data1[10];
        int data2[10];
        int px[2]={0,0};
        for (i=0;i<10;i++) {
          data1[i]=readvalConfig(1);
          data2[i]=readvalConfig(2);
        }
        for (i=0;i<10;i++) {
          px[0]+=data1[i];
          px[1]+=data2[i];
        }
        config_zero_gauss[0]=px[0]/10;
        config_zero_gauss[1]=px[1]/10;
        
  }

  p=(byte *)&config_address;
  for (i=0;i<sizeof(config_address);i++) {
    *p=EEPROM.read(offset);
    offset++;
    *p++;
  }
  p=(byte *)&rotation;
  for (i=0;i<sizeof(rotation);i++) {
    *p=EEPROM.read(offset);
    offset++;
    *p++;
  }
  Serial.print("address: ");
  Serial.print(config_address);
  Serial.print(", rotation: ");
  Serial.print(rotation);
  Serial.print(", zero: ");
  Serial.print(config_zero_gauss[0]);
  Serial.print(" ");
  Serial.println(config_zero_gauss[1]);
}

static boolean step8[8][4]={
  {1  ,0  ,1  ,0},
  {1  ,0  ,0  ,0},
  {1  ,0  ,0  ,1},
  {0  ,0  ,0  ,1},
  {0  ,1  ,0  ,1},
  {0  ,1  ,0  ,0},
  {0  ,1  ,1  ,0},
  {0  ,0  ,1  ,0}};


int readval(byte num) {
  ADMUX  = (1<<REFS0) | (num==1?(1<<MUX0):(1<<MUX1));
  ADCSRA |= (1<<ADSC);
  while(ADCSRA & (1<<ADSC));
  int a=ADC;
  return a;
}


void zero_hands() {
  unsigned int steps_to_do=12000;
  int max_value[2]={0,0};
  unsigned int max_position[2]={0,0};
  int step=0;
  int i;
  int value1,value2;
  int q=0;
  int skip_motor[2]={0,0};
  while (steps_to_do--) {
    run_motor(0);
    run_motor(1);
   
    q=(q+1)%16;
    if (!q==0) {
      
      value1=readval(motorSensor[0]);
      value1=abs(value1-config_zero_gauss[0]);
      //Serial.println(value1);
      if (max_value[1]<value1) {
       max_value[1]=value1;
       max_position[1]=steps_to_do; 
      }
      value2=readval(motorSensor[1]);
      value2=abs(value2-config_zero_gauss[1]);
      if (max_value[0]<value2) {
       max_value[0]=value2;
       max_position[0]=steps_to_do;     
      }
      value2=value2>>3;
      
      Serial.print(value2);Serial.print(",");
      if (value2>2) {
        //Serial.print("1");
      } else {
        //Serial.print("0");
      }
    } 
    _delay_us(500);
    /*if (max_value[0]>30 && max_value[1]>30 && value1<10 && value2<10) {
      max_position[0]-=steps_to_do;
      max_position[1]-=steps_to_do;
      break;
    }*/
  }
  set_direction(0,get_direction(0)*(-1));
  set_direction(1,get_direction(1)*(-1));
  
  int t;
  if (max_position[0]>max_position[1]) t=1; else t=0;
  
  while (max_position[t]--) {
    
    run_motor(0);
    run_motor(1);
    if (t==0) max_position[1]--; else max_position[0]--;
    _delay_us(500);
  }
 
  if (t==0) t=1; else t=0;
  while (max_position[t]--) {
    run_motor(t);
    _delay_us(500);
    
  }
  
  for (t=0;t<700;t++) {
    run_motor(0);
    run_motor(1);
    _delay_us(1450);
  }
  set_direction(0,get_direction(0)*(-1));
  set_direction(1,get_direction(1)*(-1));
  max_value[0]=0;
  max_value[1]=0;
  max_position[0]=0;
  max_position[1]=0;
  
  for (t=0;t<1000;t++) {
    run_motor(0);
     _delay_us(1450);
    if (t%5==0) {
      int q;
      value2=0;
      int vals[10];
      for (q=0;q<10;q++) {
      value1=readval(motorSensor[1]);
      value1=abs(value1-config_zero_gauss[1]);
      vals[q]=value1;
        _delay_us(200);
      }
      int countt[10]={0,0,0,0,0,0,0,0,0,0};
      int qq;
      for (q=0;q<10;q++) {
        for (qq=0;qq<10;qq++) {
          if (vals[q]==vals[qq]) countt[q]++;
        }
      }
      qq=0;
      for (q=0;q<10;q++) {
        if (countt[q]>2) { value2+=vals[q]; qq++; };
      }
      /*for (q=0;q<10;q++) {
        Serial.print(vals[q]);
        Serial.print(" (");
        Serial.print(countt[q]);
        Serial.print("),");
      }*/
       
      if (qq>0) value2=value2/qq;
      Serial.print(">>> ");
      Serial.print(value2);
      Serial.print(" ");
      if (max_value[0]<value2) {
        max_value[0]=value2;
        max_position[0]=1000-t; 
        Serial.print("X");    
      }
      Serial.println("");
    }   
   
  }
  set_direction(0,get_direction(0)*(-1));
  while (max_position[0]--) {
    run_motor(0);
    _delay_us(450);
  }

  max_value[0]=0;
  max_value[1]=0;
  max_position[0]=0;
  max_position[1]=0;
  
 for (t=0;t<1000;t++) {
    run_motor(1);
    if (t%5==0) {
      int q;
      value2=0;
      int vals[10];
      for (q=0;q<10;q++) {
      value1=readval(motorSensor[0]);
      value1=abs(value1-config_zero_gauss[0]);
      vals[q]=value1;
        _delay_us(200);
      }
      int countt[10]={0,0,0,0,0,0,0,0,0,0};
      int qq;
      for (q=0;q<10;q++) {
        for (qq=0;qq<10;qq++) {
          if (vals[q]==vals[qq]) countt[q]++;
        }
      }
      qq=0;
      for (q=0;q<10;q++) {
        if (countt[q]>2) { value2+=vals[q]; qq++; };
      }
      /*for (q=0;q<10;q++) {
        Serial.print(vals[q]);
        Serial.print(" (");
        Serial.print(countt[q]);
        Serial.print("),");
      }*/
       
      if (qq>0) value2=value2/qq;
      /*Serial.print(">>> ");
      Serial.print(value2);
      Serial.print(" ");*/
      if (max_value[0]<value2) {
        max_value[0]=value2;
        max_position[0]=1000-t; 
        //Serial.print("X");    
      }
      //Serial.println("");
    }   
    _delay_us(450);
  }
  set_direction(1,get_direction(1)*(-1));
  while (max_position[0]--) {
    run_motor(1);
    _delay_us(450);
  }
  

  _delay_ms(1000);
}
