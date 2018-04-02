#ifndef _MY_AVR_MODULE_h
#define _MY_AVR_MODULE_h

#if defined(__AVR_ATmega328P__)
#include <avr/wdt.h> // Reset_AVR
#endif
#include <myBaseModule.h>
//#include "LowPower.h"

//#define LOWPOWER_MAX_IDLETIME			1000		//ms

#ifdef HAS_TRIGGER
	#include <myAVR_interrupt.h>
#endif

class myAVR : public myBaseModule {

private:

#ifdef READVCC_CALIBRATION_CONST
	bool initialVCCSample:1;
#endif

	static uint16_t AVR_InternalReferenceVoltage;
#if HAS_POWER_OPTIMIZATIONS
	static byte AVR_Auto_LowPower;
	static uint8_t AVR_LowPower_InfoPoll_Counter;
	#if INCLUDE_DEBUG_OUTPUT
	unsigned long awakeTime;
	#endif
#endif

#if HAS_LEDs
	#ifdef LED_ACTIVITY
	static unsigned long DelayActivityLED;
	static byte activityLEDActive;
	static byte blockActivityLED;
	#endif
	static void LedOnOff (uint8_t id, uint8_t on);
#endif

#ifdef HAS_BUZZER
	static void BuzzerTime (const char* cmd);
#endif
#if HAS_SWITCH
	static void SwitchOnOff(const char* cmd);
#endif
/*#if HAS_ROLLO_SWITCH*/
/*	static void SwitchUpDown(const char* cmd);*/
/*#endif*/

	int getAvailableRam();

#if HAS_ADC
	unsigned long getADCValue(uint8_t aPin);
#endif
	unsigned long getVCC(void);


#ifdef HAS_TRIGGER
	myAVRInterrupt cInterrupt;
#endif
#ifdef HAS_TRIGGER_1
	myAVRInterrupt cInterrupt1;
#endif

public:

	myAVR()
		#ifdef HAS_TRIGGER
		: cInterrupt(TRIGGER_PIN,TRIGGER_DEBOUNCE_TIME, HAS_TRIGGER, TRIGGER_EVENT)
		#endif
		#ifdef HAS_TRIGGER_1
		, cInterrupt1(TRIGGER_1_PIN,TRIGGER_1_DEBOUNCE_TIME, HAS_TRIGGER_1, TRIGGER_1_EVENT)
		#endif
		{ };

	void initialize();
	bool poll();
	void infoPoll(byte prescaler);
	void send(char *cmd, uint8_t typecode=0) REDUCED_FUNCTION_OPTIMIZATION; //needed for Low Power Mode -> https://lowpowerlab.com/forum/index.php/topic,1620.msg11752.html#msg11752
	void displayData(RecvData *DataBuffer);
	void printHelp();

#if HAS_LEDs
	static void activityLed (uint8_t on);
#endif

//	static void tick();

};
#endif
