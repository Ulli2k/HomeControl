
#ifndef _MY_TIMING_MODULE
#define _MY_TIMING_MODULE

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
	
class TIMING {

private:

public:
	static void initialize();
	static void configure();
	static void attachInterrupt(void (*isr)(), unsigned long milliseconds);
	static void detachInterrupt(void (*isr)());
	static void delayMicroseconds(unsigned int us);
	static void delay(unsigned long ms);
	static unsigned long micros();
	static unsigned long millis();
	static unsigned long millis_since(unsigned long start_ms);
};

#endif
