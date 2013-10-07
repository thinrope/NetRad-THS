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
#include "account.h"
#include "netrad_ths.h"

#define SEPARATOR	"-----------------------------------------------------"
#define DEBUG		0

static char VERSION[] = "1.1.2";

// this holds the info for the device
static device_t dev;

// holds the control info for the device
static devctrl_t ctrl;

EthernetClient client;

#define LINE_SZ 100
static char json_buf[LINE_SZ];

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
	chibiCmdAdd("getlat", cmdGetLatitude);
	chibiCmdAdd("setlat", cmdSetLatitude);
	chibiCmdAdd("getlon", cmdGetLongitude);
	chibiCmdAdd("setlon", cmdSetLongitude);
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

	Serial.println("");
	Serial.println("");
	cmdStat(0,0);

	tone(pinSpkr, 500); delay(100); noTone(pinSpkr);
	tone(pinSpkr, 1500); delay(50); noTone(pinSpkr);
	tone(pinSpkr, 500); delay(100); noTone(pinSpkr);


	// Initiate a DHCP session
	//Serial.println("Getting an IP address...");
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
	//Serial.println("setup(): done.");
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
			//Serial.println("Disconnecting.");
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
	String CPM_string = "";
	if (client.connected())
	{
		//Serial.println("updateDataStream():: Disconnecting.");
		client.stop();
	}

	// Try to connect to the server
	//Serial.println("updateDataStream():: Connecting to safecast.org ...");
	if (client.connect(serverIP, 80))
	{
		//Serial.println("updateDataStream():: Connected");
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
		printf("Failed. Retries left: %d.\r\n", MAX_FAILED_CONNS - ctrl.conn_fail_cnt);
		lastConnectionTime = millis();
		return;
	}

	// Convert from cpm to µSv/h with the pre-defined coefficient
	float DRE = CPM * conversionCoefficient;
	appendFloatValueAsString(CPM_string, CPM);

    // prepare the log entry
	memset(json_buf, 0, LINE_SZ);
	sprintf_P(json_buf, PSTR("{\"longitude\":\"%s\",\"latitude\":\"%s\",\"device_id\":\"%s\",\"value\":\"%s\",\"unit\":\"cpm\"}"),  \
	              dev.lon, \
	              dev.lat, \
	              dev.ID,  \
	              CPM_string.c_str());
	// FIXME: Need also "captured_at":"2012-09-27T02:55:28Z" and the whole RTC enchilada...

	int len = strlen(json_buf);
	json_buf[len] = '\0';
    // {"longitude":"139.695506","latitude":"35.656065","device_id":"1",value":"35","unit":"cpm"}

	//Serial.println("updateDataStream():: Sending the following data:");
	Serial.println(json_buf);
	//Serial.println(SEPARATOR);

	client.print("POST /measurements.json?api_key=");
	client.print(apiKey);
	client.println(" HTTP/1.1");
	client.println("User-Agent: Arduino");
	client.println("Host: api.safecast.org");
	client.print("Content-Length: ");
	client.println(strlen(json_buf));
	client.println("Content-Type: application/json");
	client.println();
	client.println(json_buf);
}

//**************************************************************************
// calculate elapsed time, taking into account rollovers
//**************************************************************************
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

//**************************************************************************
// implement the printf function from within arduino
//**************************************************************************
static int uart_putchar (char c, FILE *stream)
{
		Serial.write(c) ;
		return 0 ;
}

