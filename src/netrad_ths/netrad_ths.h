#ifndef netrad_ths_h
#define netrad_ths_h

#include "Arduino.h"


// function prototypes {{{1
// ----------------------------------------------------------------------------

void updateDataStream(float);
void GetFirmwareVersion(void);

/* interrupt handling routine */
void onPulse(void);

/* workarounds and utility funcions */
unsigned long elapsedTime(unsigned long);
void appendFloatValueAsString(String& ,float);
static int uart_putchar (char, FILE *);


/* chibiArduino specific CLI */
void cmdGetMAC(int, char **);
void cmdSetMAC(int, char **);
void cmdGetDevID(int, char **);
void cmdSetDevID(int, char **);
void cmdGetLatitude(int, char **);
void cmdSetLatitude(int, char **);
void cmdGetLongitude(int, char **);
void cmdSetLongitude(int, char **);
void cmdStat(int, char **);
void cmdHelp(int, char **);
// ----------------------------------------------------------------------------
// }}}1

#endif

// vim: set tabstop=4 shiftwidth=4 syntax=c foldmethod=marker :

