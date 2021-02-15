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

#include "Arduino.h"
#include "font.h"

const int stoppositions[5]={0, 2699,5399, 8099, 9449/*1349*/};


void getdigitx(byte num, byte tt1, byte tt2, byte *t1, byte *t2) {
  if (num==0) {
    if (tt1==0 && tt2==0) {
       *t1=0;
       *t2=1;
       return;
    }
    if (tt1==0 && tt2==1) {
      *t1=3;
      *t2=0;
      return;
    }
    if (tt1==1 && tt2==0) {
      *t1=0;
      *t2=2;
      return;
    }
    if (tt1==1 && tt2==1) {
      *t1=0;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==0) {
      *t1=1;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==1) {
      *t1=2;
      *t2=3;
      return;
    }
  }
  if (num==1) {
    if (tt1==0 && tt2==0) {
       *t1=4;
       *t2=4;
       return;
    }
    if (tt1==0 && tt2==1) {
      *t1=0;
      *t2=0;
      return;
    }
    if (tt1==1 && tt2==0) {
      *t1=4;
      *t2=4;
      return;
    }
    if (tt1==1 && tt2==1) {
      *t1=0;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==0) {
      *t1=4;
      *t2=4;
      return;
    }
    if (tt1==2 && tt2==1) {
      *t1=2;
      *t2=2;
      return;
    }
  }
  if (num==2) {
    if (tt1==0 && tt2==0) {
       *t1=0;
       *t2=1;
       return;
    }
    if (tt1==0 && tt2==1) {
      *t1=3;
      *t2=3;
      return;
    }
    if (tt1==1 && tt2==0) {
      *t1=2;
      *t2=1;
      return;
    }
    if (tt1==1 && tt2==1) {
      *t1=3;
      *t2=0;
      return;
    }
    if (tt1==2 && tt2==0) {
      *t1=1;
      *t2=1;
      return;
    }
    if (tt1==2 && tt2==1) {
      *t1=3;
      *t2=2;
      return;
    }
  }

  if (num==3) {
    if (tt1==0 && tt2==0) {
       *t1=1;
       *t2=1;
       return;
    }
    if (tt1==0 && tt2==1) {
      *t1=3;
      *t2=0;
      return;
    }
    if (tt1==1 && tt2==0) {
      *t1=1;
      *t2=1;
      return;
    }
    if (tt1==1 && tt2==1) {
      *t1=0;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==0) {
      *t1=1;
      *t2=1;
      return;
    }
    if (tt1==2 && tt2==1) {
      *t1=3;
      *t2=2;
      return;
    }
  }

  if (num==4) {
    if (tt1==0 && tt2==0) {
       *t1=4;
       *t2=4;
       return;
    }
    if (tt1==0 && tt2==1) {
      *t1=0;
      *t2=0;
      return;
    }
    if (tt1==1 && tt2==0) {
      *t1=0;
      *t2=1;
      return;
    }
    if (tt1==1 && tt2==1) {
      *t1=0;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==0) {
      *t1=2;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==1) {
      *t1=2;
      *t2=2;
      return;
    }
  }

  if (num==5) {
    if (tt1==0 && tt2==0) {
       *t1=1;
       *t2=1;
       return;
    }
    if (tt1==0 && tt2==1) {
      *t1=0;
      *t2=3;
      return;
    }
    if (tt1==1 && tt2==0) {
      *t1=0;
      *t2=1;
      return;
    }
    if (tt1==1 && tt2==1) {
      *t1=3;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==0) {
      *t1=1;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==1) {
      *t1=3;
      *t2=3;
      return;
    }
  }

  if (num==6) {
    if (tt1==0 && tt2==0) {
       *t1=0;
       *t2=1;
       return;
    }
    if (tt1==0 && tt2==1) {
      *t1=0;
      *t2=3;
      return;
    }
    if (tt1==1 && tt2==0) {
      *t1=2;
      *t2=0;
      return;
    }
    if (tt1==1 && tt2==1) {
      *t1=3;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==0) {
      *t1=1;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==1) {
      *t1=3;
      *t2=3;
      return;
    }
  }

  if (num==7) {
    if (tt1==0 && tt2==0) {
       *t1=4;
       *t2=4;
       return;
    }
    if (tt1==0 && tt2==1) {
      *t1=0;
      *t2=0;
      return;
    }
    if (tt1==1 && tt2==0) {
      *t1=4;
      *t2=4;
      return;
    }
    if (tt1==1 && tt2==1) {
      *t1=0;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==0) {
      *t1=1;
      *t2=1;
      return;
    }
    if (tt1==2 && tt2==1) {
      *t1=2;
      *t2=3;
      return;
    }
  }


  if (num==8) {
    if (tt1==0 && tt2==0) {
       *t1=0;
       *t2=1;
       return;
    }
    if (tt1==0 && tt2==1) {
      *t1=0;
      *t2=3;
      return;
    }
    if (tt1==1 && tt2==0) {
      *t1=1;
      *t2=2;
      return;
    }
    if (tt1==1 && tt2==1) {
      *t1=3;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==0) {
      *t1=1;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==1) {
      *t1=3;
      *t2=2;
      return;
    }
  }


  if (num==9) {
    if (tt1==0 && tt2==0) {
       *t1=1;
       *t2=1;
       return;
    }
    if (tt1==0 && tt2==1) {
      *t1=0;
      *t2=3;
      return;
    }
    if (tt1==1 && tt2==0) {
      *t1=0;
      *t2=1;
      return;
    }
    if (tt1==1 && tt2==1) {
      *t1=0;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==0) {
      *t1=1;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==1) {
      *t1=3;
      *t2=2;
      return;
    }
  }
  if (num=='X') {
    if (tt1==0 && tt2==0) {
       *t1=4;
       *t2=4;
       return;
    }
    if (tt1==0 && tt2==1) {
      *t1=4;
      *t2=4;
      return;
    }
    if (tt1==1 && tt2==0) {
      *t1=4;
      *t2=4;
      return;
    }
    if (tt1==1 && tt2==1) {
      *t1=4;
      *t2=4;
      return;
    }
    if (tt1==2 && tt2==0) {
      *t1=4;
      *t2=4;
      return;
    }
    if (tt1==2 && tt2==1) {
      *t1=4;
      *t2=4;
      return;
    }
  }

  if (num=='o') {
    if (tt1==0 && tt2==0) {
       *t1=4;
       *t2=4;
       return;
    }
    if (tt1==0 && tt2==1) {
      *t1=4;
      *t2=4;
      return;
    }
    if (tt1==1 && tt2==0) {
      *t1=0;
      *t2=1;
      return;
    }
    if (tt1==1 && tt2==1) {
      *t1=3;
      *t2=0;
      return;
    }
    if (tt1==2 && tt2==0) {
      *t1=1;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==1) {
      *t1=3;
      *t2=2;
      return;
    }
  }

  if (num=='C') {
    if (tt1==0 && tt2==0) {
       *t1=0;
       *t2=1;
       return;
    }
    if (tt1==0 && tt2==1) {
      *t1=3;
      *t2=3;
      return;
    }
    if (tt1==1 && tt2==0) {
      *t1=0;
      *t2=2;
      return;
    }
    if (tt1==1 && tt2==1) {
      *t1=4;
      *t2=4;
      return;
    }
    if (tt1==2 && tt2==0) {
      *t1=1;
      *t2=2;
      return;
    }
    if (tt1==2 && tt2==1) {
      *t1=3;
      *t2=3;
      return;
    }
  }
  
}

void getdigit(byte num, byte tt1, byte tt2, unsigned int *p1, unsigned int *p2) {
  byte t1;
  byte t2;
  getdigitx(num,tt1,tt2,&t1,&t2);
  *p1=stoppositions[t1];
  *p2=stoppositions[t2];
}
