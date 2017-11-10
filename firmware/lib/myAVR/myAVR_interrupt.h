#ifndef _MY_AVR_INTERRUPT_MODULE_h
#define _MY_AVR_INTERRUPT_MODULE_h

#include <myBaseModule.h>

#define TRIGGER_BUTTON_ID			3

#ifndef TRIGGER_EVENT
	#define TRIGGER_EVENT	MODULE_AVR_TRIGGER
#endif

#ifndef TRIGGER_1_EVENT
	#define TRIGGER_1_EVENT	MODULE_AVR_TRIGGER
#endif

#ifndef TRIGGER_DEBOUNCE_TIME
	#define TRIGGER_DEBOUNCE_TIME		50
#endif

#ifndef TRIGGER_1_DEBOUNCE_TIME
	#define TRIGGER_1_DEBOUNCE_TIME		50
#endif

#if HAS_TRIGGER==TRIGGER_BUTTON_ID || HAS_TRIGGER_1==TRIGGER_BUTTON_ID
	#define INCLUDE_BUTTON				1
#endif

#if HAS_TRIGGER==TRIGGER_BUTTON_ID && HAS_TRIGGER_1==TRIGGER_BUTTON_ID
	#error Just one Button is supported
#endif

#ifdef INCLUDE_BUTTON
	//https://github.com/mathertel/OneButton
	#include "OneButton.h"
	#define TRIGGER_BUTTON_NONE						0
	#define TRIGGER_BUTTON_CLICK					1
	#define TRIGGER_BUTTON_DOUBLE_CLICK		2
	#define TRIGGER_BUTTON_LONG_CLICK			3	
#endif

class myAVRInterrupt : public myBaseModule {

private:
	struct {
		volatile uint8_t irqPin:5; 		//Value-Range: 0-21 (max. A7 in Atmega328p) (SPI & myAVR_interrupt)
		uint8_t event:8;
		bool level:1;					//0:low; 1:high; 2:pulse
		bool pulse:1; 				//send pulse information only (when it goes high) level==2
		bool button:1;				  //send pulse hold information
		bool triggerDetected:1;
		volatile bool triggerLevel:1;
		volatile unsigned long triggerDetectedTime;
		uint16_t debounce;
	} sTrigger;

#ifdef INCLUDE_BUTTON
	OneButton cButton;
	static byte myAVR_ButtonState; //0-3
	static void myAVR_Button_Click() { myAVR_ButtonState = TRIGGER_BUTTON_CLICK; }
	static void myAVR_Button_DoubleClick() { myAVR_ButtonState = TRIGGER_BUTTON_DOUBLE_CLICK; }
	static void myAVR_Button_LongClick() { myAVR_ButtonState = TRIGGER_BUTTON_LONG_CLICK; }
#endif

protected:
	void interrupt();

public:
	myAVRInterrupt(uint8_t irq=INTOne, uint16_t debounce=100, uint8_t level=1, uint8_t event=MODULE_AVR_TRIGGER);
		
	void initialize();

	bool poll();
	//void infoPoll(byte prescaler);
	//void send(char *cmd, uint8_t typecode=0) REDUCED_FUNCTION_OPTIMIZATION; //needed for Low Power Mode -> https://lowpowerlab.com/forum/index.php/topic,1620.msg11752.html#msg11752
	void displayData(RecvData *DataBuffer);
	//void printHelp();

};

#endif
