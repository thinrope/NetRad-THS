//
// XXX ATMega328P: 31 KB Flash (1KB bootloader), 2KB SRAM, 1KB EEPROM
// TODO: Use PROGMEM: http://arduino.cc/en/Reference/PROGMEM
//


// description of a GM tube
struct GM_tube
{
  char name[16];
  int voltage;
  float own_background;
  float dead_time;
  float Cs137;					// CPM * (1 / coeff) = µSv/h
};

extern const GM_tube GM_tubes[3] = 
{
	//	name	 V		dead	own		Cs137	
	{"LND-712",	500,	0.0,	60.0,	142.85},
	{"SBM-20",	500,	0.0,	60.0,	150.00},
	{"SI22G",	380,	0.0,	75.0,	633.00}
};

// vim: set tabstop=4 shiftwidth=4 syntax=c foldmethod=marker :
