
/*
avrdude -b 115200 -p m328p -c arduino -P net:beagle:4000 -Uflash:w:HomeControl.cpp.hex  #433MHz
avrdude -b 115200 -p m328p -c arduino -P net:beagle:5000 -Uflash:w:HomeControl.cpp.hex  #868MHz
nc beagle 4001
*/

//TODO: BUZZER is missing! --> pwmPin.h

#if defined(__AVR_ATmega328P__)
  #include <PinChangeInt.h> //notwendig da es nur im BaseModule nicht reicht ?
#endif

/*********************** Include all Modules *****************************/
#include <myBaseModule.h>

//Global Command RingBuffer
DataFIFO_t DataRing;

#include <myDataProcessing.h>
myDataProcessing DataProc;

#if HAS_LEDs
  #include <led.h>
  typedef LED<LED1_PIN> LEDType;
  LEDType cLED;
#endif

#include <activity.h>
typedef Activity<LEDType> ActivityType;
ActivityType activity;

#if HAS_UART
  #include <myUart.h>
  myUart Remote(SER_BAUD, HW_UART);
#endif

#if HAS_RADIO
  #include <myRadio.h>
  typedef LibSPI<10 /* SS */> SPI1;
  typedef LibRadio<1, SPI1, 6 /* ResetPin */> LibRadio1;
  typedef myRadio<LibRadio1, 7/*Interrupt*/> RadioType1;
  // typedef LibSPI<SS /* SS */> SPI1;
  // typedef LibRadio<1, SPI1, A0 /* ResetPin */> LibRadio1;
  // typedef myRadio<LibRadio1, 2/*Interrupt*/> RadioType1;
  RadioType1 cRadio;
#endif

#if HAS_DIGITAL_PIN
  #include <digitalPin.h>
  // typedef digitalPin<Relay_SetReset,8,9> SwitchType;
  typedef digitalPin<PinMode_ALL_EVENTS,9> DigitalPinType;
  DigitalPinType digPin;
#endif

#if HAS_ANALOG_PIN
  #include <analogPin.h>
  typedef analogPin<A1> AnalogPinType;
  AnalogPinType anaPin;
  // typedef analogPin<1> Analog2PinType;
  // Analog2PinType ana2Pin;
#endif
// #if HAS_ROLLO
// 	#include <myRollo.h>
// 	myROLLO myRollo;
// #endif

// #if HAS_IR_TX || HAS_IR_RX
//   #include <myIRMP.h>
//   myIRMP myIrmp;
// #endif

// #if HAS_BME280
//   #include <Wire.h>
//   #include <myBME280.h>
//   myBME280 myBME;
// #endif
//
// #if HAS_POWER_MONITOR_CT || HAS_POWER_MONITOR_PULSE
// 	#include <myPowerMonitor.h>
// 	myPOWERMONITOR myPowerMonitor;
// #endif

/*********************** Modules Table *****************************/

const typeModuleInfo ModuleTab[] = {
  { MODULE_DATAPROCESSING   ,"qhwW"    			, &DataProc }, //immer am Anfang

  { MODULE_ACTIVITY         ,"modp"           , &activity },

#if HAS_UART
  { MODULE_SERIAL           ,""       			, &Remote   },
#endif

#if HAS_DIGITAL_PIN
  { MODULE_DIGITAL_PIN      ,"SSN"           , &digPin     },
#endif

#if HAS_ANALOG_PIN
  { MODULE_ANALOG_PIN      ,"AC"           , &anaPin     },
  // { MODULE_ANALOG_PIN      ,"AC"           , &ana2Pin     },
#endif

#if HAS_LEDs
  { MODULE_LED              , "la"          , &cLED    },
#endif

#if HAS_ROLLO
  { MODULE_ROLLO              ,"J"   				 , &myRollo  },
#endif

#if HAS_IR_TX || HAS_IR_RX
  { MODULE_IRMP             ,"I"     				, &myIrmp   },
#endif

#if HAS_BME280
  { MODULE_BME280           ,"E"     			 , &myBME   },
#endif

#if HAS_POWER_MONITOR_CT || HAS_POWER_MONITOR_PULSE
	{ MODULE_POWERMONITOR			,"P"					, &myPowerMonitor	},
#endif

#if HAS_RADIO
// RADIO immer am Schluss für Tunneling
  { MODULE_RADIO            ,"RfT"             , &cRadio    },
#endif

  { -1, "", NULL }
};

/*********************** MAIN Function *****************************/
void setup() {

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
