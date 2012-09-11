// Connection:
// * An Arduino Ethernet Shield
// * D3: The output pin of the Geiger counter (active low)
//
// Reference:
// * http://www.sparkfun.com/products/9848

#include <avr/eeprom.h>
#include <chibi.h>
#include <limits.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <stdlib.h>
#include "PrivateSettings.h"


// holds the control info for the device
static devctrl_t ctrl;

char* logRecord;
char* tmpbuf;

// log interval in ms
unsigned long logInterval = 12000; // 12s

// The next time to feed
unsigned long nextExecuteMillis;

// Value to store counts
volatile unsigned long counts;

// Event flag signals when a geiger event has occurred
volatile unsigned char eventFlag; 

unsigned long lastLogged;
unsigned long lastCounts;

// The conversion coefficient from CPM to µSv/h
float conversionCoefficient;

// NetRAD - this is specific to the NetRAD board
int pinSpkr = 6;	// pin number of piezo speaker
int pinLED = 7;		// pin number of event LED
int resetPin = A1;
int radioSelect = A3;

// this is for printf
static FILE uartout = {0};

/**************************************************************************/
/*!
//  On each falling edge of the Geiger counter's output,
//  increment the counter and signal an event. The event
//  can be used to do things like pulse a buzzer or flash an LED
*/
/**************************************************************************/
void onPulse()
{
	counts++;
	eventFlag = 1;
}

/**************************************************************************/
// calculate difference in unsigned long counters with overflow
/**************************************************************************/
unsigned long ulong_diff(unsigned long a, unsigned long b)
{
	return (a >= b) ? a - b : ULONG_MAX - (b - a);
}

// This is to implement the printf function from within arduino
/**************************************************************************/
static int uart_putchar (char c, FILE *stream)
{
	Serial.write(c) ;
	return 0 ;
}




void log_record()
{
	unsigned long current_counts = counts;
	unsigned long current_time = millis();
	unsigned long d_c = ulong_diff(current_counts, lastCounts);
	unsigned long d_time = ulong_diff(current_time, lastLogged);

	float CPM = (float)d_c / (float)d_time * 1000.0 * 60.0;
	float DRE = CPM * conversionCoefficient;

	Serial.print(dtostrf((float)current_time/1000.0, -8, 3, tmpbuf));
	Serial.print("\t");

	Serial.print(dtostrf((float)d_time/1000.0, -8, 3, tmpbuf));
	Serial.print("\t");

	Serial.print(ultoa(current_counts, tmpbuf, 10)); // ULONG_MAX is 4,294,967,295
	Serial.print("\t");

	Serial.print(ultoa(d_c, tmpbuf, 10));
	Serial.print("\t");

	Serial.print(dtostrf(CPM, +8, 3, tmpbuf));
	Serial.print("\t");

	Serial.print(dtostrf(DRE, +8, 3, tmpbuf));
	Serial.println();

	lastLogged = current_time;
	lastCounts = current_counts;
}



void setup()
{
	// fill in the UART file descriptor with pointer to writer.
	fdev_setup_stream (&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);


	// The uart is the standard output device STDOUT.
	stdout = &uartout ;

	// init command line parser
	chibiCmdInit(57600);

	// put radio in idle state
	pinMode(radioSelect, OUTPUT);
	digitalWrite(radioSelect, HIGH);	// disable radio chip select

	// init the control info
	memset(&ctrl, 0, sizeof(devctrl_t));

	// enable watchdog to allow reset if anything goes wrong
	wdt_enable(WDTO_8S);

	// Set the conversion coefficient from CPM to µSv/h
	switch (tubeModel)
	{
		case LND_712:
		// Reference:
		// http://www.lndinc.com/products/711/
		//
		// Co-60: 18 cps/mR/h
		// ??????????????????????
		// 60,000CPM ≒ 140µGy/h
		// 1CPM ≒ 0.002333µGy/h
		conversionCoefficient = 0.002333;
		Serial.println("#Sensor model: LND 712");
		Serial.println("#Conversion factor: 428.6 CPM = 1 uSv/h");	//FIXME: this des not sound right! I expect 138CPM 
		break;
	case SBM_20:
		// Reference:
		// http://www.libelium.com/wireless_sensor_networks_to_control_radiation_levels_geiger_counters
		// using 25 cps/mR/hr for SBM-20. translates to 1 count =0.0067 uSv/h
		conversionCoefficient = 0.0067;
		Serial.println("#Sensor model: SBM-20");
		Serial.println("#Conversion factor: 150 CPM = 1 uSv/h");
		break;
	case INSPECTOR:
		Serial.println("#Sensor Model: Medcom Inspector");
		Serial.println("#Conversion factor: 310 CPM = 1 uSv/h");
		conversionCoefficient = 0.0029;
		break;
	default:
		Serial.println("Sensor model: UNKNOWN!");
		break;
	}

	// Attach an interrupt to the digital pin and start counting
	//
	// Note:
	// Most Arduino boards have two external interrupts:
	// numbers 0 (on digital pin 2) and 1 (on digital pin 3)
	attachInterrupt(1, onPulse, interruptMode);

	lastLogged = millis();
	logRecord = (char*)(calloc(101, sizeof(char)));
	if (logRecord == NULL)
	{
		Serial.println("calloc failed!");
	}
	tmpbuf = (char*)(calloc(17, sizeof(char)));
	if (tmpbuf == NULL)
	{
		Serial.println("calloc failed!");
	}

	Serial.println();
	Serial.println();
	Serial.println();
	Serial.println("system_time\td_time\tcounts\td_c\tCPM\tDRE");
	log_record();

	// kick the dog
	wdt_reset();
}

void loop() 
{
	// kick the dog only if we're not in RESET state. if we're in RESET,
	// then that means something happened to mess up our connection and
	// we will just let the device gracefully reset
	if (ctrl.state != RESET)
	{
		wdt_reset();
	}

	// Add any geiger event handling code here
	if (eventFlag)
	{
		eventFlag = 0;			// clear the event flag for later use

		//Serial.print("."); 
		tone(pinSpkr, 1000);	// beep the piezo speaker

		digitalWrite(pinLED, HIGH); // flash the LED
		delay(20);
		digitalWrite(pinLED, LOW);

		noTone(pinSpkr);		// turn off the speaker pulse
	}

	if (ulong_diff(millis(),lastLogged) > logInterval)
	{
		log_record();
	}
}

// vim: set tabstop=4 shiftwidth=4 syntax=c foldmethod=marker :
