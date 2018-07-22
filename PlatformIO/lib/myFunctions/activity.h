#ifndef _MY_ACTIVITY_h
#define _MY_ACTIVITY_h

//TODO: infoPollCylces auf RTC umstellen, Sleep auf InfoPoll umstellen

#include <myBaseModule.h>
#include <led.h>

#if HAS_POWER_OPTIMIZATIONS
	#include <safePower.h>
	#ifndef SAFE_POWER_MAX_IDLETIME
		#define SAFE_POWER_MAX_IDLETIME		50
	#endif
#endif

#if defined(INFO_POLL_CYCLE_TIME) && ((INFO_POLL_CYCLE_TIME > 2040) || (INFO_POLL_CYCLE_TIME < 8)) //uint8_t *8 Sekunden -> 2040 Sek -> 34 Min
	#error Max INFO_POLL_CYCLE_TIME is 2040 due to uint8_t limitation and can not be smaller than 8.
#endif

//################# getAvailableRam #######################
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__
//##########################################################

//################# Reset #######################
//#define PwrDown_AVR() { TIMING::delay(10); cli(); set_sleep_mode(SLEEP_MODE_PWR_DOWN); sleep_mode(); } // in den Schlafmodus wechseln
#if defined(__AVR_ATmega328P__)
	#define Reset_AVR() { wdt_enable(WDTO_15MS); while(1) {} }
#else
	#define Reset_AVR() { NVIC_SystemReset(); }
#endif
//##########################################################

