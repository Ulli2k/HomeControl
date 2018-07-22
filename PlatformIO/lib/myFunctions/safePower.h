

#ifndef _MY_SAFE_POWER_h
#define _MY_SAFE_POWER_h

#include <myBaseModule.h>

#if defined (__SAMD21G18A__)
#include <RTCZero.h>
RTCZero rtc;
#endif

#define LIBCALL_TurnOffFunctions	1 //needed to use WDT Interrupt Service Routine
// #include <TurnOffFunktions.h>

#ifndef SAFE_POWER_AUTO_DEFAULT_VALUE
	#define SAFE_POWER_AUTO_DEFAULT_VALUE	false
#endif

class safePower : public myBaseModule {

private:
  static bool safePower_auto;
  static uint8_t safePower_Multiplier_Counter;
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
		PowerOpti_BOD_OFF;
		PowerOpti_ADC0_OFF;
		PowerOpti_AIN_OFF;
		PowerOpti_SPI_OFF;
		PowerOpti_TWI_OFF;
		PowerOpti_WDT_OFF;

		// PowerOpti_AllPins_OFF;
		PowerOpti_USB_OFF;
		// PowerOpti_USART0_OFF
	}

	void setPowerDownAuto(bool on) {
		safePower_auto = on;
	}
	bool setPowerDown(bool forever=true, uint16_t cycleTime=0) REDUCED_FUNCTION_OPTIMIZATION {
		bool INTwakeUp=false; //return 0 if wake up due to infoPoll

		if(forever) {
			FKT_POWERDOWN_FOREVER;
			INTwakeUp=true;
		} else {
			for(; (safePower_Multiplier_Counter*8)<cycleTime; safePower_Multiplier_Counter++) { //needed for multiplier of 8s
				FKT_WDT_POWERDOWN(SLEEP_8S); //if sleep time is not forever
				if(!isAwakeReasonWD()) { //wake up due to Interrupt
					INTwakeUp=true;
					break;
				}
			}
			if( isAwakeReasonWD() && (safePower_Multiplier_Counter*8 >= cycleTime)) {
				safePower_Multiplier_Counter=0;
			}
		}
		#if INCLUDE_DEBUG_OUTPUT
		awakeTime = millis();
		#endif

		return INTwakeUp;
	}

};

bool safePower::safePower_auto = SAFE_POWER_AUTO_DEFAULT_VALUE;
uint8_t safePower::safePower_Multiplier_Counter = 0;
#if INCLUDE_DEBUG_OUTPUT
	unsigned long safePower::awakeTime = 0;
#endif

#endif
