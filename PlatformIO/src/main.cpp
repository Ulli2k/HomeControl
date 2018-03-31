
/*
avrdude -b 115200 -p m328p -c arduino -P net:beagle:4000 -Uflash:w:HomeControl.cpp.hex  #433MHz
avrdude -b 115200 -p m328p -c arduino -P net:beagle:5000 -Uflash:w:HomeControl.cpp.hex  #868MHz
nc beagle 4001
*/

#if defined(__AVR_ATmega328P__)
//   #include <myTiming.h> //notwendig da es nur im BaseModule nicht reicht ?
  #include <PinChangeInt.h> //notwendig da es nur im BaseModule nicht reicht ?
#endif

/*********************** Include all Modules *****************************/
#include <myBaseModule.h>

#include <RingBuffer.h>
DataFIFO_t DataRing;

#include <myDataProcessing.h>
myDataProcessing DataProc;

#if HAS_UART
  #include <myUart.h>
  myUart Remote(SER_BAUD, HW_UART);
#endif

#if HAS_AVR && defined(__AVR_ATmega328P__)
  #include <myAVR.h>
  myAVR myAvr;
#endif

#if HAS_ROLLO
	#include <myRollo.h>
	myROLLO myRollo;
#endif

#if HAS_IR_TX || HAS_IR_RX
  #include <myIRMP.h>
  myIRMP myIrmp;
#endif

#if HAS_BME280
  #include <Wire.h>
  #include <myBME280.h>
  myBME280 myBME;
#endif

#if HAS_POWER_MONITOR_CT || HAS_POWER_MONITOR_PULSE
	#include <myPowerMonitor.h>
	myPOWERMONITOR myPowerMonitor;
#endif


#if HAS_SPI && HAS_RFM69
  #include <mySPI.h>
  mySPI mySpi;
  //PROGMEM http://www.arduino.cc/en/Reference/PROGMEM
  //http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=38003
  const typeSPItable SPITab[] = {
  //{ ID              , SS           , SPI-IRQ, IRQ_InterruptType }
    { SPI_CONFIG_ID_1 , 10 /*PB2*/, INTZero /*PD2:INT0*/	, Interrupt_Rising },
	#if HAS_RFM69>=2
    { SPI_CONFIG_ID_2 , 4	 /*PD4*/, INTOne	/*PD4:INT1*/	, Interrupt_Rising },
  #endif
  #if HAS_RFM69>=3
    { SPI_CONFIG_ID_3 , 7	 /*PD7*/, 8				/*PB0:PINCHG*/, Interrupt_Rising },
  #endif
  #if HAS_RFM69>=4
  	/* ACHTUNG!! doppelter PinChangeInterrupt geht aktuell nur mit Pin 6 */
    { SPI_CONFIG_ID_4 , A5/*ADC5*/, 6				/*PD6:PINCHG*/, Interrupt_Rising },
  #endif
    { SPI_CONFIG_ID_INVALID, 0, 0, Interrupt_Off   },
  };

  #include <myRFM69.h>
  myRFM69 myRFM69_1(SPI_CONFIG_ID_1, A0 /*Reset*/,true /*HighPower*/,RFM69_PROTOCOL_MyProtocol);	//868MHz - top
  #if HAS_RFM69>=2
  myRFM69 myRFM69_2(SPI_CONFIG_ID_2, A1 /*ADC1*/,true /*HighPower*/,RFM69_PROTOCOL_MyProtocol); //868MHz - bottom
  #endif
  #if HAS_RFM69>=3
  myRFM69 myRFM69_3(SPI_CONFIG_ID_3, A3 /*ADC3*/,true /*HighPower*/,RFM69_PROTOCOL_MyProtocol); 	//868MHz - top
  #endif
  #if HAS_RFM69>=4
  myRFM69 myRFM69_4(SPI_CONFIG_ID_4, A4 /*ADC4*/,true /*HighPower*/,RFM69_PROTOCOL_HX2262); 		//433MHz - top
  #endif
#endif

/*********************** Modules Table *****************************/

const typeModuleInfo ModuleTab[] = {
  { MODULE_DATAPROCESSING   ,"qhwW"    			, &DataProc }, //immer am Anfang

#if HAS_UART
  { MODULE_SERIAL           ,""       			, &Remote   },
#endif

#if HAS_AVR
  { MODULE_AVR              ,"mdopCAlabSN"   , &myAvr    },
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


// RFM immer am Schluss für Tunneling
#if HAS_RFM69
  { MODULE_SPI              ,""       		, &mySpi    },
  { MODULE_RFM69            ,"RfT" 				, &myRFM69_1   }, 	//868Mhz - top
	#if HAS_RFM69>=2
  { MODULE_RFM69            ,"RfT" 				, &myRFM69_2   }, //868Mhz - bottom
 	#endif
 	#if HAS_RFM69>=3
  { MODULE_RFM69            ,"RfT" 				, &myRFM69_3   }, //868Mhz - top
  #endif
 	#if HAS_RFM69>=4
  { MODULE_RFM69            ,"RfT" 				, &myRFM69_4   }, //433Mhz - top
	#endif
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

#if HAS_INFO_POLL
  bool infoPoll;
  byte preScaler=0;
  infoPoll = myBaseModule::infoPollCycles(false,&preScaler);
#endif

  while(pmt->typecode >= 0) {

	#if HAS_INFO_POLL
  	if(infoPoll) {
  		pmt->module->infoPoll(preScaler);
  	}

  	if(pmt->module->poll() || infoPoll) {
	#else
		if(pmt->module->poll()) {
 	#endif

    #if HAS_LEDs && defined(LED_ACTIVITY)
      myAVR::activityLed(1);
    #endif
		#if HAS_POWER_OPTIMIZATIONS
			myBaseModule::idleCycles(1); //is FIFO empty and idle time over?
		#endif
    }

    pmt++;
  }
}
