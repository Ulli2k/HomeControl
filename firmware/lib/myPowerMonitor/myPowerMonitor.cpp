
#include <myPowerMonitor.h>

#define myPOWERMONITOR_VOLTAGE_CAL						216.5		//230(Main Voltage) × 13(VoltageDevider 10k+120k) ÷ ( 9 (Secondary Voltage) × 1.20(transformer without load) ) = 276,85 (emonTX v3)
#define myPOWERMONITOR_CURRENT_CAL						10.646//13.3333	//(current constant = (100/0,050)/150(burden) = 13,3333
#define myPOWERMONITOR_POWERFAKTOR_CAL				1.28//7 		//1.7

#define myPOWERMONITOR_CURRENT_ONLY						0xFF
#define myPOWERMONITOR_AvgSmoothFactor  			0.25

#define myPOWERMONITOR_InfoIAvgChangeThres		0.05
#define myPOWERMONITOR_InfoVAvgChangeThres		20
#define myPOWERMONITOR_InfoPAvgChangeThres		5

#define myPOWERMONITOR_MIN_I									0.01
#define myPowerMonitor_ADCPin2Char(x)					((x) - A0 + '0')

//#define myPOWERMONITOR_PULSE_PIN							INTOne //aktuell gehen nur INT0 oder INT1
//#define POWER_MONITOR_PULSE_DEBOUNCETIME			100

myPOWERMONITOR::myPOWERMONITOR() {
#ifdef HAS_POWER_MONITOR_CT
	skipFirstSample=true;
#endif

}

void myPOWERMONITOR::initialize() {

#ifdef HAS_POWER_MONITOR_CT
	nextIndex4OutPut = 0;
  _emon = EmonLibPro();
  _emon.begin();
#endif
  
//#ifdef HAS_POWER_MONITOR_PULSE
//	pulseDetected=!HAS_POWER_MONITOR_PULSE;
//	pulseLevelTime=0;
//  cfgInterrupt(this, myPOWERMONITOR_PULSE_PIN, Interrupt_Change);
//#endif
}

bool myPOWERMONITOR::poll() {

#ifdef HAS_POWER_MONITOR_CT
	if(nextIndex4OutPut) {
		TIMING::delay(50); //delay for RFM-TX
		addToRingBuffer(MODULE_POWERMONITOR_POWER, --nextIndex4OutPut, NULL, 0);
		return 1;
		
	} else if(_emon.FlagCALC_READY) {
  	_emon.calculateResult();
  	if(skipFirstSample) { skipFirstSample=false; return 0; } // Skip first results due to garbage
  	if(relevantChanges()) {
  		infoPoll(0);
  	}
		return 1;  	
  }
#endif

//#ifdef HAS_POWER_MONITOR_PULSE
//	if( (((TIMING::millis() - pulseLevelTime) > POWER_MONITOR_PULSE_DEBOUNCETIME) && pulseDetected!=pulseLevel ) || 
//			(pulseLevel==HAS_POWER_MONITOR_PULSE && pulseDetected!=HAS_POWER_MONITOR_PULSE) ) {
//		pulseDetected = pulseLevel;
//		if(pulseDetected==HAS_POWER_MONITOR_PULSE) {
//			const char buf = 'p';
//			addToRingBuffer(MODULE_POWERMONITOR_POWER, 0, (unsigned char*)&buf, 1);
//			return 1;
//		}
//	}
//#endif

  return 0;
}	

#ifdef HAS_POWER_MONITOR_CT
#define absDiff(x,y) ((x)>(y)?(x)-(y):(y)-(x))
bool myPOWERMONITOR::relevantChanges() {

	if(myPOWERMONITOR_InfoVAvgChangeThres <= absDiff(lastResultV,_emon.ResultV[0].U)) {
		lastResultV = _emon.ResultV[0].U;
		return 1;
	}
	for (uint8_t i=0; i<CURRENTCOUNT; i++) {
		if(	myPOWERMONITOR_InfoIAvgChangeThres <= absDiff(lastResultP[i].I,_emon.ResultP[i].I) ||
	#if VOLTSCOUNT==0
				myPOWERMONITOR_InfoPAvgChangeThres <= absDiff(lastResultP[i].S,_emon.ResultP[i].S) 
	#else
				myPOWERMONITOR_InfoPAvgChangeThres <= absDiff(lastResultP[i].P,_emon.ResultP[i].P) 
	#endif
			) {
			lastResultP[i].I = _emon.ResultP[i].I;
	#if VOLTSCOUNT==0
			lastResultP[i].S = _emon.ResultP[i].S;
	#else
			lastResultP[i].P = _emon.ResultP[i].P;
	#endif
			return 1;
		}
	}	
	return 0;
}
#endif