//**************************************************************************
// Since "+" operator doesn't support float values,
// convert a float value to a fixed point value "%+.3f"
//**************************************************************************
void appendFloatValueAsString(String& outString,float value)
{

	// FIXME: throw error on value > 4,294,967,295

	uint32_t integerPortion = (uint32_t)value;
	uint32_t fractionalPortion = (value - integerPortion + 0.0005) * 1000;

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

//**************************************************************************
// return simply the remaining RAM (not very accurate)
//**************************************************************************
int freeRAM ()
{
	extern int __heap_start, *__brkval;
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

// chibiArduino specific CLI {{{2
// ----------------------------------------------------------------------------

void print_OK(void)
{
	printf_P(PSTR("OK.\t["));
	Serial.print(freeRAM());
	printf_P(PSTR(" b free]\r\n"));
}

void cmdGetMAC(int arg_cnt, char **args)
{
	printf_P(PSTR("MAC_address:\t%04X\r\n"), dev.addr);
}

void cmdSetMAC(int arg_cnt, char **args)
{
	dev.addr = strtol(args[1], NULL, 16);
	eeprom_write_block((byte *)&dev, 0, sizeof(device_t));
	print_OK();
	cmdGetMAC(0,0);
}

void cmdGetLatitude(int arg_cnt, char **args)
{
	printf_P(PSTR("Latitude:\t%s\r\n"), dev.lat);
}

void cmdSetLatitude(int arg_cnt, char **args)
{
	if (strlen(args[1]) < 16)
	{
		memset(dev.lat, 0, sizeof(dev.lat));
		memcpy(dev.lat, args[1], strlen(args[1]) + 1);
		eeprom_write_block((byte *)&dev, 0, sizeof(device_t));
		print_OK();
		cmdGetLatitude(0,0);
	}
	else
	{
		printf_P(PSTR("ERROR - Too many characters in latitude. Must be under 16 characters.\r\n"));
	}
}

void cmdGetLongitude(int arg_cnt, char **args)
{
	printf_P(PSTR("Longitude:\t%s\r\n"), dev.lon);
}

void cmdSetLongitude(int arg_cnt, char **args)
{
	if (strlen(args[1]) < 16)
	{
		memset(dev.lon, 0, sizeof(dev.lon));
		memcpy(dev.lon, args[1], strlen(args[1]) + 1);
		eeprom_write_block((byte *)&dev, 0, sizeof(device_t));
		print_OK();
		cmdGetLongitude(0,0);
	}
	else
	{
		printf_P(PSTR("ERROR - Too many characters in longitude. Must be under 16 characters.\r\n"));
	}
}

void GetFirmwareVersion()
{
	printf_P(PSTR("Firmware_ver:\t%s\r\n"), VERSION);
}

void cmdGetDevID(int arg_cnt, char **args)
{
	printf_P(PSTR("Device_ID:\t%s\r\n"), dev.ID);
}

void cmdSetDevID(int arg_cnt, char **args)
{
	if (strlen(args[1]) < 10)
	{
		memcpy(dev.ID, args[1], strlen(args[1]) + 1);
		eeprom_write_block((byte *)&dev, 0, sizeof(device_t));
		printf_P(PSTR("Device ID set to %s\r\n"), dev.ID);
	}
	else
	{
		printf_P(PSTR("ERROR - Too many characters in Device ID. Must be under 10 characters.\r\n"));
	}
}

void cmdStat(int arg_cnt, char **args)
{
	cmdGetDevID(arg_cnt, args);
	cmdGetMAC(arg_cnt, args);
	cmdGetLatitude(arg_cnt, args);
	cmdGetLongitude(arg_cnt, args);
	GetFirmwareVersion();

	printf_P(PSTR("Free RAM:\t"));
	Serial.print(freeRAM());
	printf_P(PSTR(" bytes\r\n"));
}

void cmdHelp(int arg_cnt, char **args)
{
	printf_P(PSTR("Use the following set/get commands:\r\n"));
	printf_P(PSTR("\t{set|get}mac:\t\tchibi wireless MAC address (0 .. FFFF)\r\n"));
	printf_P(PSTR("\t{set|get}dev:\t\tthe device ID (need to be registered)\r\n"));
	printf_P(PSTR("\t{set|get}lat:\t\tthe device latitude\r\n"));
	printf_P(PSTR("\t{set|get}lon:\t\tthe device longitude\r\n"));
}
// ----------------------------------------------------------------------------
// }}}2
// ----------------------------------------------------------------------------
// }}}1

// vim: set tabstop=4 shiftwidth=4 syntax=c foldmethod=marker :
