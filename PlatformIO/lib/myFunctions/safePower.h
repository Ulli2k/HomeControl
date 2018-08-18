

#ifndef _MY_SAFE_POWER_h
#define _MY_SAFE_POWER_h

//TODO: Timer0 & 1 dependancy with setLowPower

/******** DEFINE dependencies ******
	SAFE_POWER_AUTO_DEFAULT_VALUE: go into sleep after startup

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

#ifndef SAFE_POWER_AUTO_DEFAULT_VALUE
	#define SAFE_POWER_AUTO_DEFAULT_VALUE	false
#endif

class safePower : public myBaseModule {

private:
  static bool safePower_auto;
	#ifndef HAS_RTC
  static uint8_t safePower_Multiplier_Counter;
	#endif
  #if INCLUDE_DEBUG_OUTPUT
	static unsigned long awakeTime;
	#endif

public:
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

	bool setPowerDown(bool forever=true, uint16_t cycleTime=0 /*s*/) REDUCED_FUNCTION_OPTIMIZATION {

		bool INTwakeUp=false; //return 0 if wake up due to infoPoll

		if(forever) {
			FKT_POWERDOWN_FOREVER;
			INTwakeUp=true;

		} else {
			#ifndef HAS_RTC
				for(; (safePower_Multiplier_Counter*8)<cycleTime; safePower_Multiplier_Counter++) { //needed for multiplier of 8s
			#endif
					FKT_USB_OFF;
					FKT_WDT_POWERDOWN(SLEEP_8S); //if sleep time is not forever
					if(!isAwakeReasonWD()) { //wake up due to Interrupt
						INTwakeUp=true;
						#ifndef HAS_RTC
						break;
						#endif
					}
			#ifndef HAS_RTC
				}
				if( isAwakeReasonWD() && (safePower_Multiplier_Counter*8 >= cycleTime)) {
					safePower_Multiplier_Counter=0;
				}
			#endif
			// myTiming::configure(); //reset timing functionality millis,delay...
		}
		#if INCLUDE_DEBUG_OUTPUT
		awakeTime = millis();
		#endif

		return INTwakeUp;
	}

};

bool safePower::safePower_auto = SAFE_POWER_AUTO_DEFAULT_VALUE;
#ifndef HAS_RTC
	uint8_t safePower::safePower_Multiplier_Counter = 0;
#endif
#if INCLUDE_DEBUG_OUTPUT
	unsigned long safePower::awakeTime = 0;
#endif

#endif
