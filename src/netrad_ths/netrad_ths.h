#ifndef netrad_ths_h
#define netrad_ths_h

#include "Arduino.h"


// function prototypes {{{1
// ----------------------------------------------------------------------------

void GetFirmwareVersion(void);

/* interrupt handling routines */
void onPulse0(void);
void onPulse1(void);

/* workarounds and utility funcions */
unsigned long elapsedTime(unsigned long);
void appendFloatValueAsString(String& ,float);
static int uart_putchar (char, FILE *);


/* chibiArduino specific CLI */
void cmdHelp(int, char **);
// ----------------------------------------------------------------------------
// }}}1

#endif

// vim: set tabstop=4 shiftwidth=4 syntax=c foldmethod=marker :