#if defined(LED_ACTIVITY)
template<class LED>
#endif
class Activity :
#if defined(HAS_POWER_OPTIMIZATIONS)
public safePower {
#else
public myBaseModule {
#endif

private:
	#if defined(LED_ACTIVITY)
		LED cLED;
	#endif

  #if HAS_INFO_POLL
  	static unsigned long tInfoPoll; //static default value defined in .cpp
  	#define	INFO_POLL_PRESCALER_MAX		0xFF
  	static byte preScaler;
  #endif

	#if HAS_POWER_OPTIMIZATIONS
		static unsigned long tIdleCycles;
		// static bool isPowerDownPrepared;
	#endif

public:

    Activity() {	}

		const char* getFunctionCharacter() { return "modp"; }

		void initialize() {
			#if HAS_POWER_OPTIMIZATIONS
				PowerOpti_AllPins_OFF;
			#endif
		}

		void trigger() {
			#if defined(LED_ACTIVITY)
			cLED.activityLed(1);
			#endif
			#if HAS_POWER_OPTIMIZATIONS
			idleCycles(1); //is FIFO empty and idle time over?
			#endif
		}

  #if HAS_INFO_POLL && INFO_POLL_CYCLE_TIME
  	static bool infoPollCycles(bool force=false/*, byte *ppreScaler=NULL*/) {
  		const typeModuleInfo* pmt = ModuleTab;
  		if(force || (millis_since(tInfoPoll) >= (unsigned long)(INFO_POLL_CYCLE_TIME*1000UL))) {
   			// if(!ppreScaler) {
  	 			while(pmt->typecode >= 0) {
  					pmt->module->infoPoll(preScaler);
  					pmt++;
  				}
  			// }
  			// *ppreScaler = preScaler;
  			preScaler = (preScaler<INFO_POLL_PRESCALER_MAX ? preScaler+1 : 0);
  			tInfoPoll=millis();
  			return true;
  		}
  		return false;
  	}
  #endif

    bool poll() {
  		bool ret=0;

    #if HAS_INFO_POLL
      ret = infoPollCycles();
    #endif

		#if HAS_POWER_OPTIMIZATIONS
			if((safePower::is_safePower() && Activity::idleCycles(0,SAFE_POWER_MAX_IDLETIME)) /*|| isPowerDownPrepared*/) {
	  		//send((char*)"",MODULE_ACTIVITY_POWERDOWN);
				#if INCLUDE_DEBUG_OUTPUT
	  		if(DEBUG) { /*if(isPowerDownPrepared)*/ { DS_P("awake: ");DU(millis_since(getLastAwakeTime()),0);DS_P("ms\n"); } }
				#endif
	  		// addToRingBuffer(MODULE_DATAPROCESSING, MODULE_ACTIVITY_POWERDOWN, NULL, 0); //execute PowerDown command with RingBuffer because Debug Messages have to be flushed!
				enablePowerDown();
	  		ret = true;
	  	}
			#endif

			return ret;
    }

    void send(char *cmd, uint8_t typecode) {
    	switch(typecode) {
    		case MODULE_ACTIVITY_RAMAVAILABLE:
    			addToRingBuffer(MODULE_ACTIVITY_RAMAVAILABLE, 0, NULL, 0);
    			break;
    		case MODULE_ACTIVITY_REBOOT:
    			Reset_AVR();
    			break;
				#if HAS_POWER_OPTIMIZATIONS
				case MODULE_ACTIVITY_POWERDOWN:
					// isPowerDownPrepared=false;
					safePower::setLowPower();
					safePower::setPowerDownAuto((cmd[0]!='0'));
					break;
				case MODULE_ACTIVITY_LOWPOWER:
					safePower::setLowPower();
					break;
				#endif
    	}
    }

		#if HAS_POWER_OPTIMIZATIONS
		//check if idle time is over and FIFO is empty
		static uint8_t idleCycles(int8_t plus=0, unsigned long timeLimit=0) {
			if(plus==1) {
				tIdleCycles=millis();
				return 0;
			} else {
				if(tIdleCycles==0) tIdleCycles=millis()+1;
				//DS("idle ");Serial.print(millis()-tIdleCycles);DNL();Serial.flush();
				return ( ( (millis_since(tIdleCycles)>=timeLimit) && !FIFO_available(DataRing)) ? 1 : 0);
			}

		}

		void enablePowerDown() REDUCED_FUNCTION_OPTIMIZATION {

			//DS_P("PowerDown\n");

			// if(!isPowerDownPrepared) {
			// 	addToRingBuffer(MODULE_DATAPROCESSING, MODULE_SERIAL); //flush UARTs
			// 	isPowerDownPrepared=true;
			// 	return; //PowerDown after one additional run of poll cycle because Debug Messages have to be flushed!
			// }

			// isPowerDownPrepared=false;
			#if defined(LED_ACTIVITY)
			cLED.LedOnOff(0);
			#endif

			DFL();DFL();

		#ifdef INFO_POLL_CYCLE_TIME
			if(!safePower::setPowerDown( ((INFO_POLL_CYCLE_TIME) ? false : true),INFO_POLL_CYCLE_TIME)) { //return 0 if wake up due to InfoPoll
		#else
			if(!safePower::setPowerDown()) { //sleep forever
		#endif
				//Functions for InfoPoll`s
				#if HAS_INFO_POLL && INFO_POLL_CYCLE_TIME
				infoPollCycles(1); //force InfoPoll Functions
				#endif
			}
			// idleCycles(1); //reset idle cylces
			trigger();
		}

		#endif

    void displayData(RecvData *DataBuffer) {

    	switch(DataBuffer->ModuleID) {
    		case MODULE_ACTIVITY_RAMAVAILABLE:
    			DU(getAvailableRam(),0);
    			break;
    	}
    }

    void printHelp() {
    	DS_P(" * [RAM]     m\n");
    	DS_P(" * [Reboot]  o\n");
			#if HAS_POWER_OPTIMIZATIONS
			// DS_P("\n ## SAFE POWER ##\n");
			DS_P(" * [PwrDown] d<0:off,1:on>\n");
			DS_P(" * [LowPwr]  p\n");
			#endif
    }

    int getAvailableRam() {
    	char top;
    #ifdef __arm__
    	return &top - reinterpret_cast<char*>(sbrk(0));
    #elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
    	return &top - __brkval;
    #else  // __arm__
    	return __brkval ? &top - __brkval : &top - __malloc_heap_start;
    #endif
    }
};

#if HAS_INFO_POLL
	#if defined(LED_ACTIVITY)
		template<class LED>
	  unsigned long   Activity<LED>::tInfoPoll   	= 0xFFFF;	//default value max due to forcing InfoPoll on Startup before e.g. going to sleep.
	  template<class LED>
		byte            Activity<LED>::preScaler		= 0;
	#else
		unsigned long   Activity::tInfoPoll   	= 0xFFFF;	//default value max due to forcing InfoPoll on Startup before e.g. going to sleep.
		byte            Activity::preScaler		= 0;
	#endif
#endif

#if HAS_POWER_OPTIMIZATIONS
	#if defined(LED_ACTIVITY)
		template<class LED>
		unsigned long Activity<LED>::tIdleCycles		=	0;
	#else
		unsigned long Activity::tIdleCycles		=	0;
	#endif
	// template<class LED>
	// bool Activity<LED>::isPowerDownPrepared			=	false;
#endif

#endif
