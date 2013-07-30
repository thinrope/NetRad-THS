// NOTE:
// Before uploading to your Arduino board,
// please replace with your own settings

#define MAX_FAILED_CONNS 3

// Your API key (a public secure key is recommended)
const char *apiKey = "pcaztEazJwDTzzGK9Nsv";

// REPLACE WITH A PROPER MAC ADDRESS
byte macAddress[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xBA, 0xBE };

// Update interval in minutes
const int updateIntervalInMinutes = 1;

typedef struct
{
  unsigned short addr;
  unsigned long feedID;
  char devID[20];
  char firmware_version[10];
} device_t;

typedef struct
{
    unsigned char state;
    unsigned char conn_fail_cnt;
} devctrl_t;

enum TubeModel {
  LND_712,
  SBM_20,
  J408GAMMA,
  J306BETA,
  INSPECTOR,
  CRM100
};

enum states
{
    NORMAL = 0,
    RESET = 1
};

// Tube model
const TubeModel tubeModel = LND_712;

// Interrupt mode:
// * For most geiger counter modules: FALLING
// * Geiger Counter Twig by Seeed Studio: RISING
const int interruptMode = RISING;

// vim: set tabstop=4 shiftwidth=4 syntax=c foldmethod=marker :
