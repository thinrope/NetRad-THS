#define _CONNECTION_FAILURES_MAX 3
#define _DEVICE_NORMAL 0
#define _DEVICE_RESET 1

#define __STR_HELPER(x) #x
#define __STR(x) __STR_HELPER(x)
#define _FULL_VERSION "2.0.0-devel [" __STR(VERSION_COMMIT) "]"

// FIXME: dump to EEPROM instead?
static struct
{
	char const * ID = "80";
	char const * lat = "+34.56" ;
	char const * lon = "+140.80" ;
	char const * sensor = "SI-180G";
	char const * location_country = "Japan";
	char const * location_other = "Tokyo-to, Minato-ku";
	float const CPM2DRE = 8.33E-3;

	char const * firmware_version = _FULL_VERSION;

	char const * API_acount = "kalin@safecast.org";
	char const * API_key = "puEMBiqTsN8EfbJh5Tto";
	char const * API_endpoint = "107.161.164.166";		//  IP of realtime.safecast.org
	unsigned int const API_update_seconds = 300UL;		// default: 300s -> 300UL
} nGeigie;

char device_state = 0;
char device_connection_failures = 0;
void  (* device_reset) (void) = 0;	// usage: device_reset();


EthernetClient client;
byte macAddress[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x13, 0x80 };
IPAddress server_IP(107, 161, 164, 166);  // FIXME: IP of realtime.safecast.org
int const server_port = 80;

// Hardware configuration for NetRAD-THS + Freakduino
int const pin_piezo = 6;		// geiger clicker, on the NetRAD
int const pin_LED = 7;			// geiger LED, on the NetRAD
int const pin_wiznet_reset = 15;	// LAN chip reset pin, on the NetRAD
int const pin_radio_select = 17; 	// Radio select pin, on the Freakduino FIXME: const
int const interrupt_mode = RISING;	// NetRAD produces positive pulses => RISING
