#include <Arduino.h>
#include "config.h"

const char *ssid     = "ssid";
const char *password = "password";


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
    {7, 38}
  }
};

int16_t offsets[4][3][2] = {
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
