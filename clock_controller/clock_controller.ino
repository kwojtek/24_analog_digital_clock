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

#include "DS3231.h"
#include <Wire.h>
#include "font.h"

static DS3231 clock;
int set_positions[8][3][2];
byte set_directions[8][3][2];
int get_positions[8][3][2];
unsigned int set_speeds[8][3];
static RTCDateTime dt;

byte clock_address[4][3][2] = {
  { {21, 22},
    {11, 12},
    {01, 02}
  },

  { {23, 24},
    {13, 14},
    {03, 04}
  },

  { {25, 26},
    {15, 16},
    {05, 06}
  },

  { {27, 28},
    {17, 18},
    {07, 8}
  }
};

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
void show_time_speed(int spd) {
  byte digital[4] = {0, 0, 0, 0};
  byte digit;
  byte ypos;
  byte xpos;
  int backup;

  dt = clock.getDateTime();
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
}

void loop() {
  // put your main code here, to run repeatedly:

}
