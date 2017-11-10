#ifndef _MY_DATAPROCESSING_MODULE_h
#define _MY_DATAPROCESSING_MODULE_h

#include <myBaseModule.h>

extern const typeModuleInfo ModuleTab[];

class myDataProcessing : public myBaseModule {
	
private:
#if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING==2 //Satellite
	static byte OutputTunnel;
#endif
	
	byte WelcomeMSG;
	
	static const char welcomeText[] PROGMEM;
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

	static void printPROGMEM(const char * s); //PGM_P
};

#endif
