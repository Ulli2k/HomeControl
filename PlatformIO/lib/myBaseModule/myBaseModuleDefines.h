
#ifndef _MY_BASE_MODULE_DEFINES_h
#define _MY_BASE_MODULE_DEFINES_h

typedef uint8_t byte; //workarround because byte is defined in Arduino.h

#define MODULE_DATAPROCESSING    									0x00
	#define MODULE_DATAPROCESSING_QUIET	 								0x10
	#define MODULE_DATAPROCESSING_HELP									0x20
	#define MODULE_DATAPROCESSING_FIRMWARE							0x30
	#define MODULE_DATAPROCESSING_WAKE_SIGNAL						0x40
	#define MODULE_DATAPROCESSING_DEVICEID							0xE0	//internal use, no official command
	#define MODULE_DATAPROCESSING_OUTPUTTUNNEL					0xF0	//internal use, no official command
#define MODULE_ACTIVITY														0x01
	#define MODULE_ACTIVITY_RAMAVAILABLE									0x11
	#define MODULE_ACTIVITY_REBOOT												0x21
	#define MODULE_ACTIVITY_POWERDOWN											0x31
	#define MODULE_ACTIVITY_LOWPOWER											0x41
	#define MODULE_ACTIVITY_DUMP_REGS											0x51
// 	#define MODULE_AVR_BUZZER														0x71
#define MODULE_SERIAL           	 								0x02
#define MODULE_IRMP             									0x03
#define MODULE_RADIO															0x04
	#define MODULE_RADIO_OPTIONS												0x14
	#define MODULE_RADIO_MODEQUERY											0x24
	#define MODULE_RADIO_TUNNELING											0x34
	#define MODULE_RADIO_SENDACK												0xF4 //internal use, no official command
	#define MODULE_RADIO_OPTION_SEND										0xE4 //internal use, no official command
	#define MODULE_RADIO_OPTION_POWER										0xD4 //internal use, no official command
	#define MODULE_RADIO_OPTION_TEMP										0xC4 //internal use, no official command
#define MODULE_BME280															0x05
	#define MODULE_BME280_ENVIRONMENT										0x15
#define MODULE_POWERMONITOR												0x06
	#define MODULE_POWERMONITOR_POWER										0x16
#define MODULE_ROLLO															0x07
	#define MODULE_ROLLO_CMD														0x17
	#define MODULE_ROLLO_EVENT													0x27
#define MODULE_LED																0x08
	#define MODULE_LED_LED															0x18
	#define MODULE_LED_ACTIVITY													0x28
#define MODULE_DIGITAL_PIN												0x09
	#define MODULE_DIGITAL_PIN_SIMPLE										0x19
	#define MODULE_DIGITAL_PIN_RELAY_SR									0x29
	#define MODULE_DIGITAL_PIN_EVENT										0x39
#define MODULE_ANALOG_PIN													0x0A
	#define MODULE_ANALOG_PIN_VCC												0x1A
	#define MODULE_ANALOG_PIN_SAMPLE										0x2A



#define MODULE_TYP(m) 						(uint8_t)(m & 0x0F)
#define MODULE_PROTOCOL(m)				(m > 0x0F ? ( (m >> 4) -1) : 0)

#define MODULE_COMMAND_CHAR(c)		( {\
	 																		char b='-';\
																			const typeModuleInfo* pmt = ModuleTab;\
																			while(pmt->typecode >= 0) {\
																				if(pmt->typecode==MODULE_TYP(c)) {\
																					b=getFunctionChar(pmt->module->getFunctionCharacter(),MODULE_PROTOCOL(c));\
																					break;\
																				}\
																				pmt++;\
																			}\
																			b;\
																		} )
/*
#define MODULE_COMMAND_CHAR(m,c)  ( {\
	 																		char b='-';\
																			const typeModuleInfo* pmt = ModuleTab;\
																			while(pmt->typecode >= 0) {\
																				if(pmt->typecode==(m)) {\
																					b=getFunctionChar(pmt->getFunctionCharacter(),MODULE_PROTOCOL(c));\
																					break;\
																				}\
																				pmt++;\
																			}\
																			b;\
																		} )
*/


#if defined(__arm__) && !defined(PROGMEM)
	#define PROGMEM	//not needed for ARM, const is enough
#else
	#include <avr/pgmspace.h> //needed for PROGMEM on AVRs
#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
const char welcomeText[] PROGMEM =
	  "[RN"
	#if (DEVICE_ID<=9)
		"0"
	#endif
	  STR(DEVICE_ID)
	#if defined(STORE_CONFIGURATION)
		"s"
	#endif
	#ifdef HAS_ANALOG_PIN
		",C"
	#endif
	#ifdef HAS_DIGITAL_PIN
		",N|S"
	#endif
	#ifdef HAS_LEDs
		",L"
	#endif
	#ifdef HAS_ROLLO
		",J"
	#endif
	#ifdef HAS_BUZZER
		",B"
	#endif
	#ifdef HAS_RADIO
		",R"
		#if HAS_RADIO_LISTENMODE && !HAS_RADIO_TX_ONLY
		"l"
		#elif !HAS_RADIO_LISTENMODE && HAS_RADIO_TX_ONLY
		"t"
		#endif
		#if defined(HAS_RADIO_FORWARDING)
		"h"
		#elif defined(HAS_RADIO_TUNNELING)
		"s"
		#endif
	#endif
	#if (defined(HAS_IR_TX) || defined(HAS_IR_RX))
		",I"
		#if HAS_IR_TX
			"s"
		#endif
		#if HAS_IR_RX
			"r"
		#endif
	#endif
	#ifdef HAS_POWER_MONITOR_CT //|| HAS_POWER_MONITOR_PULSE
		",P"
	#endif
	#ifdef HAS_BME280
		",E"
	#endif
	"]";

#endif