//#ifdef HAS_POWER_MONITOR_PULSE
////volatile unsigned long lastPulseDetected=0, entprellZeit=20;
////#define POWER_MONITOR_PULSE_DEBOUNCETIME	100

//void myPOWERMONITOR::interrupt() {
//	pulseLevel = digitalRead((myPOWERMONITOR_PULSE_PIN==INTOne?3:2));
//	pulseLevelTime = TIMING::millis();
//}
//#endif

void myPOWERMONITOR::infoPoll(byte prescaler) {
#ifdef HAS_POWER_MONITOR_CT
	char b = 'c';
	send(&b,MODULE_POWERMONITOR_POWER); //just print the current available results
#endif
}

void myPOWERMONITOR::send(char *cmd, uint8_t typecode) {

//	OutPutData data;
	switch(typecode) {
#ifdef HAS_POWER_MONITOR_CT
		case MODULE_POWERMONITOR_POWER:
			
			if(cmd[0] == 'e') {
				_emon.printStatus();
				break;
			}
			if(cmd[0] != 'c') {
				unsigned long startTime = TIMING::millis();
				while (TIMING::millis()-startTime <= SAMPLE_CYCLE_TIME_MS && !(_emon.FlagCALC_READY));
				if(!_emon.FlagCALC_READY) break;
				_emon.calculateResult();
			}
			nextIndex4OutPut=CURRENTCOUNT;
			for (uint8_t i=0; i<CURRENTCOUNT; i++) {
				if(DEBUG) {
					DS_P("PowerMonitor[");
				#if VOLTSCOUNT==0
					DU(adc_pin_order[i],0);
				#else
					DU(adc_pin_order[i+1],0);
				#endif
					DS_P("]: ");
					Serial.print(_emon.ResultV[0].U,3);DS_P("V ");
					Serial.print(_emon.ResultV[0].HZ,3);DS_P("Hz ");

					Serial.print(_emon.ResultP[i].I,3);DS_P("A ");
					Serial.print(_emon.ResultP[i].P,3);DS_P("W ");
					Serial.print(_emon.ResultP[i].S,3);DS_P("VA ");
					Serial.print(_emon.ResultP[i].F,3);DS_P("Pfact ");
					DNL();
				}
			}
			//DS("send\n");
			//addToRingBuffer(MODULE_POWERMONITOR_POWER, --nextIndex4OutPut, NULL, 0);
			break;
#endif			
	}
}

#define RoundAndShift(x) (long int)(((x)>0?(x)+0.005:(x)-0.005)*100)
void myPOWERMONITOR::displayData(RecvData *DataBuffer) {

//#ifdef HAS_POWER_MONITOR_PULSE	
//		if(DataBuffer->Data[0] == 'p') {
//			DC('p');
//			return;
//		}
//#endif

#ifdef HAS_POWER_MONITOR_CT
	#if VOLTSCOUNT==0
		DU(adc_pin_order[DataBuffer->DataTypeCode],0);
	#else
		DU(adc_pin_order[DataBuffer->DataTypeCode+1],0);
	#endif
		DH4(RoundAndShift(_emon.ResultP[DataBuffer->DataTypeCode].I));
		DH4(RoundAndShift(_emon.ResultV[0].U));
	#if VOLTSCOUNT==0
		DH4(RoundAndShift(_emon.ResultP[DataBuffer->DataTypeCode].S)>>16); 
		DH4(RoundAndShift(_emon.ResultP[DataBuffer->DataTypeCode].S));
	#else
		DH4(RoundAndShift(_emon.ResultP[DataBuffer->DataTypeCode].P)>>16); 
		DH4(RoundAndShift(_emon.ResultP[DataBuffer->DataTypeCode].P));	
		DH4(RoundAndShift(_emon.ResultP[DataBuffer->DataTypeCode].F)>>16);
		DH4(RoundAndShift(_emon.ResultP[DataBuffer->DataTypeCode].F));
	#endif
#endif
}

void myPOWERMONITOR::printHelp() {

	DS_P("\n ## PowerMontor ##\n");
#ifdef HAS_POWER_MONITOR_CT	
	DS_P(" * [Power] P<c:current available>\n");
#endif
//#ifdef HAS_POWER_MONITOR_PULSE	
//	DS_P(" * [Pulse] Pp\n");
//#endif
	
	
}
