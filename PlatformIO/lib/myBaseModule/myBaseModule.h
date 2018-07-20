
#ifndef _MY_BASE_MODULE_
#define _MY_BASE_MODULE_

// #include <avr/pgmspace.h> //PSTR() und phm_read_byte
#include <../myFunctions/RingBuffer.h>

#if defined(STORE_CONFIGURATION)
	#include <avr/eeprom.h>
#endif

/*********************** Declare Module Commands ***********************/
#include <myBaseModuleDefines.h>

/*********************** Declare Timing Functions for all Modules ***********************/
//#define TIMING myTiming
#include <myBaseTiming.h>

/*********************** Declare Interrupt Functions for all Modules ***********************/
#include <myBaseInterrupt.h>

/*********************** Power Saving Functions *******************************/
#define REDUCED_FUNCTION_OPTIMIZATION		__attribute__((optimize("-O1")))
#include <TurnOffFunctions.h>

/*********************** Global Helper Functions *******************************/
#define	ULONG_MAX	4294967295UL 	/* max value of "unsigned long int" */

#define HexStr2uint8(x) 		((uint8_t) (HexChar2uint8((x)[0])*16 + HexChar2uint8((x)[1])))
#define HexStr2uint16(x) 		((uint16_t) ( (HexStr2uint8(x) << 8) + (HexStr2uint8(x+2)) ) )
uint8_t HexChar2uint8				(char data);
void 		shiftCharRight			(byte *data, uint8_t len, uint8_t bytes);
void 		shiftCharLeft				(byte *data, uint8_t start, uint8_t len, uint8_t bytes);

/*********************** Declare Display Functions for all Modules ***********************/
// #if INCLUDE_DEBUG_OUTPUT
// 	#define DEBUG						1//(!myBaseModule::QuietMode)
// #else
// 	#define DEBUG						0
// #endif
// #define DEBUG_ONOFF(x) 		1//(myBaseModule::QuietMode = (x))
#include <myBaseDisplay.h>

/*********************** Declare Global Module Funktions/Variables *************/
extern DataFIFO_t DataRing;

class myBaseModule;
typedef struct {
    int8_t typecode;
    const char* name;
    myBaseModule* module;
} typeModuleInfo;
extern const typeModuleInfo ModuleTab[];

// #if HAS_AVR	&& defined(READVCC_CALIBRATION_CONST)
// 	#define VCC_SUPPLY_mV (myBaseModule::VCC_Supply_mV)
// #else
// 	#define VCC_SUPPLY_mV 3300
// #endif

/********************************************************************************/
class myBaseModule : public myTiming, public myInterrupt, public myDisplay {


protected:

#if HAS_POWER_OPTIMIZATIONS
	static unsigned long tIdleCycles; //static default value defined in .cpp
#endif

// #if HAS_INFO_POLL
// 	static unsigned long tInfoPoll; //static default value defined in .cpp
// 	#define	INFO_POLL_PRESCALER_MAX		0xFF
// 	static byte preScaler;
// #endif

public:
	// class BaseFkt { }

// #if HAS_AVR
// 	static unsigned int VCC_Supply_mV;
// #endif

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

#if INCLUDE_HELP
  virtual void printHelp() { };
#endif

// Append a new data item to the outgoing packet buffer (if there is room)
static void addToRingBuffer (byte module, byte code, const byte* buf = NULL, byte len = 0) {

	RecvData rBuf;

	if(len >= MAX_RING_DATA_SIZE) {  //immer 1 zeichen für die Terminierung sicher stellen
		/*if(DEBUG) */ {
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

// #if HAS_POWER_OPTIMIZATIONS
// 	//check if idle time is over and FIFO is empty
// 	static uint8_t idleCycles(int8_t plus=0, unsigned long timeLimit=0) {
// 		if(plus==1) {
// 			tIdleCycles=millis();
// 			return 0;
// 		} else {
// 			if(tIdleCycles==0) tIdleCycles=millis()+1;
// 			//DS("idle ");Serial.print(millis()-tIdleCycles);DNL();Serial.flush();
// 			return ( ( (millis_since(tIdleCycles)>=timeLimit) && !FIFO_available(DataRing)) ? 1 : 0);
// 		}
//
// 	}
// #endif

// #if HAS_INFO_POLL && INFO_POLL_CYCLE_TIME
// 	static bool infoPollCycles(bool force=false, byte *ppreScaler=NULL) {
// 		const typeModuleInfo* pmt = ModuleTab;
// 		if(force || (millis_since(tInfoPoll) >= (unsigned long)(INFO_POLL_CYCLE_TIME*1000UL))) {
//  			if(!ppreScaler) {
// 	 			while(pmt->typecode >= 0) {
// 					pmt->module->infoPoll(preScaler);
// 					pmt++;
// 				}
// 			}
// 			*ppreScaler = preScaler;
// 			preScaler = (preScaler<INFO_POLL_PRESCALER_MAX ? preScaler+1 : 0);
// 			tInfoPoll=millis();
// 			return true;
// 		}
// 		return false;
// 	}
// #endif


#if defined(STORE_CONFIGURATION)
	static void getStoredValue(void *value, const void *p, size_t len) {
		eeprom_read_block(value, p, len);
	}
#else
	#define getStoredValue
#endif

#if defined(STORE_CONFIGURATION)
	static void StoreValue(void *value, void *p, size_t len) {
		eeprom_write_block(value, p, len);
	}
#else
	#define StoreValue
#endif

};

#endif
