
#include <myBaseModule.h>

myBaseModule *INT0Module = NULL;
myBaseModule *INT1Module = NULL;
myBaseModule *PinChgModule = NULL;
myBaseModule *PinChgModule1 = NULL;

#ifndef QUIETMODE_DEFAULT_VALUE
	#define QUIETMODE_DEFAULT_VALUE		1
#endif
byte myBaseModule::QuietMode	=	QUIETMODE_DEFAULT_VALUE;
char myBaseModule::lastPrintChar = '\0';

#if HAS_POWER_OPTIMIZATIONS	
unsigned long myBaseModule::tIdleCycles=0;
#endif

#if HAS_INFO_POLL
unsigned long myBaseModule::tInfoPoll = 0xFFFF;	//default value max due to forcing InfoPoll on Startup before e.g. going to sleep.
byte myBaseModule::preScaler					= 0;
#endif

#if HAS_AVR	
	unsigned int myBaseModule::VCC_Supply_mV=3300;
#endif

#if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING==2 //Satellite
byte myBaseModule::DisplayCopy=0;
char myBaseModule::cDisplayCopy[MAX_RING_DATA_SIZE] = {0};
#endif
//bool myBaseModule::UartActive = 1;

void myBaseModule::INT1_interrupt() {
	if(INT1Module!=NULL)
			INT1Module->interrupt();
}

void myBaseModule::INT0_interrupt() {
	if(INT0Module!=NULL)
			INT0Module->interrupt();
}

void myBaseModule::PinChg_interrupt() {
	if(PinChgModule!=NULL)
			PinChgModule->interrupt();
}

void myBaseModule::PinChg_interrupt1() {
	if(PinChgModule1!=NULL)
			PinChgModule1->interrupt();
}

/******************************* Global Helper Functions ******************************/
uint8_t HexChar2uint8(char data) {
  data -= '0';
  if(data>42) data-= 39;  		//a-f
  else if (data>9) data-= 7;	//A-F
  return data;
}

void shiftCharRight(byte *data, uint8_t len, uint8_t bytes) {
	for(uint8_t i=len; i>0 ;i--) {
		data[i+bytes-1] = data[i-1];
	}
}

void shiftCharLeft(byte *data, uint8_t start, uint8_t len, uint8_t bytes) {
	for(uint8_t i=start; i<=start+len ;i++) {
		data[i-bytes] = data[i];
	}
}
