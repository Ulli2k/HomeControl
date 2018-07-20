
#ifndef _MY_INTERRUPT_
#define _MY_INTERRUPT_


#include <stdint.h>

//https://github.com/GreyGnome/PinChangeInt
#if defined(__AVR_ATmega328P__)
  #ifndef PinChangeInt_h
  // #define NO_PORTB_PINCHANGES // to indicate that port b will not be used for pin change interrupts
   #define NO_PORTC_PINCHANGES // to indicate that port c will not be used for pin change interrupts
   #define NO_PORTD_PINCHANGES // to indicate that port d will not be used for pin change interrupts
   #define NO_PIN_STATE        // to indicate that you don't need the pinState
   #define NO_PIN_NUMBER       // to indicate that you don't need the arduinoPin
   #define DISABLE_PCINT_MULTI_SERVICE
   #define LIBCALL_PINCHANGEINT
   #include <PinChangeInt.h>
  #endif
#else
  #define EXINT_PIN_2_REG_MASK(pin)    ((uint32_t)(1<<(EExt_Interrupts)digitalPinToInterrupt(pin)))
#endif


/* Interrupt Functions */
#define INTZero							2	//Pin of Int0 (Atmega328p)
#define INTOne							3 //Pin of Int1 (Atmega328p)
#define Interrupt_Off				0
#define Interrupt_Block			1
#define Interrupt_Release		2
#define Interrupt_Low				3
#define Interrupt_High			4
#define Interrupt_Falling		5
#define Interrupt_Rising		6
#define Interrupt_Change		7

class myBaseModule;

class myInterrupt {

  // uint8_t _irqState;
public:
  uint8_t _irqPin;
  uint32_t _irqPinMask;

	myInterrupt() { _irqPin=0xFF; _irqPinMask=0xFFFFFFFF; };

  virtual void interrupt() { };

#if defined(__AVR_ATmega328P__)
  static void INT0_interrupt();
  static void INT1_interrupt();
  static void PinChg_interrupt();
  static void PinChg_interrupt1();
  static uint8_t PinChg_irqPin;
#else
  static void _interrupt();
#endif

  void cfgInterrupt(myBaseModule *module, uint8_t irqPin, byte state);
};

#endif
