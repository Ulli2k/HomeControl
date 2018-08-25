

#ifndef _MY_SAFE_POWER_h
#define _MY_SAFE_POWER_h

//TODO: Timer0 & 1 dependancy with setLowPower
//TODO: UART OFF to safe Power
//TODO: ATMEGA328 --> ohne RTC -> max sleep 8S danach IDLE Time abwarten und wieder 8S Sleep
//TODO: DeepSleep
				//WDT (6µA) maximal 2 Sekunden sleep, dafür in ms Raster  (https://github.com/adafruit/Adafruit_SleepyDog/blob/master/examples/Sleep/Sleep.ino)
				//RTC (7µA) lange sleep Zeiten, dafür im sec Raster


/******** DEFINE dependencies ******
	SAFE_POWER_AUTO_ACTIVE: go into sleep after startup

	HAS_RTC: in case of not availability of RTC, sleep mode will be looped for intervalls
	INCLUDE_DEBUG_OUTPUT_ prints awake time
	HAS_IR_TX & HAS_IR_RX: do not turn of Timer0 & 1
************************************/

#include <myBaseModule.h>
#include <AlarmClock.h>

#ifdef __AVR_ATmega328P__
	ISR (WDT_vect) {
		// WDIE & WDIF is cleared in hardware upon entering this ISR
		wdt_disable();
	}
#endif

#define SAFE_POWER_MIN_SLEEP_TIME_MS		15	//[ms] prevents safePower from Sleep

class safePower : public myBaseModule, public Alarm {

private:
  static bool safePower_auto;
	// #ifndef HAS_RTC
  // static uint8_t safePower_Multiplier_Counter;
	// #endif
  #if INCLUDE_DEBUG_OUTPUT
	static unsigned long awakeTime;
	#endif

public:
	safePower() : Alarm(0) { }

	void initialize() {
		PowerOpti_AllPins_OFF;
		#ifdef HAS_RTC
		rtc.initialize(); // geht nicht über Classen  konstruktor
		#endif
	}

	virtual void trigger (__attribute__((unused)) AlarmClock& clock) {}

	bool is_safePower() {
		return safePower_auto;
	}
	#if INCLUDE_DEBUG_OUTPUT
	unsigned long getLastAwakeTime() {
		return awakeTime;
	}
	#endif
	bool isAwakeReasonWD() {
		return FKT_WDT_WAKEUP_EVENT;
	}
	void setLowPower() REDUCED_FUNCTION_OPTIMIZATION { //Shutsoff not needed Functions
		//CPU MHz vs Voltage: (--> 8MHz min.2.4Volt)
		//From 1.8V to 2.7V: M = 4 + ((V - 1.8) / 0.15)
		//From 2.7V to 4.5V: M = 10 + ((V - 2.7) / 0.18)
		//http://www.rocketscream.com/blog/2011/07/04/lightweight-low-power-arduino-library/
		/*
		* Brown Out Detector	- Bei Unterspannung spielt der AVR ggf. verrückt. Detector schaltet vorher ab. - derzeit nicht notwendig
		* ADC								- Batteriespannung + VCC + IR-RX
		* SPI								- RFM Modul Kommunikation
		* TWI								- Two wire interface - nie notwendig
		* UART								- FHEM Interface (Arduino interrupt gesteuert)
		* Timer0							- Für TX Infrarot Funktion
		* Timer1							- Für RX & TX Infrarot poll Funktion
		* Timer2							- Für Zeitmessung notwendig. z.B. millis() (löst ständig interrupts aus!)
		*/
		#if !HAS_IR_TX && !HAS_IR_RX
		PowerOpti_TIMER1_OFF;
		PowerOpti_TIMER0_OFF;
		#endif
		#if !HAS_IR_TX
		PowerOpti_TIMER0_OFF;
		#endif
		PowerOpti_TIMER_OFF;
		PowerOpti_BOD_OFF;
		// PowerOpti_ADC0_OFF;

		#ifndef HAS_ANALOG_PIN
		PowerOpti_ADC_OFF;
		#endif
		PowerOpti_AIN_OFF;
		PowerOpti_SPI_OFF;
		PowerOpti_TWI_OFF;
		PowerOpti_WDT_OFF;

		// PowerOpti_AllPins_OFF;
		PowerOpti_USB_OFF;
		// PowerOpti_USART0_OFF;
		FKT_SERCOM_OFF;
		PowerOpti_DAC_OFF;
		PowerOpti_DSU_OFF;
		PowerOpti_DMA_OFF;
		// PowerOpti_RTC_OFF;
		#ifndef HAS_DIGITAL_PIN
		PowerOpti_EIC_OFF;
		#endif
	}

