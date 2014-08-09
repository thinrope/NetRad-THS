#ifndef netrad_ths_h
#define netrad_ths_h

#include "Arduino.h"


// function prototypes
// ----------------------------------------------------------------------------

void POST_data(float);

/* interrupt handling routine */
void on_pulse(void);

/* utility funcions */
unsigned long ms_since(unsigned long);

/* debug related */
void DEBUG_event(void);
int DEBUG_free_RAM(void);

// ----------------------------------------------------------------------------
//

#endif

// vim: set tabstop=4 shiftwidth=4 syntax=c foldmethod=marker :
