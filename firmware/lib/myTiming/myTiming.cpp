
#include "myTiming.h"

#define TIMER2_PRESCALER	64 //32,64  <- Ist bei 8Mhz und 16Mhz nur bei 64 korrekt

// the prescaler is set so that timer2 ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.
#define MICROSECONDS_PER_TIMER2_OVERFLOW (clockCyclesToMicroseconds(TIMER2_PRESCALER * 256))
//#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
//#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )

// the whole number of milliseconds per timer2 overflow
#define MILLIS_TIMER2_INC (MICROSECONDS_PER_TIMER2_OVERFLOW / 1000)

// the fractional number of milliseconds per timer2 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz - this doesn't lose precision.)
#define FRACT_TIMER2_INC ((MICROSECONDS_PER_TIMER2_OVERFLOW % 1000) >> 3)
#define FRACT_TIMER2_MAX (1000 >> 3)

volatile unsigned long timer2_overflow_count = 0;
volatile unsigned long timer2_millis = 0;
static unsigned char timer2_fract = 0;

static uint8_t cTimeCallbacks=0;
sTimeCallbacks TimeCallbacks[MAX_TIME_CALLBACKS];

#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
ISR(TIM2_OVF_vect)
#else
ISR(TIMER2_OVF_vect)
#endif
{
	// copy these to local variables so they can be stored in registers
	// (volatile variables must be read from memory on every access)
	unsigned long m = timer2_millis;
	unsigned char f = timer2_fract;

	m += MILLIS_TIMER2_INC;
	f += FRACT_TIMER2_INC;
	if (f >= FRACT_TIMER2_MAX) {
		f -= FRACT_TIMER2_MAX;
		m += 1;
	}

	timer2_fract = f;
	timer2_millis = m;
	timer2_overflow_count++;

	for(uint8_t c=0; c<cTimeCallbacks; c++) {
		if(timer2_millis > (TimeCallbacks[c].created + TimeCallbacks[c].ms) ) {
			TimeCallbacks[c].created=timer2_millis;
			TimeCallbacks[c].isrCallback();
		}
	}
}

unsigned long TIMING::millis() {
	unsigned long m;
	uint8_t oldSREG = SREG;

	// disable interrupts while we read timer2_millis or we might get an
	// inconsistent value (e.g. in the middle of a write to timer2_millis)
	cli();
	m = timer2_millis;
	//sei();
	SREG = oldSREG;
	

	return m;
}

unsigned long TIMING::micros() {
	unsigned long m;
	uint8_t oldSREG = SREG, t;
	
	cli();
	m = timer2_overflow_count;
#if defined(TCNT2)
	t = TCNT2;
#elif defined(TCNT2L)
	t = TCNT2L;	
#else
	#error TIMER 2 not defined
#endif
  
#ifdef TIFR2
	if ((TIFR2 & _BV(TOV2)) && (t < 255))
		m++;
#else
	if ((TIFR & _BV(TOV2)) && (t < 255))
		m++;
#endif

	SREG = oldSREG;
	
	return ((m << 8) + t) * (TIMER2_PRESCALER / clockCyclesPerMicrosecond());
}

void TIMING::delay(unsigned long ms) {
	uint16_t start = (uint16_t)TIMING::micros();

	while (ms > 0) {
		if (((uint16_t)TIMING::micros() - start) >= 1000) {
			ms--;
			start += 1000;
		}
	}
}

/* Delay for the given number of microseconds.  Assumes a 8 or 16 MHz clock. */

void TIMING::delayMicroseconds(unsigned int us) {
	// calling avrlib's delay_us() function with low values (e.g. 1 or
	// 2 microseconds) gives delays longer than desired.
	//delay_us(us);
#if F_CPU >= 20000000L
	// for the 20 MHz clock on rare Arduino boards

	// for a one-microsecond delay, simply wait 2 cycle and return. The overhead
	// of the function call yields a delay of exactly a one microsecond.
	__asm__ __volatile__ (
		"nop" "\n\t"
		"nop"); //just waiting 2 cycle
	if (--us == 0)
		return;

	// the following loop takes a 1/5 of a microsecond (4 cycles)
	// per iteration, so execute it five times for each microsecond of
	// delay requested.
	us = (us<<2) + us; // x5 us

	// account for the time taken in the preceeding commands.
	us -= 2;

#elif F_CPU >= 16000000L
	// for the 16 MHz clock on most Arduino boards

	// for a one-microsecond delay, simply return.  the overhead
	// of the function call yields a delay of approximately 1 1/8 us.
	if (--us == 0)
		return;

	// the following loop takes a quarter of a microsecond (4 cycles)
	// per iteration, so execute it four times for each microsecond of
	// delay requested.
	us <<= 2;

	// account for the time taken in the preceeding commands.
	us -= 2;
#else
	// for the 8 MHz internal clock on the ATmega168

	// for a one- or two-microsecond delay, simply return.  the overhead of
	// the function calls takes more than two microseconds.  can't just
	// subtract two, since us is unsigned; we'd overflow.
	if (--us == 0)
		return;
	if (--us == 0)
		return;

	// the following loop takes half of a microsecond (4 cycles)
	// per iteration, so execute it twice for each microsecond of
	// delay requested.
	us <<= 1;
    
	// partially compensate for the time taken by the preceeding commands.
	// we can't subtract any more than this or we'd overflow w/ small delays.
	us--;
#endif

	// busy wait
	__asm__ __volatile__ (
		"1: sbiw %0,1" "\n\t" // 2 cycles
		"brne 1b" : "=w" (us) : "0" (us) // 2 cycles
	);
}

void TIMING::configure() {

	// this needs to be called before setup() or some functions won't
	// work there
	sei();
	
	// Setup Timer 2 for counting microseconds
#if defined(TCCR2A) && defined(WGM21) //fast PWM
	sbi(TCCR2A, WGM21);
	sbi(TCCR2A, WGM20);
#endif  

	// set timer 2 prescale factor to 64
#if defined(TCCR2B) && defined(CS22)
	#if TIMER2_PRESCALER==64
		sbi(TCCR2B, CS22);
	#elif TIMER2_PRESCALER==32
		sbi(TCCR2B, CS20);
		sbi(TCCR2B, CS21);
	#else
		#warning Timer 2 prescale factor not possible
	#endif
#else
	#warning Timer 2 prescale factor 64 not set correctly
#endif

	// enable timer 2 overflow interrupt
#if defined(TIMSK2) && defined(TOIE2)
	sbi(TIMSK2, TOIE2); //Ulli
#else
	#error	Timer 2 overflow interrupt not set correctly
#endif
}

void TIMING::initialize() {

	configure();
	cTimeCallbacks=0;
}

void TIMING::attachInterrupt(void (*isr)(), unsigned long milliseconds)
{
	TimeCallbacks[cTimeCallbacks].isrCallback = isr;
	TimeCallbacks[cTimeCallbacks].ms = milliseconds;
	TimeCallbacks[cTimeCallbacks].created = TIMING::millis();
	cTimeCallbacks++;
}

void TIMING::detachInterrupt(void (*isr)())
{
	for(uint8_t i=0; i<cTimeCallbacks; i++) {
		if(TimeCallbacks[i].isrCallback == isr) {
			for(uint8_t r=i+1; r<cTimeCallbacks; r++) {
				TimeCallbacks[i].isrCallback = TimeCallbacks[r].isrCallback;
				TimeCallbacks[i].ms = TimeCallbacks[r].ms;
				TimeCallbacks[i].created = TimeCallbacks[r].created;
			}
			cTimeCallbacks--;
			break;
		}
	}
}

