#ifndef _MY_ACTIVITY_h
#define _MY_ACTIVITY_h

//TODO Offset integrieren um RTC und Timing nach Sleep zu korrigieren

/******** DEFINE dependencies ******
	HAS_POWER_OPTIMIZATIONS: sleep functions
		SAFE_POWER_MAX_IDLETIME: min idle time between two sleep modes
	HAS_INFO_POLL: polls the info function of each module
		INFO_POLL_CYCLE_TIME:	[s] for next infoPoll cycle
	LED_ACTIVITY: LED class for signalling activity
	INCLUDE_DEBUG_OUTPUT: for ZeroRegs Dump Function
************************************/


#include <myBaseModule.h>
#include <led.h>

#if HAS_POWER_OPTIMIZATIONS
	#include <safePower.h>
	#ifndef SAFE_POWER_MAX_IDLETIME
		#define SAFE_POWER_MAX_IDLETIME		50
	#endif
#endif

#ifdef HAS_INFO_POLL
	#if defined(INFO_POLL_CYCLE_TIME)
		// #if ((INFO_POLL_CYCLE_TIME > 2040) || (INFO_POLL_CYCLE_TIME < 8)) //uint8_t *8 Sekunden -> 2040 Sek -> 34 Min
		// 	#error Max INFO_POLL_CYCLE_TIME is 2040 due to uint8_t limitation and can not be smaller than 8.
		// #endif
	#elif not defined(INFO_POLL_CYCLE_TIME)
		#error Please define HAS_INFO_POLL and INFO_POLL_CYCLE_TIME
	#endif
#endif

#if defined(INCLUDE_DEBUG_OUTPUT) && defined (__SAMD21G18A__)
	#include <ZeroRegs.h>
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
	#include <avr/wdt.h>
	#define Reset_AVR() { wdt_enable(WDTO_15MS); while(1) {} }
#else
	#define Reset_AVR() { NVIC_SystemReset(); }
#endif
//##########################################################

#ifdef LED_ACTIVITY
template<class LED>
#endif
class Activity : public myBaseModule
#ifdef HAS_INFO_POLL
, public Alarm
#endif
{

private:
	#if defined(LED_ACTIVITY)
		LED cLED;
	#endif

  #if HAS_INFO_POLL
  	#define	INFO_POLL_PRESCALER_MAX		0xFF
  	static byte preScaler;
  #endif

	#if HAS_POWER_OPTIMIZATIONS
		safePower cSafePower;
		static unsigned long tIdleCycles;
	#endif

public:

#ifdef HAS_INFO_POLL
    Activity() : Alarm(0) { async(false);	}
#endif

		const char* getFunctionCharacter() { return "modpD"; }

		void initialize() {

			#if HAS_POWER_OPTIMIZATIONS
				cSafePower.initialize();
			#endif

			#ifdef HAS_INFO_POLL
				tickLoop = seconds2ticks(INFO_POLL_CYCLE_TIME);
				sysclock.add(*this);

				execInfoPoll(); //run infoPoll on startup
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

  #if HAS_INFO_POLL
		virtual void trigger (__attribute__((unused)) AlarmClock& clock) {
      execInfoPoll();
    }

		// exec infoPoll Function of all Modules
		static void execInfoPoll() {
			const typeModuleInfo* pmt = ModuleTab;

			ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
				while(pmt->typecode >= 0) {
					pmt->module->infoPoll(preScaler);
					pmt++;
				}
				preScaler = (preScaler<INFO_POLL_PRESCALER_MAX ? preScaler+1 : 0);
			}

		}
  #endif

    bool poll() {
  		bool ret=0;

		#if HAS_POWER_OPTIMIZATIONS
			// ret = rtc.runready();

			// Ultra Low Power after idle time
			if((cSafePower.is_safePower() && Activity::idleCycles(0,SAFE_POWER_MAX_IDLETIME))) {
	  		//send((char*)"",MODULE_ACTIVITY_POWERDOWN);
	  		if(DEBUG) { DS_P("awake: ");DU(millis_since(cSafePower.getLastAwakeTime()),0);DS_P("ms\n"); }
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
					cSafePower.setLowPower();
					cSafePower.setPowerDownAuto((cmd[0]!='0'));
					break;
				case MODULE_ACTIVITY_LOWPOWER:
					cSafePower.setLowPower();
					break;
				#endif

				#if defined(INCLUDE_DEBUG_OUTPUT) && defined (__SAMD21G18A__)
				case MODULE_ACTIVITY_DUMP_REGS:
					ZeroRegOptions opts = { PRINT_TO_SERIAL	, false };
					printZeroRegs(opts);
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

			#if defined(LED_ACTIVITY)
			cLED.LedOnOff(0);
			#endif

			DS("Servus..");
			DFL();DFL(); //flash Display Buffer

		// #ifdef INFO_POLL_CYCLE_TIME
		// 	if(!safePower::setPowerDown(false,INFO_POLL_CYCLE_TIME)) { //return 0 if wake up due to InfoPoll
		// 		// execInfoPoll(); //NOT needed: will be called by RTC function
		// 	}
		// #else
		// 	safePower::setPowerDown(); //sleep forever, no InfoPoll
		// #endif

			cSafePower.setPowerDown();
			DS("done\n");
			trigger(); //reset idle cylces and activityLED
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
			#ifdef INCLUDE_DEBUG_OUTPUT
			DS_P(" * [DumpReg] D\n");
			#endif
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
		byte            Activity<LED>::preScaler		= 0;
	#else
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
#endif

#endif
