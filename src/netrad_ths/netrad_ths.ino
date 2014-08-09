#include <SPI.h>		// needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <util.h>
#include <limits.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include "board_specific_settings.h"
#include "netrad_ths.h"

#define DEBUG		0
#define SEPARATOR	Serial.println(F("|----------------------------------------"));

#define _SS_MAX_RX_BUFF 128 	// FIXME: Do we need software serial? RX buffer size

#define LINE_SZ 100

static char json_buf[LINE_SZ];

// Sampling interval (e.g. 60,000ms = 1min)
unsigned long const POST_interval = nGeigie.API_update_seconds * 1000UL;

// The last time
unsigned long last_POST_success = 0UL;


// Event flag signals when a geiger event has occurred
volatile unsigned char eventFlag = 0;		// FIXME: Can we get rid of eventFlag and use counts>0?
volatile unsigned long counts_per_sample = 0UL;	// 32-bit, max 4G; about 24h @ 50Kcps

static FILE uartout = {0};		// needed for printf

unsigned long tmp_counts = 0UL;
unsigned long tmp_ms = 1UL;	//FIXME: divzero?
float CPM = 0.0;

// main program (setup/loop) {{{1
// ----------------------------------------------------------------------------
void setup() {

	stdout = &uartout ;
	Serial.begin(57600);

	Serial.println(F("| INIT: STATION"));
	SEPARATOR

	pinMode(pin_LED, OUTPUT);
	// beep 3 times as a greeting
	pinMode(pin_piezo, OUTPUT);
	tone(pin_piezo, 500); delay(100); noTone(pin_piezo);
	tone(pin_piezo, 1500); delay(50); noTone(pin_piezo);
	tone(pin_piezo, 500); delay(100); noTone(pin_piezo);

	Serial.print(F("| Device ID:\t"));		Serial.println(nGeigie.ID);
	Serial.print(F("| Firmware:\t"));		Serial.println(nGeigie.firmware_version);
	Serial.println();

	Serial.println(F("| INIT: GEIGER"));
	SEPARATOR
	attachInterrupt(1, on_pulse, interrupt_mode);
	Serial.print(F("| GM tube:\t"));		Serial.println(nGeigie.sensor);
	Serial.print(F("| CPM2DRE:\t"));		Serial.println(nGeigie.CPM2DRE);
	Serial.println();

	Serial.println(F("| INIT: NETWORK"));
	SEPARATOR

	// reset the Wiznet chip
	pinMode(pin_wiznet_reset, OUTPUT);
	digitalWrite(pin_wiznet_reset, HIGH); delay(20);
	digitalWrite(pin_wiznet_reset, LOW); delay(20);
	digitalWrite(pin_wiznet_reset, HIGH);

	wdt_enable(WDTO_8S);
	if (Ethernet.begin(macAddress) == 0)
	{
		Serial.println(F("][ ERROR: failed DHCP! resetting..."));
		delay(500);
		device_reset();
	}
	else
	{
		wdt_reset();
		Serial.print(F("| IP address:\t")); Serial.println(Ethernet.localIP());
	}
	Serial.println();
	

	Serial.println(F("| INIT: RADIO"));
	SEPARATOR

	// put radio in idle state
	pinMode(pin_radio_select, OUTPUT);
	digitalWrite(pin_radio_select, HIGH);
	Serial.print(F("| status:\tpin ")); Serial.print(pin_radio_select); Serial.println(F(" disabled."));
	Serial.println();

	Serial.println();

	Serial.print(F("| MONITOR: POSTing every "));
	Serial.print(nGeigie.API_update_seconds);
	Serial.println(F(" s"));
	SEPARATOR
}

void loop()
{
	if (device_state != _DEVICE_RESET)
	{
		wdt_reset();
	}
	Ethernet.maintain();

	// Add any geiger event handling code here
	if (eventFlag)
	{
		eventFlag = 0;
		DEBUG_event();
		tone(pin_piezo, 2000);

		digitalWrite(pin_LED, HIGH);		// flash the LED
		delay(7);
		digitalWrite(pin_LED, LOW);

		noTone(pin_piezo);
	}

	// time to update server?
	tmp_ms = ms_since(last_POST_success);
	if (tmp_ms >= POST_interval)
	{
		// read and reset the counter
		tmp_counts = counts_per_sample;		// FIXME: loosing counts on unsuccessfull update here!
		counts_per_sample = 0UL;

		CPM = (float)tmp_counts / ((float)tmp_ms / 60000.0);

		POST_data(CPM);
	}
	// FIXME: add basic serial port comm (e.g. Enter for current reading)
}

// ----------------------------------------------------------------------------
// }}}1

// utility functions {{{1
// ----------------------------------------------------------------------------

void POST_data(float CPM)
{
	if (client.connected())
	{
		client.stop();
	}

	// Try to connect to the server
	if (client.connect(server_IP, server_port))
	{
		//Serial.println("\b\b\b: connected.");
		last_POST_success = millis();

		// clear the connection fail count if we have at least one successful connection
		device_connection_failures = 0;

		char CPM_string[16];
		dtostrf(CPM, 0, 1, CPM_string);

		memset(json_buf, 0, LINE_SZ);
		sprintf_P(json_buf, PSTR("{\"longitude\":\"%s\",\"latitude\":\"%s\",\"device_id\":\"%s\",\"value\":\"%s\",\"unit\":\"cpm\"}"), nGeigie.lon, nGeigie.lat, nGeigie.ID, CPM_string);

		int len = strlen(json_buf);
		json_buf[len] = '\0';

		client.print(F("POST /scripts/index.php?api_key=")); client.print(nGeigie.API_key); client.println(F(" HTTP/1.1")); // NOTE: keep the space
		client.println(F("Accept: application/json"));
		client.print(F("Host: ")); client.println(nGeigie.API_endpoint);
		client.print(F("Content-Length: ")); client.println(strlen(json_buf));
		client.println(F("Content-Type: application/json"));
		client.println();		// FIXME: only single CRNL is OK?
		client.println(json_buf);
		client.stop();

		Serial.println(json_buf);
	}
	else
	{
		Serial.println(F("ERROR: POST failed!"));	//FIXME: in that case data needs to be buffered locally
		device_connection_failures++;
		if (device_connection_failures >= _CONNECTION_FAILURES_MAX)
		{
			device_state = _DEVICE_RESET;
		}
		last_POST_success = millis();
		return;
	}

}


unsigned long ms_since(unsigned long startTime)
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

void on_pulse()
{
	counts_per_sample++;
	eventFlag = 1;
}

#if DEBUG
void DEBUG_event(void)
{
	Serial.print(millis()); Serial.print("\t");
	Serial.print(tmp_ms); Serial.print(" / "); Serial.print((unsigned long)POST_interval); Serial.print("\t");
	Serial.print(counts_per_sample); Serial.print("\t");
	Serial.print(DEBUG_free_RAM());
	Serial.println();
}

int DEBUG_free_RAM (void)
{
	extern int __heap_start, *__brkval;
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
#else
void DEBUG_event(void){};
int DEBUG_free_RAM(void){};
#endif

// ----------------------------------------------------------------------------
// }}}1

// vim: set tabstop=4 shiftwidth=4 syntax=c foldmethod=marker :
