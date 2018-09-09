
/*
avrdude -b 115200 -p m328p -c arduino -P net:beagle:4000 -Uflash:w:HomeControl.cpp.hex  #433MHz
avrdude -b 115200 -p m328p -c arduino -P net:beagle:5000 -Uflash:w:HomeControl.cpp.hex  #868MHz
nc beagle 4001

Arduino Libraries: ./.platformio/packages/framework-arduinosam/cores/samd/
*/
//TODO: BUZZER is missing! --> pwmPin.h

#if defined(__AVR_ATmega328P__)
  #include <PinChangeInt.h> //notwendig da es nur im BaseModule nicht reicht ?
#endif

/*********************** Include all Modules *****************************/
#include <myBaseModule.h>

//Global Command RingBuffer
#include <libRingBuffer.h>
DataFIFO_t DataRing;

#include <myDataProcessing.h>
myDataProcessing DataProc;

#ifdef HAS_LEDs
  #include <led.h>
  typedef LED<LED1_PIN> LEDType;
  LEDType cLED;
#endif

#include <activity.h>
#ifdef LED_ACTIVITY
  typedef Activity<LEDType> ActivityType;
#else
  typedef Activity ActivityType;
#endif
ActivityType activity;

#ifdef HAS_UART
  #include <myUart.h>
  myUart Remote(SER_BAUD);
#endif

#ifdef HAS_RADIO
  #include <myRadio.h>
  #if defined (__SAMD21G18A__)
  typedef LibSPI<10 /* SS */> SPI1;
  typedef LibRadio<1, SPI1, 5 /* ResetPin */> LibRadio1;
  typedef myRadio<LibRadio1, 7/*Interrupt*/> RadioType1;
  #else
  typedef LibSPI<SS /* SS */> SPI1;
  typedef LibRadio<1, SPI1, A0 /* ResetPin */> LibRadio1;
  typedef myRadio<LibRadio1, 2/*Interrupt*/> RadioType1;
  #endif
  RadioType1 cRadio;
#endif

#ifdef HAS_DIGITAL_PIN
  #include <digitalPin.h>
  // typedef digitalPin<Relay_SetReset,8,9> SwitchType;
  typedef digitalPin<PinMode_ALL_EVENTS,9, true> DigitalPinType;
  DigitalPinType digPin;
#endif

#ifdef HAS_ANALOG_PIN
  #include <analogPin.h>
  typedef analogPin<A1> AnalogPinType;
  AnalogPinType anaPin;
  // typedef analogPin<1> Analog2PinType;
  // Analog2PinType ana2Pin;
#endif

#ifdef HAS_PWM_PIN
  #include <pwmPin.h>
  typedef pwmPin<5, 2750, 50> PwmPinType;
  PwmPinType pPin;
#endif
// #ifdef HAS_ROLLO
// 	#include <myRollo.h>
// 	myROLLO myRollo;
// #endif

// #ifdef HAS_IR_TX || HAS_IR_RX
//   #include <myIRMP.h>
//   myIRMP myIrmp;
// #endif

#ifdef HAS_BME280
  #include <myBME280.h>
  typedef libI2C<0x76> I2CType;
  typedef myBME280<I2CType> BMEType;
  BMEType myBME;
#endif

// #if HAS_POWER_MONITOR_CT || HAS_POWER_MONITOR_PULSE
// 	#include <myPowerMonitor.h>
// 	myPOWERMONITOR myPowerMonitor;
// #endif

/*********************** Modules Table *****************************/
const typeModuleInfo ModuleTab[] = {
  //
  { MODULE_DATAPROCESSING   , &DataProc }, //immer am Anfang
  { MODULE_ACTIVITY         , &activity }, //immer am Anfang wegen "PowerOpti_AllPins_OFF"

#ifdef HAS_UART
  { MODULE_SERIAL      			, &Remote   },
#endif

#ifdef HAS_DIGITAL_PIN
  { MODULE_DIGITAL_PIN      , &digPin     },
#endif

#ifdef HAS_ANALOG_PIN
  { MODULE_ANALOG_PIN       , &anaPin     },
  // { MODULE_ANALOG_PIN       , &ana2Pin     },
#endif

#ifdef HAS_PWM_PIN
  { MODULE_PWM_PIN          , &pPin     },
#endif

#ifdef HAS_LEDs
  { MODULE_LED              , &cLED    },
#endif

// #ifdef HAS_ROLLO
//   { MODULE_ROLLO        , &myRollo  },
// #endif

// #if HAS_IR_TX || HAS_IR_RX
//   { MODULE_IRMP         , &myIrmp   },
// #endif

#ifdef HAS_BME280
  { MODULE_BME280       , &myBME   },
#endif

// #if HAS_POWER_MONITOR_CT || HAS_POWER_MONITOR_PULSE
// 	{ MODULE_POWERMONITOR	 , &myPowerMonitor	},
// #endif

#ifdef HAS_RADIO
// RADIO immer am Schluss für Tunneling
  { MODULE_RADIO            , &cRadio    },
#endif

  { -1, NULL }
};

/*********************** MAIN Function *****************************/
void setup() {

  //Basic Setup
  FIFO_init(DataRing);
  sysclock.initialize(); // initialize the system timer

  //First of all initialize everything
  const typeModuleInfo* pmt = ModuleTab;
  while(pmt->typecode >= 0) {
    pmt->module->initialize();
    pmt++;
  }
}

void loop() {
  const typeModuleInfo* pmt = ModuleTab;
  while(pmt->typecode >= 0) {
		if(pmt->module->poll()) {
      activity.trigger();
    }
    pmt++;
  }
}
