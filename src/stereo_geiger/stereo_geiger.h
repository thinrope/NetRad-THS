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
void appendFloatValueAsString(String&, float);

// ----------------------------------------------------------------------------
// }}}1

#endif

// vim: set tabstop=4 shiftwidth=4 syntax=c foldmethod=marker :
