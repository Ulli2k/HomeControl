
#ifndef MYPOWERMONITOR_h
#define MYPOWERMONITOR_h

#include "myBaseModule.h"

#if defined(HAS_POWER_MONITOR_VCC_1) || defined(HAS_POWER_MONITOR_VCC_2) || defined(HAS_POWER_MONITOR_CT1_1) || defined(HAS_POWER_MONITOR_CT1_1) || defined(HAS_POWER_MONITOR_CT1_2) || defined(HAS_POWER_MONITOR_CT2_1) || defined(HAS_POWER_MONITOR_CT2_2) || defined(HAS_POWER_MONITOR_CT3_1) || defined(HAS_POWER_MONITOR_CT3_2)
	#include "EmonLibPro_config.h"
	#include "EmonLibPro.h"
#endif

class myPOWERMONITOR : public myBaseModule {

private:
//	uint8_t _ADC;
//	uint8_t _VCC;
#ifdef HAS_POWER_MONITOR_CT
	EmonLibPro _emon;                   	// Create an instance

	uint8_t nextIndex4OutPut;
	bool skipFirstSample:1;									// Skip first result due to garbage
	bool relevantChanges();

  float lastResultV;
  ResultPowerDataStructure    lastResultP[CURRENTCOUNT];
#endif

#ifdef HAS_POWER_MONITOR_PULSE
	volatile bool pulseLevel:1;
	bool pulseDetected:1;
	volatile unsigned long pulseLevelTime;
protected:
	void interrupt();
#endif

public:
	myPOWERMONITOR();
	const char* getFunctionCharacter() { return "P"; };
	
	void initialize();
	bool poll();
	void infoPoll(byte prescaler);
	void send(char *cmd, uint8_t typecode=0);
	void displayData(RecvData *DataBuffer);
	void printHelp();

};

#endif
