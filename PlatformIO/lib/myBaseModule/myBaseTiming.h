#ifndef _MY_TIMING_
#define _MY_TIMING_

#if defined(__AVR_ATmega328P__)

	#include <Arduino.h> //uint8_t
	#include <stdint.h>

	#ifndef sbi
		#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
	#endif

	#define MAX_TIME_CALLBACKS		2

	typedef struct {
		void (*isrCallback)();
		unsigned long ms;
		unsigned long init_ms;
		//unsigned long created;
	} sTimeCallbacks;

	class myTiming {

	protected:

	public:
	  myTiming() { initialize(); };

	  static void initialize();
		static void configure();
		static void attachTimeInterrupt(void (*isr)(), unsigned long milliseconds);
		static void detachTimeInterrupt(void (*isr)());
		static void delayMicroseconds(unsigned int us);
		static void delay(unsigned long ms);
		static unsigned long micros();
		static unsigned long millis();
		static unsigned long millis_since(unsigned long start_ms);
	};

#else /* STM32 */

	class myTiming {

	protected:

	public:
	  myTiming() { };

	  // static void initialize();
		// static void configure();
		static void attachTimeInterrupt(void (*isr)(), unsigned long milliseconds) { };
		static void detachTimeInterrupt(void (*isr)()) { };
		// static void delayMicroseconds(unsigned int us);
		// static void delay(unsigned long ms);
		static unsigned long micros() { return ::micros();}
		static unsigned long millis() { return ::millis();}
		static unsigned long millis_since(unsigned long start_ms);
	};
#endif


#endif
