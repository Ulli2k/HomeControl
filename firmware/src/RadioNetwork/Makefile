##### Beispiel: make freq=16 target=PowerNode

### DISCLAIMER
### This is an example Makefile and it MUST be configured to suit your needs.
### For detailled explanations about all the avalaible options,
### please refer to https://github.com/sudar/Arduino-Makefile/blob/master/arduino-mk-vars.md

### PROJECT_DIR
### This is the path to where you have created/cloned your project
PROJECT_DIR       = /home/ulli/SubVersion/firmware

### AVR_GCC_VERSION
### Check if the version is equal or higher than 4.9
AVR_GCC_VERSION  := $(shell expr `avr-gcc -dumpversion | cut -f1` \>= 4.9)

### ARDMK_DIR
### Path to the Arduino-Makefile directory.
ARDMK_DIR         = $(PROJECT_DIR)/Arduino-Makefile

### ARDUINO_DIR
### Path to the Arduino application and ressources directory.
ARDUINO_DIR       = /opt/arduino

### USER_LIB_PATH
### Path to where the your project's libraries are stored.
USER_LIB_PATH     :=  $(realpath $(PROJECT_DIR)/lib)

### BOARD_TAG & BOARD_SUB
### For Arduino IDE 1.0.x
### Only BOARD_TAG is needed. It must be set to the board you are currently using. (i.e uno, mega2560, etc.)
# BOARD_TAG         = mega2560
### For Arduino IDE 1.6.x
### Both BOARD_TAG and BOARD_SUB are needed. They must be set to the board you are currently using. (i.e BOARD_TAG = uno, mega, etc. & BOARD_SUB = atmega2560, etc.)
### Note: for the Arduino Uno, only BOARD_TAG is mandatory and BOARD_SUB can be equal to anything
BOARDS_TXT				= $(PROJECT_DIR)/hardware/atmega328p/boards.txt
ifeq	"$(freq)" "8"
	BOARD_TAG         = mySatellite_8Mhz
else
	BOARD_TAG         = mySatellite_16Mhz
endif


#Basic Libraries
ARDUINO_LIBS			 = myBaseModule RingBuffer myTiming myDataProcessing
ARDUINO_LIBS			+= myUart
ARDUINO_LIBS			+= myAVR

### MONITOR_BAUDRATE
### It must be set to Serial baudrate value you are using.
MONITOR_BAUDRATE  	= 57600
MONITOR_PORT				= /dev/ttyUSB0
MONITOR_CMD					= picocom
MONITOR_PARAMS			= --echo

### FLASH_BAUDRATE for esp-link
FORCE_FLASH_PORT		= 0
FLASH_BAUDRATE			= 19200
FLASH_HOSTNAME			= esp
	
### AVR_TOOLS_DIR
### Path to the AVR tools directory such as avr-gcc, avr-g++, etc.
AVR_TOOLS_DIR     = /usr

### AVRDDUDE
### Path to avrdude directory.
AVRDDUDE          = /usr/bin/avrdude

### CFLAGS_STD
CFLAGS_STD        = -std=gnu11

### CXXFLAGS_STD
CXXFLAGS_STD      = -std=gnu++11

### CPPFLAGS
### Flags you might want to set for debugging purpose. Comment to stop.
CXXFLAGS         = -pedantic -Wall -Wextra
CPPFLAGS         = -DVERSION=\"$(VERSION)\"

#Load Board Specific Configuration
ifneq "$(target)" ""
	include ./$(target).mk
else
	DEVICE_ID		= 1
endif
#ENCRYPTKEY	= "LHRia3xSeoJHKe7)"
VERSION			= "1"

CPPFLAGS         += -DDEVICE_ID=$(DEVICE_ID) -DDEVICE_ID_BROADCAST=0
CPPFLAGS         += -DINCLUDE_HELP 
#CPPFLAGS				 +=	-DINCLUDE_DEBUG_OUTPUT

CPPFLAGS				 += -DHAS_UART -DSER_BAUD=$(MONITOR_BAUDRATE)
CPPFLAGS				 += -DHAS_AVR

### If avr-gcc -v is higher than 4.9, activate coloring of the output
ifeq "$(AVR_GCC_VERSION)" "1"
    CXXFLAGS += -fdiagnostics-color
endif

### MONITOR_PORT
### The port your board is connected to. Using an '*' tries all the ports and finds the right one.
#MONITOR_PORT      = -P net:beagle:4001

### don't touch this
CURRENT_DIR       = $(shell basename $(CURDIR))

### OBJDIR
### This is were you put the binaries you just compile using 'make'
CURRENT_DIR       = $(shell basename $(CURDIR))
OBJDIR            = $(PROJECT_DIR)/bin/$(CURRENT_DIR)/$(BOARD_TAG)


ifeq "$(FORCE_FLASH_PORT)" "1"
PRE_FLASH_CMD 	= curl -s -w -XPOST 'http://$(FLASH_HOSTNAME)/console/baud?rate=$(FLASH_BAUDRATE)' > /dev/null
POST_FLASH_CMD 	= curl -s -w -XPOST 'http://$(FLASH_HOSTNAME)/console/baud?rate=$(MONITOR_BAUDRATE)' > /dev/null
endif

### path to Arduino.mk, inside the ARDMK_DIR, don't touch.
include $(ARDMK_DIR)/Arduino.mk


