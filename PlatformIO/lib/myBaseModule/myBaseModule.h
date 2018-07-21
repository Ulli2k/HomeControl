
#ifndef _MY_BASE_MODULE_
#define _MY_BASE_MODULE_

// #include <avr/pgmspace.h> //PSTR() und phm_read_byte
#include <../myFunctions/libRingBuffer.h>

/*********************** Declare Module Commands ***********************/
#include <myBaseModuleDefines.h>

/*********************** Declare Timing Functions for all Modules ***********************/
#include <myBaseTiming.h>

/*********************** Declare Interrupt Functions for all Modules ***********************/
#include <myBaseInterrupt.h>

/*********************** Power Saving Functions *******************************/
#define REDUCED_FUNCTION_OPTIMIZATION		__attribute__((optimize("-O1")))
#include <libTurnOffFunctions.h>

/*********************** Global Helper Functions *******************************/
#define	ULONG_MAX	4294967295UL 	/* max value of "unsigned long int" */

#define HexStr2uint8(x) 		((uint8_t) (HexChar2uint8((x)[0])*16 + HexChar2uint8((x)[1])))
#define HexStr2uint16(x) 		((uint16_t) ( (HexStr2uint8(x) << 8) + (HexStr2uint8(x+2)) ) )
uint8_t HexChar2uint8				(char data);
void 		shiftCharRight			(byte *data, uint8_t len, uint8_t bytes);
void 		shiftCharLeft				(byte *data, uint8_t start, uint8_t len, uint8_t bytes);

/*********************** Declare Display Functions for all Modules ***********************/
#include <myBaseDisplay.h>

/*********************** Declare Global Module Funktions/Variables *************/
extern DataFIFO_t DataRing;

class myBaseModule;
typedef struct {
    int8_t typecode;
    myBaseModule* module;
} typeModuleInfo;
extern const typeModuleInfo ModuleTab[];

/********************************************************************************/
class myBaseModule : public myTiming, public myInterrupt, public myDisplay {

protected:

#if HAS_POWER_OPTIMIZATIONS
	static unsigned long tIdleCycles; //static default value defined in .cpp
#endif

public:

  myBaseModule() : myTiming(), myInterrupt(), myDisplay() {
  	FIFO_init(DataRing);
  };
  //~BaseModule() { };

/**************************************************************/
/******************* Basic Module Functions *******************/
virtual void send(char *, uint8_t) { };
virtual bool poll(void) { return 0; };
virtual void infoPoll(byte prescaler) { };
virtual void initialize(void) { };
virtual void displayData(RecvData *) { };
virtual bool validdisplayData(RecvData *) { return 1; };
virtual const char* getFunctionCharacter() { return "-"; };

#if INCLUDE_HELP
virtual void printHelp() { };
#endif

static const char getFunctionChar(const char* funcChar, uint8_t funcIndex) {
	funcIndex = ( (strlen(funcChar) <= funcIndex) ? ( strlen(funcChar) ? (strlen(funcChar)-1) : 0 ) : funcIndex );
	return funcChar[funcIndex];
};

// Append a new data item to the outgoing packet buffer (if there is room)
static void addToRingBuffer (byte module, byte code, const byte* buf = NULL, byte len = 0) {

	RecvData rBuf;

	if(len >= MAX_RING_DATA_SIZE) {  //immer 1 zeichen für die Terminierung sicher stellen
		if(DEBUG) {
			DS_P("Ringbuffer overrun <");
			for(uint8_t c=0; c<=len; c++) { DH2(buf[c]); DC(' '); } DS_P("|");
			DH2(module);DS_P("|");
			DU(code,0);DS_P("|");
			DU(len,0); DS_P(">\n");
		}
		len = MAX_RING_DATA_SIZE-1;
	}
	rBuf.ModuleID = module;
	rBuf.DataTypeCode = code;
	if(buf!=NULL && len!=0)
		memcpy(rBuf.Data,buf,len);
	rBuf.DataSize = len;
	rBuf.Data[len] = 0x0; //Terminierung; +1 ist sicher gestellt durch len check

	DataFIFO_write(DataRing,rBuf);
}

};

#endif
