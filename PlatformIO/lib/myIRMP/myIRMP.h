
#ifndef MYIRMP_h
#define MYIRMP_h

#include "myBaseModule.h"

#include "TimerOne.h"

extern "C" {
//#if HAS_IR_RX
  #include <irmp.h>
//#endif
  // #define F_INTERRUPTS 14925
  /*
      Pin 25 PC2/PCINT10 ---> Port 3 AIO3
   		Arduino Analog 2 / Digital 16
   */
//#if HAS_IR_TX
  #include <irsnd.h>
//#endif
  // #define F_INTERRUPTS 14925
  // #define IRSND_OCx  IRSND_OC0A
  /*
      OC0A --> Pin12 PD6 --> Port 3 DIO3
   		Arduino Digital 6 PWM Timer 0

   		irsend.c Fehler --> OC0A --> PD6 nicht PB6
   */
}


/* F_INTERRUPTS is the interrupt frequency defined in irmpconfig.h */
#define US (long) ((1000000.0 / F_INTERRUPTS) + 0.5)

class myIRMP : public myBaseModule {

public:
	myIRMP() { }

	void initialize();
	bool poll();
	void send(char *cmd, uint8_t typecode=0);
	void sendIR(unsigned int protocol, unsigned long address, unsigned long command, unsigned int flags);
	void displayData(RecvData *DataBuffer);
	void printHelp();
  const char* getFunctionCharacter() { return "I"; };

	uint16_t getReceivedAddress(RecvData *rData);
	uint16_t getReceivedCommand(RecvData *rData);
	uint8_t getReceivedFlag(RecvData *rData);

private:

	static void timerinterrupt();

};

#endif
