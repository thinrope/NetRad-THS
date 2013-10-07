#ifndef BOARD_SPECIFIC_SETTINGS_H
#define BOARD_SPECIFIC_SETTINGS_H
// NOTE:
// Before uploading to your Arduino board,
// please replace with your own settings

#define MAX_FAILED_CONNS 3

// REPLACE WITH A PROPER MAC ADDRESS for the Etherenet PHY
byte macAddress[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xBA, 0xBE };

// Update interval in minutes
const int updateIntervalInMinutes = 1;

typedef struct
{
  unsigned short addr;
  char lat[20];
  char lon[20];
  char ID[20];
  char firmware_version[10];
} device_t;

typedef struct
{
    unsigned char state;
    unsigned char conn_fail_cnt;
} devctrl_t;

enum states
{
	NORMAL	= 0,
	RESET	= 1,
};

// Tube model
//LND712:	0.01
//Inspector:	0.0029
//SBM-20:	0.0067

// The conversion coefficient from cpm to µSv/h
const float conversionCoefficient = 0.0067;

// Interrupt mode:
// * For most geiger counter modules: FALLING
// * Geiger Counter Twig by Seeed Studio: RISING
const int interruptMode = RISING;

// NetRAD - this is specific to the NetRAD board
const int pinSpkr = 6;	// pin number of piezo speaker
const int pinLED = 7;		// pin number of event LED
const int resetPin = A1;
const int radioSelect = A3;

#endif BOARD_SPECIFIC_SETTINGS_H
// vim: set tabstop=4 shiftwidth=4 syntax=c foldmethod=marker :
