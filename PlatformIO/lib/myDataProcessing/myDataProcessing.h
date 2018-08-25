#ifndef _MY_DATAPROCESSING_MODULE_h
#define _MY_DATAPROCESSING_MODULE_h

/******** DEFINE dependencies ******
	HAS_DISPLAY_TUNNELING: adds Output/Display tunneling function
		MODULE_DATAPROCESSING_TUNNELING_MODULE: Define Module for tunneling
			HAS_RADIO: HAS_RADIO_TUNNELING
		TUNNELING_ACTIVE gibt es nicht da dies über z.B. RADIO_TUNNELING_ACTIVE bei der Radio Initialisierung aktiviert wird
	HAS_FORWARD_MESSAGES: adds messaging forward to Satellites
		HAS_RADIO: HAS_RADIO_FORWARDING
	INCLUDE_HELP: adds Module Help function "h"
************************************/

#if (defined(HAS_DISPLAY_TUNNELING) || defined(HAS_FORWARD_MESSAGES)) && defined(HAS_RADIO) && !defined(MODULE_DATAPROCESSING_TUNNELING_MODULE)
	#define MODULE_DATAPROCESSING_TUNNELING_MODULE 				MODULE_RADIO_TUNNELING
	#ifdef HAS_DISPLAY_TUNNELING
		#define HAS_RADIO_TUNNELING
	#endif
	#ifdef HAS_FORWARD_MESSAGES
		#define HAS_RADIO_FORWARDING
	#endif

	#if defined(HAS_DISPLAY_TUNNELING) && defined(HAS_FORWARD_MESSAGES)
		#error Just one Function (HAS_DISPLAY_TUNNELING or HAS_FORWARD_MESSAGES) can be activated!
	#endif
#endif

#include <myBaseModule.h>

extern const typeModuleInfo ModuleTab[];
extern const char welcomeText[] PROGMEM;

class myDataProcessing : public myBaseModule {

private:
#ifdef HAS_DISPLAY_TUNNELING
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
