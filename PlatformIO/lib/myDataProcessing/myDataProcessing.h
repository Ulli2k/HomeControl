#ifndef _MY_DATAPROCESSING_MODULE_h
#define _MY_DATAPROCESSING_MODULE_h

#include <myBaseModule.h>

extern const typeModuleInfo ModuleTab[];
extern const char welcomeText[] PROGMEM;

class myDataProcessing : public myBaseModule {

private:
#if HAS_RADIO && HAS_RADIO_CMD_TUNNELING==2 //Satellite
	static byte OutputTunnel;
#endif

	byte WelcomeMSG:1;

	// static const char welcomeText[] PROGMEM;
#if INCLUDE_HELP
	void printModuleHelp();
#endif
	inline bool isRightDeviceID(char *cmd);
	inline void callSendFkt(char *cmd);

public:

	myDataProcessing();

	//void initialize();
	bool poll();
	void send(char *cmd, uint8_t typecode=0);
	void printHelp();
	void displayData(RecvData *DataBuffer);
	const char* getFunctionCharacter() { return "qhwW"; };	

	static void printPROGMEM(const char * s); //PGM_P
};

#endif
