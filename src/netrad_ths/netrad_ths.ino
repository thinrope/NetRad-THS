// Connection:
// * An Arduino Ethernet Shield
// * D3: The output pin of the Geiger counter (active low)
//
// Reference:
// * http://www.sparkfun.com/products/9848

#include <SPI.h>
#include <Ethernet.h>
#include <avr/eeprom.h>
#include <chibi.h>
#include <limits.h>
#include <avr/wdt.h>
#include <stdint.h>
#include "board_specific_settings.h"
#include "netrad_ths.h"

#define SEPARATOR	"-----------------------------------------------------"
#define DEBUG		0

static char VERSION[] = "1.1.1";

// this holds the info for the device
static device_t dev;

// holds the control info for the device
static devctrl_t ctrl;

EthernetClient client;
IPAddress serverIP (173, 203, 98, 29);		// api.pachube.com
IPAddress localIP (10, 11, 12, 13);			// falback localIP address


String csvData = "";

// Sampling interval (e.g. 60,000ms = 1min)
unsigned long updateIntervalInMillis = 0;

// The next time to feed
unsigned long nextExecuteMillis = 0;

// Event flag signals when a geiger event has occurred
volatile unsigned char eventFlag = 0;		// FIXME: Can we get rid of eventFlag and use counts>0?
int counts_per_sample;

// The last connection time to disconnect from the server
// after uploaded feeds
long lastConnectionTime = 0;

// The conversion coefficient from cpm to µSv/h
float conversionCoefficient = 0;

// NetRAD - this is specific to the NetRAD board
int pinSpkr = 6;	// pin number of piezo speaker
int pinLED = 7;		// pin number of event LED
int resetPin = A1;
int radioSelect = A3;

static FILE uartout = {0};		// needed for printf

// main program (setup/loop) {{{1
// ----------------------------------------------------------------------------

