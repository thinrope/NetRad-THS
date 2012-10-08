# for the arduinoChibi we need this
ARDUINO_SKETCHBOOK = $(HOME)/sketchbook/NetRad-THS

# hardware settings
BOARD_TAG = pro328
ARDUINO_PORT = /dev/ttyUSB0

# firmware settings
MONITOR_BAUDRATE = 57600
ARDUINO_LIBS = SPI Ethernet Ethernet/utility chibi 

# default setting for kalin@Gentoo
MONITOR_CMD = screen
ARDUINO_DIR = /usr/share/arduino
ARDMK_DIR = $(HOME)/sketchbook/NetRad-THS/tools/Arduino-Makefile
AVR_TOOLS_DIR = /usr

include ${ARDMK_DIR}/arduino-mk/Arduino.mk
