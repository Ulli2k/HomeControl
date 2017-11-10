#ifndef _MY_ROLLO_MODULE_h
#define _MY_ROLLO_MODULE_h

#include <myBaseModule.h>

#define ROLLO_MAX_MOVING_TIME		210		//[s*10]
#define ROLLO_POWER_OFF_DELAY		200 		//[ms]

class myROLLO : public myBaseModule {

private:

	uint16_t maxMovingTime; //sec*1ß
	
	void SwitchUpDown(const char* cmd);
	static void movingInterrupt();

public:

	static byte waiting4stop;
		
	myROLLO() { };
	
	void initialize();
	bool poll();
	//void infoPoll(byte prescaler);
	void send(char *cmd, uint8_t typecode=0) REDUCED_FUNCTION_OPTIMIZATION; //needed for Low Power Mode -> https://lowpowerlab.com/forum/index.php/topic,1620.msg11752.html#msg11752
	void displayData(RecvData *DataBuffer);
	void printHelp();
	
};
#endif
