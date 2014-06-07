#include <avr/eeprom.h>
#include <chibi.h>
#include <limits.h>
#include <stdint.h>
#include "board_specific_settings.h"
#include "netrad_ths.h"

#define SEPARATOR	"-----------------------------------------------------"
#define DEBUG		0

static char VERSION[] = "0.0.2";

String csvData = "";

unsigned long t0, t1;
unsigned long log_interval = 60000UL;			// in ms

// per channel
#define CHANNELS 2
int tubes[] = {2, 1};									// 0: LND-712, 1: SBM-20, 2: SI22G; see .h file
unsigned int beep_Hz[] = {800U, 2400U};
unsigned long counts_last[] = {0UL ,0UL};
unsigned long counts_total[] = {0UL, 0UL};
int pin_SPKR[] = {6, 6};
int pin_LED[] = {7, 7};
float CPM[] = {0.0, 0.0};
float DRE[] = {0.0, 0.0};
volatile unsigned char event[] = {0, 0};				// NOTE: those must be volitile
volatile unsigned int counts_per_sample[] = {0, 0};		// NOTE: those must be volatile

static FILE uartout = {0};		// needed for printf

// main program (setup/loop) {{{1
// ----------------------------------------------------------------------------

void setup()
{
	for (int c=0; c < CHANNELS; c++)
	{
		pinMode(pin_SPKR[c], OUTPUT);
		pinMode(pin_LED[c], OUTPUT);
	}

	// standard Chibi init
	fdev_setup_stream (&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
	stdout = &uartout ;
	chibiCmdInit(57600);
	chibiCmdAdd("help", cmdHelp);

	for (int c=0; c < CHANNELS; c++)			// beep each channel's buzzer 3 times
	{
		Serial.print("CH_");
		Serial.print(c);
		Serial.print(": ");
		Serial.println(GM_tubes[tubes[c]].name);

		tone(pin_SPKR[c], beep_Hz[c]); delay(100); noTone(pin_SPKR[c]); delay(50);
		tone(pin_SPKR[c], beep_Hz[c]); delay(100); noTone(pin_SPKR[c]); delay(50);
		tone(pin_SPKR[c], beep_Hz[c]); delay(100); noTone(pin_SPKR[c]); delay(50);
		delay(1000);
	}
	attachInterrupt(0, onPulse0, RISING);		// pin D2
	attachInterrupt(1, onPulse1, RISING);		// pin D3

	Serial.print("version "); Serial.println(VERSION);
	Serial.println("setup(): done.");
	Serial.println(SEPARATOR);
	Serial.println("t_0,total_0,last_0,CPM_0,DRE_0,total_1,last_1,CPM_1,DRE_1,t_1");
}

void loop()
{
	chibiCmdPoll()	;						// poll the serial for any input

	for (int c=0; c < CHANNELS; c++)		// check for any events, per channel
	{
		if (event[c])
		{
			event[c] = 0;

			tone(pin_SPKR[c], beep_Hz[c]);

			digitalWrite(pin_LED[c], HIGH); delay(20); digitalWrite(pin_LED[c], LOW);

			noTone(pin_SPKR[c]);
		}
	}

	if (elapsedTime(t0) >= log_interval)	// if time to log, do it
	{
		t0 = millis();						// <-- BEGIN TIME CRITICAL
		for (int c=0; c < CHANNELS; c++)
		{
			counts_last[c] = counts_per_sample[c];
			counts_per_sample[c] = 0;
		}
		t1 = millis();						// <-- END TIME CRITICAL
		for (int c=0; c < CHANNELS; c++)
		{
			counts_total[c] += counts_last[c];
			CPM[c] = 60000.0 * counts_last[c] / (float) log_interval;
			DRE[c] = CPM[c] * GM_tubes[tubes[c]].Cs137_coeff;
		}

		csvData = "";
		appendFloatValueAsString(csvData, (float)t0 / 1000.0);
		csvData += ",";
		for (int c=0; c < CHANNELS; c++)
		{
			csvData += counts_total[c] + ",";
			csvData += counts_last[c] + ",";
			appendFloatValueAsString(csvData, CPM[c]); csvData += ",";
			appendFloatValueAsString(csvData, DRE[c]); csvData += ",";
		}
		appendFloatValueAsString(csvData, (float)t1 / 1000.0);
		Serial.println(csvData);
	}
}

// ----------------------------------------------------------------------------
// }}}1

// utility functions {{{1
// ----------------------------------------------------------------------------

//**************************************************************************/
/*!
//  On each falling edge of the Geiger counter's output,
//  increment the counter and signal an event. The event
//  can be used to do things like pulse a buzzer or flash an LED
*/
/**************************************************************************/
void onPulse0()
{
	counts_per_sample[0]++;
	event[0] = 1;
}

void onPulse1()
{
	counts_per_sample[1]++;
	event[1] = 1;
}

/**************************************************************************/
// calculate elapsed time, taking into account rollovers
//
// NOTE: To test, modify hardware/arduino/cores/arduino/wiring.c:41 from 0 to:
//		volatile unsigned long timer0_millis = ULONG_MAX - (1000*10);
// then the time will overflow in 10s of running ;-)
//
/**************************************************************************/
unsigned long elapsedTime(unsigned long startTime)
{
	unsigned long stopTime = millis();

	if (startTime >= stopTime)
	{
		return startTime - stopTime;
	}
	else
	{
		return (ULONG_MAX - (startTime - stopTime));
	}
}

// implement the printf function from within arduino
/**************************************************************************/
static int uart_putchar (char c, FILE *stream)
{
		Serial.write(c) ;
		return 0 ;
}

/**************************************************************************/
/*!
// Since "+" operator doesn't support float values,
// convert a float value to a fixed point value "%+.3f"
// TODO: Evaluate against dtostrf from avr/stdlib.h
*/
/**************************************************************************/
void appendFloatValueAsString(String& outString, float value)
{
	long integerPortion = (long)value;
	long fractionalPortion = (value - integerPortion + 0.0005) * 1000;

	outString += integerPortion;
	outString += ".";

	if (fractionalPortion < 10)			// e.g. 9 > "00" + "9" = "009"
	{
		outString += "00";
	}
	else if (fractionalPortion < 100)	// e.g. 99 > "0" + "99" = "099"
	{
		outString += "0";
	}

	outString += fractionalPortion;
}

// chibiArduino specific CLI {{{2
// ----------------------------------------------------------------------------

/**************************************************************************/
// Print some help
/**************************************************************************/
void cmdHelp(int arg_cnt, char **args)
{
	printf_P(PSTR("No help yet, sorry!\r\n"));
}
// ----------------------------------------------------------------------------
// }}}2
// ----------------------------------------------------------------------------
// }}}1

// vim: set tabstop=4 shiftwidth=4 syntax=c foldmethod=marker :