	void setPowerDownAuto(bool on) {
		safePower_auto = on;
	}

	void setPowerDown() {

	    sysclock.disable();
	    uint32_t ticks = sysclock.next();
	    if( (sysclock.isready() == false && ticks >= millis2ticks(SAFE_POWER_MIN_SLEEP_TIME_MS)) || ticks == 0) {
	        // hal.radio.setIdle();
	        uint32_t offset = doSleep(ticks); //return ticks
	        sysclock.correct(offset);
	        sysclock.enable();
					if(DEBUG) { DS("sysclock cor: ");DU(offset,0);DNL(); }
	    } else {
	      sysclock.enable();
	    }

			#if INCLUDE_DEBUG_OUTPUT
			awakeTime = millis();
			#endif
	}

	// bool setPowerDown(bool forever=true, uint16_t cycleTime=0 /*s*/) REDUCED_FUNCTION_OPTIMIZATION {
	//
	// 	bool INTwakeUp=false; //return 0 if wake up due to infoPoll
	//
	// 	if(forever) {
	// 		FKT_POWERDOWN_FOREVER;
	// 		INTwakeUp=true;
	//
	// 	} else {
	// 		#ifndef HAS_RTC
	// 			for(; (safePower_Multiplier_Counter*8)<cycleTime; safePower_Multiplier_Counter++) { //needed for multiplier of 8s
	// 		#endif
	// 				FKT_USB_OFF;
	// 				FKT_WDT_POWERDOWN(SLEEP_8S); //if sleep time is not forever
	// 				if(!isAwakeReasonWD()) { //wake up due to Interrupt
	// 					INTwakeUp=true;
	// 					#ifndef HAS_RTC
	// 					break;
	// 					#endif
	// 				}
	// 		#ifndef HAS_RTC
	// 			}
	// 			if( isAwakeReasonWD() && (safePower_Multiplier_Counter*8 >= cycleTime)) {
	// 				safePower_Multiplier_Counter=0;
	// 			}
	// 		#endif
	// 		// myTiming::configure(); //reset timing functionality millis,delay...
	// 	}
	// 	#if INCLUDE_DEBUG_OUTPUT
	// 	awakeTime = millis();
	// 	#endif
	//
	// 	return INTwakeUp;
	// }

	uint32_t doSleep (uint32_t ticks) REDUCED_FUNCTION_OPTIMIZATION { //return ticks
    uint32_t offset = 0;

		#ifdef HAS_RTC
			tick = ticks;
			if(tick) rtc.setAlarm(tick);
			// rtc.add(*this);

			uint32_t c1 = rtc.getCounter(true);
			FKT_WDT_POWERDOWN();
	    uint32_t c2 = rtc.getCounter(false);
	 		offset = (c2 - c1);

		#else
			period_t sleeptime = SLEEP_FOREVER;
			if( ticks > seconds2ticks(8) ) 			 { offset = seconds2ticks(8); sleeptime = SLEEP_8S; }
			else if( ticks > seconds2ticks(4) )  { offset = seconds2ticks(4);  sleeptime = SLEEP_4S; }
			else if( ticks > seconds2ticks(2) )  { offset = seconds2ticks(2);  sleeptime = SLEEP_2S; }
			else if( ticks > seconds2ticks(1) )  { offset = seconds2ticks(1);  sleeptime = SLEEP_1S; }
			else if( ticks > millis2ticks(500) ) { offset = millis2ticks(500); sleeptime = SLEEP_500MS; }
			else if( ticks > millis2ticks(250) ) { offset = millis2ticks(250); sleeptime = SLEEP_250MS; }
			else if( ticks > millis2ticks(120) ) { offset = millis2ticks(120); sleeptime = SLEEP_120MS; }
			else if( ticks > millis2ticks(60)  ) { offset = millis2ticks(60);  sleeptime = SLEEP_60MS; }
			else if( ticks > millis2ticks(30)  ) { offset = millis2ticks(30);  sleeptime = SLEEP_30MS; }
			else if( ticks > millis2ticks(15)  ) { offset = millis2ticks(15);  sleeptime = SLEEP_15MS; }

			FKT_WDT_POWERDOWN(sleeptime);
		#endif

    return min(ticks,offset);;
	}

};

#ifdef SAFE_POWER_AUTO_ACTIVE
bool safePower::safePower_auto = true;
#else
bool safePower::safePower_auto = false;
#endif
// #ifndef HAS_RTC
// 	uint8_t safePower::safePower_Multiplier_Counter = 0;
// #endif
#if INCLUDE_DEBUG_OUTPUT
	unsigned long safePower::awakeTime = 0;
#endif

#endif