void setup()
{
	byte *dev_ptr;
	pinMode(pinSpkr, OUTPUT);
	pinMode(pinLED, OUTPUT);

	// fill in the UART file descriptor with pointer to writer.
	fdev_setup_stream (&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);

	// The uart is the standard output device STDOUT.
	stdout = &uartout ;

	// init command line parser
	chibiCmdInit(57600);

	// put radio in idle state
	pinMode(radioSelect, OUTPUT);
	digitalWrite(radioSelect, HIGH);		// disable radio chip select

	// reset the Wiznet chip
	pinMode(resetPin, OUTPUT);
	digitalWrite(resetPin, HIGH);
	delay(20);
	digitalWrite(resetPin, LOW);
	delay(20);
	digitalWrite(resetPin, HIGH);

	// add in the commands to the command table
	chibiCmdAdd("getmac", cmdGetMAC);	
	chibiCmdAdd("setmac", cmdSetMAC);	
	chibiCmdAdd("getfeed", cmdGetFeedID);	
	chibiCmdAdd("setfeed", cmdSetFeedID);	
	chibiCmdAdd("getdev", cmdGetDevID);	
	chibiCmdAdd("setdev", cmdSetDevID);	
	chibiCmdAdd("stat", cmdStat);
	chibiCmdAdd("help", cmdHelp);

	// get the device info
	dev_ptr = (byte *)&dev;
	eeprom_read_block((byte *)&dev, 0, sizeof(device_t));

	// init the control info
	memset(&ctrl, 0, sizeof(devctrl_t));

	// enable watchdog to allow reset if anything goes wrong			
	wdt_enable(WDTO_8S);

	// Set the conversion coefficient from cpm to µSv/h
	switch (tubeModel)
	{
		case LND_712:
			// Reference:
			// http://www.lndinc.com/products/348/
			//
			// 1,000CPS ≒ 0.14mGy/h
			// 60,000CPM ≒ 140µGy/h
			// 1CPM ≒ 0.002333µGy/h
			conversionCoefficient = 0.002333;
			Serial.println("Sensor model: LND 712");
			break;
		case SBM_20:
			// Reference:
			// http://www.libelium.com/wireless_sensor_networks_to_control_radiation_levels_geiger_counters
			// using 25 cps/mR/hr for SBM-20. translates to 1 count = .0067 uSv/hr
			conversionCoefficient = 0.0067;
			Serial.println("Sensor model: SBM-20");
			Serial.println("Conversion factor: 150 cpm = 1 uSv/Hr");
			break;
		case INSPECTOR:
			Serial.println("Sensor Model: Medcom Inspector");
			Serial.println("Conversion factor: 310 cpm = 1 uSv/Hr");
			conversionCoefficient = 0.0029;
			break;
		default:
			Serial.println("Sensor model: UNKNOWN!");
			break;
	}

	tone(pinSpkr, 500); delay(100); noTone(pinSpkr);
	tone(pinSpkr, 1500); delay(50); noTone(pinSpkr);
	tone(pinSpkr, 500); delay(100); noTone(pinSpkr);

	cmdStat(0,0);

	// Initiate a DHCP session
	Serial.println("Getting an IP address...");
	if (Ethernet.begin(macAddress) == 0)
	{
		Serial.println("Failed to configure Ethernet using DHCP");
		// DHCP failed, so use a fixed IP address:
		Ethernet.begin(macAddress, localIP);
	}

	Serial.print("local_IP:\t");
	Serial.println(Ethernet.localIP());

	// Attach an interrupt to the digital pin and start counting
	//
	// Note:
	// Most Arduino boards have two external interrupts:
	// numbers 0 (on digital pin 2) and 1 (on digital pin 3)
	attachInterrupt(1, onPulse, interruptMode);
	updateIntervalInMillis = updateIntervalInMinutes * 60000;

	unsigned long now = millis();
	nextExecuteMillis = now + updateIntervalInMillis;

	// kick the dog
	wdt_reset();
	Serial.println("setup(): done.");
	Serial.println(SEPARATOR);
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

	// poll the command line for any input
	chibiCmdPoll();

	// maintain the DHCP lease, if needed
	Ethernet.maintain();

	if (DEBUG)
	{
		// Echo received strings to a host PC
		if (client.available())
		{
			char c = client.read();
			Serial.print(c);
		}

		if (client.connected() && (elapsedTime(lastConnectionTime) > 10000))
		{
			Serial.println();
			Serial.println("Disconnecting.");
			client.stop();
		}
	}

	// Add any geiger event handling code here
	if (eventFlag)
	{
		eventFlag = 0;					// clear the event flag for later use

		//Serial.print(".");
		tone(pinSpkr, 1000);			// beep the piezo speaker

		digitalWrite(pinLED, HIGH);		// flash the LED
		delay(20);
		digitalWrite(pinLED, LOW);

		noTone(pinSpkr);				// turn off the speaker
	}

	// check if its time to update server.
	// elapsedTime function will take into account counter rollover.
	if (elapsedTime(lastConnectionTime) < updateIntervalInMillis)
	{
		return;
	}

	float CPM = (float)counts_per_sample / (float)updateIntervalInMinutes;
	counts_per_sample = 0;

	updateDataStream(CPM);
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
void onPulse()
{
	counts_per_sample++;
	eventFlag = 1;
}

/**************************************************************************/
/*!
//  Send data to server
*/
/**************************************************************************/
void updateDataStream(float CPM) {
//	if (client.connected())
//	{
//		Serial.println("updateDataStream():: Disconnecting.");
//		client.stop();
//	}

	// Try to connect to the server
	Serial.println();
	Serial.println("updateDataStream():: Connecting to cosm.com ...");
	if (client.connect(serverIP, 80))
	{
		Serial.println("updateDataStream():: Connected");
		lastConnectionTime = millis();

		// clear the connection fail count if we have at least one successful connection
		ctrl.conn_fail_cnt = 0;
	}
	else
	{
		ctrl.conn_fail_cnt++;
		if (ctrl.conn_fail_cnt >= MAX_FAILED_CONNS)
		{
				ctrl.state = RESET;
		}
		printf("Failed. Retries left: %d.\n", MAX_FAILED_CONNS - ctrl.conn_fail_cnt);
		lastConnectionTime = millis();
		return;
	}

	// Convert from cpm to µSv/h with the pre-defined coefficient
	float DRE = CPM * conversionCoefficient;

	csvData = "0,";
	appendFloatValueAsString(csvData, CPM);
	csvData += "\n1,";
	appendFloatValueAsString(csvData, DRE);
	csvData += "\n13,";
	csvData += millis() / 1000;

	Serial.println("updateDataStream():: Sending the following data:");
	Serial.println(csvData);
	Serial.println(SEPARATOR);

	client.print("PUT /v2/feeds/");
	client.print(dev.feedID);
	client.println(" HTTP/1.1");
	client.println("User-Agent: Arduino");
	client.println("Host: api.pachube.com");
	client.print("X-PachubeApiKey: ");
	client.println(apiKey);
	client.print("Content-Length: ");
	client.println(csvData.length());
	client.println("Content-Type: text/csv");
	client.println();
	client.println(csvData);
}

/**************************************************************************/
// calculate elapsed time, taking into account rollovers
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
// convert a float value to a fixed point value
*/
/**************************************************************************/
void appendFloatValueAsString(String& outString,float value)
{
	int integerPortion = (int)value;
	int fractionalPortion = (value - integerPortion + 0.0005) * 1000;

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
// Get address
/**************************************************************************/
void cmdGetMAC(int arg_cnt, char **args)
{
	printf_P(PSTR("MAC_address:\t%04X\n"), dev.addr);
}

/**************************************************************************/
// Set address
/**************************************************************************/
void cmdSetMAC(int arg_cnt, char **args)
{
	dev.addr = strtol(args[1], NULL, 16);
	eeprom_write_block((byte *)&dev, 0, sizeof(device_t));
	printf_P(PSTR("MAC_address set to %04X\n"), dev.addr);
}

/**************************************************************************/
// Get the current feed ID
/**************************************************************************/
void cmdGetFeedID(int arg_cnt, char **args)
{
	printf_P(PSTR("Feed_ID:\t%u\n"), dev.feedID);
}

/**************************************************************************/
// Set the current feed ID
/**************************************************************************/
void cmdSetFeedID(int arg_cnt, char **args)
{
	dev.feedID = strtol(args[1], NULL, 10);
	eeprom_write_block((byte *)&dev, 0, sizeof(device_t));
	printf_P(PSTR("Feed ID set to %u\n"), dev.feedID);
}

void GetFirmwareVersion()
{
	printf_P(PSTR("Firmware_ver:\t%s\n"), VERSION);
}

/**************************************************************************/
// Get the device ID
/**************************************************************************/
void cmdGetDevID(int arg_cnt, char **args)
{
	printf_P(PSTR("Device_ID:\t%s\n"), dev.devID);
}

/**************************************************************************/
// Set the device ID
/**************************************************************************/
void cmdSetDevID(int arg_cnt, char **args)
{
	if (strlen(args[1]) < 10)
	{
		memcpy(dev.devID, args[1], strlen(args[1]) + 1);
		eeprom_write_block((byte *)&dev, 0, sizeof(device_t));
		printf_P(PSTR("Device ID set to %s\n"), dev.devID);
	}
	else
	{
		printf_P(PSTR("ERROR - Too many characters in Device ID. Must be under 10 characters.\n"));
	}
}

/**************************************************************************/
// Print out the current device ID
/**************************************************************************/
void cmdStat(int arg_cnt, char **args)
{
	cmdGetMAC(arg_cnt, args);
	cmdGetFeedID(arg_cnt, args);
	cmdGetDevID(arg_cnt, args);
	GetFirmwareVersion();
}

/**************************************************************************/
// Print some help
/**************************************************************************/
void cmdHelp(int arg_cnt, char **args)
{
	printf_P(PSTR("Use the following commands:\n"));
	printf_P(PSTR("\tgetmac:\t\tshows the chibi wireless MAC address.\n"));
	printf_P(PSTR("\tsetmac:\t\tsets the chibi wireless MAC address(0 .. FFFF).\n"));
	printf_P(PSTR("\tsetfeed:\tsets the COSM feed ID\n"));
	printf_P(PSTR("\tgetfeed:\tshows the COSM feed ID\n"));
	printf_P(PSTR("\tsetdev:\t\tsets the device ID (0 .. 10 chars)\n"));
	printf_P(PSTR("\tgetdev:\t\tshows the device ID\n"));
}
// ----------------------------------------------------------------------------
// }}}2
// ----------------------------------------------------------------------------
// }}}1

// vim: set tabstop=4 shiftwidth=4 syntax=c foldmethod=marker :
