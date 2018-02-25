
#define LIBCALL_TurnOffFunctions	1 //needed to use WDT Interrupt Service Routine
#include "myAVR.h"

#ifndef LOWPOWER_MAX_IDLETIME
	#define LOWPOWER_MAX_IDLETIME		50
#endif

#if defined(INFO_POLL_CYCLE_TIME) && ((INFO_POLL_CYCLE_TIME > 2040) || (INFO_POLL_CYCLE_TIME < 8)) //uint8_t *8 Sekunden -> 2040 Sek -> 34 Min
	#error Max INFO_POLL_CYCLE_TIME is 2040 due to uint8_t limitation and can not be smaller than 8.
#endif

//#define PwrDown_AVR() { TIMING::delay(10); cli(); set_sleep_mode(SLEEP_MODE_PWR_DOWN); sleep_mode(); } // in den Schlafmodus wechseln
#define Reset_AVR() { wdt_enable(WDTO_15MS); while(1) {} }

#if READVCC_CALIBRATION_CONST
	uint16_t myAVR::AVR_InternalReferenceVoltage = 1150;
#endif

#if defined(STORE_CONFIGURATION)
	#if HAS_POWER_OPTIMIZATIONS
	byte EEMEM eeAVR_Auto_LowPower = false;
	#endif
#endif

#if HAS_POWER_OPTIMIZATIONS
	#ifndef AVR_LOWPOWER_DEFAULT_VALUE
		#define AVR_LOWPOWER_DEFAULT_VALUE	false
	#endif
	byte myAVR::AVR_Auto_LowPower = AVR_LOWPOWER_DEFAULT_VALUE;
	uint8_t myAVR::AVR_LowPower_InfoPoll_Counter = 0;
#endif

#if HAS_LEDs
	#ifndef LED1_PIN
		#define LED1_PIN	9
	#endif
	#ifndef DELAY_ACTIVITY_LED
		#define DELAY_ACTIVITY_LED	100
	#endif

	unsigned long myAVR::DelayActivityLED = 0;
	byte myAVR::activityLEDActive = 1;
	byte myAVR::blockActivityLED = 0;
#endif

#if (HAS_RFM12_OOK || HAS_RFM12_FSK)
	#include <myRFM12.h>
	extern myRFM12 myRFM;
#endif

void myAVR::initialize() {

#if HAS_POWER_OPTIMIZATIONS
	AVR_Auto_LowPower=AVR_LOWPOWER_DEFAULT_VALUE;
#if defined(STORE_CONFIGURATION)
	getStoredValue((void*)&AVR_Auto_LowPower, (const void*)&eeAVR_Auto_LowPower,1);
#endif
	AVR_LowPower_InfoPoll_Counter=0;
#endif

#if HAS_LEDs && defined(LED_ACTIVITY)
	DelayActivityLED = 0;
	activityLEDActive = 1;
	blockActivityLED = 0;
	activityLed(1);
#endif

#if HAS_SWITCH
	#if (!(defined(SWITCH1_S_PIN) && defined(SWITCH1_R_PIN)) && !(defined(SWITCH2_S_PIN) && defined(SWITCH2_R_PIN)))
		#error Please define SWITCHx_S_PIN and SWITCHx_R_PIN
	#endif
	#if(defined(SWITCH1_S_PIN) && defined(SWITCH1_R_PIN))
		pinMode(SWITCH1_S_PIN, OUTPUT);
		pinMode(SWITCH1_R_PIN, OUTPUT);
	#endif
	#if(defined(SWITCH2_S_PIN) && defined(SWITCH2_R_PIN))
		pinMode(SWITCH2_S_PIN, OUTPUT);
		pinMode(SWITCH2_R_PIN, OUTPUT);
	#endif
#endif

//#if HAS_ROLLO_SWITCH
//	#if !(defined(SWITCH_UP_PIN) && defined(SWITCH_DOWN_PIN))
//		#error Please define SWITCH_UP_PIN and SWITCH_DOWN_PIN
//	#endif
//	digitalWrite(SWITCH_UP_PIN, 0); //Wichtig: Erst ausschalten dann als Output konfigurieren
//	pinMode(SWITCH_UP_PIN, OUTPUT);
//	digitalWrite(SWITCH_DOWN_PIN, 0); //Wichtig: Erst ausschalten dann als Output konfigurieren
//	pinMode(SWITCH_DOWN_PIN, OUTPUT);
//#endif

#ifdef HAS_TRIGGER
	cInterrupt.initialize();
#endif

#ifdef HAS_TRIGGER_1
	cInterrupt1.initialize();
#endif

#ifdef READVCC_CALIBRATION_CONST
	initialVCCSample=true;
#endif

}

bool myAVR::poll() {
	bool ret=0;

#ifdef READVCC_CALIBRATION_CONST
	if(initialVCCSample) {
		initialVCCSample=false;
		VCC_Supply_mV = getVCC();
		if(DEBUG) { DS_P("VCC(cal): ");DU(VCC_Supply_mV,0);DS_P("mV\n"); }
	}
#endif

#if HAS_LEDs && defined(LED_ACTIVITY)
	activityLed(0);
#endif

#ifdef HAS_TRIGGER
	ret += cInterrupt.poll();
#endif

#ifdef HAS_TRIGGER_1
	ret += cInterrupt1.poll();
#endif

#if HAS_POWER_OPTIMIZATIONS
	if(AVR_Auto_LowPower && idleCycles(0,LOWPOWER_MAX_IDLETIME)) {
		//send((char*)"",MODULE_AVR_POWERDOWN);
		#if INCLUDE_DEBUG_OUTPUT
		if(DEBUG) { DS_P("awake: ");DU(TIMING::millis_since(awakeTime),0);DS_P("ms\n"); }
		#endif
		addToRingBuffer(MODULE_DATAPROCESSING, MODULE_SERIAL, NULL, 0); //flush UARTs
		addToRingBuffer(MODULE_DATAPROCESSING, MODULE_AVR_POWERDOWN, NULL, 0); //execute PowerDown command with RingBuffer because Debug Messages have to be flushed!
		ret += 1;
	}
#endif

	return ret;
}

void myAVR::infoPoll(byte prescaler) {

#if HAS_ADC || defined(INFO_POLL_PRESCALER_VCC)
	char buf[2];
	buf[1]=0x0;
#endif

#if HAS_ADC
	//char buffer[] = "6";
	#ifdef INFO_POLL_PRESCALER_ADC
	if(!(prescaler % INFO_POLL_PRESCALER_ADC))
	#endif
	{
//		#ifdef HAS_ADC_7_CALIBRATION_CONST
//			buf[0] = '7';
//			send(buf,MODULE_AVR_ADC);
//		#endif
		#if defined(HAS_ADC) && HAS_ADC
			buf[0] = '0'+HAS_ADC;
			send(buf,MODULE_AVR_ADC);
		#endif
	}
#endif

#ifdef INFO_POLL_PRESCALER_VCC
	if(!(prescaler % INFO_POLL_PRESCALER_VCC)) {
		buf[0]='r';
		send(buf,MODULE_AVR_ATMEGA_VCC); //poll VCC only in POWER_OPTIMIZATION  Mode
	}
#endif

}

int myAVR::getAvailableRam() {
	extern int __heap_start, *__brkval;
	int v;
	return ((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
}

#if HAS_ADC
unsigned long myAVR::getADCValue(uint8_t aPin) {

	//PowerOptimization(BOD_,ADC0_ON,AIN_,TIMER2_,TIMER1_,TIMER0_,SPI_,USART0_,TWI_,WDT_);
	PowerOpti_ADC0_ON;
	unsigned long ADCReading = analogRead(aPin);
	VCC_Supply_mV = getVCC();
	unsigned long ADCmV = (((float)ADCReading*VCC_Supply_mV)/(1023));//((float)ADCReading * 3.3 * 9)/(1023*2.976);
	//PowerOptimization(BOD_,ADC0_OFF,AIN_,TIMER2_,TIMER1_,TIMER0_,SPI_,USART0_,TWI_,WDT_);
	PowerOpti_ADC0_OFF;

	switch(aPin) {
	#if defined(HAS_ADC_6_CALIBRATION_CONST) && HAS_ADC_6_CALIBRATION_CONST>0
		case 6:
			ADCmV = ADCmV*(HAS_ADC_6_CALIBRATION_CONST/1000);
		break;
	#endif

	#if defined(HAS_ADC_7_CALIBRATION_CONST) && HAS_ADC_7_CALIBRATION_CONST>0
		case 7:
			ADCmV = ADCmV*(HAS_ADC_7_CALIBRATION_CONST/1000);
		break;
	#endif
	}
	return ADCmV;
}
#endif

#if READVCC_CALIBRATION_CONST
unsigned long myAVR::getVCC(void) { // gibt tatsächlichen Wert Vcc x 100 aus.

  unsigned long result;
  byte saveADMUX;
  byte saveADCSRA;
  byte saveADCSRB;

	PowerOpti_ADC_ON;

	saveADMUX = ADMUX;
  saveADCSRA = ADCSRA;
  saveADCSRB = ADCSRB;
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  // REFS1 REFS0          --> 0 1, AVcc internal reference -Selects AVcc external reference
  // MUX3 MUX2 MUX1 MUX0  --> 1110 1.1V (VBG)         -Wählt den Kanal 14, um die Bandgap-Spannung zu messen

  ADMUX = (1<<REFS0) | (1<<MUX3) | (1<<MUX2) | (1<<MUX1);
  ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
	ADCSRB=0;
  TIMING::delay(20); // Wait for Vref to settle

	//first measurement is most of the time slightly wrong
  ADCSRA |= (1<<ADSC);
  while(ADCSRA & (1<<ADSC)); //measuring
  TIMING::delay(50); // Wait for Vref to settle
	ADCSRA |= (1<<ADSC);
  while(ADCSRA & (1<<ADSC)); //measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both
  result = (high<<8) | low;
//  #ifdef READVCC_CALIBRATION_CONST
  	result = READVCC_CALIBRATION_CONST / result;
//  #else
//	  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000 (because...= 3300*1023/3 since 1.1 is exactly 1/3 of 3.3V)
//	#endif

	ADMUX = saveADMUX;
	ADCSRA = saveADCSRA;
	ADCSRB = saveADCSRB;

  PowerOpti_ADC_OFF;

  return result;
}
#endif

void myAVR::send(char *cmd, uint8_t typecode) {

#if READVCC_CALIBRATION_CONST || defined(HAS_ADC)
	unsigned long buf[2];
#endif

	switch(typecode) {

		case MODULE_AVR_RAMAVAILABLE:
			addToRingBuffer(MODULE_AVR_RAMAVAILABLE, 0, NULL, 0);
			break;

		case MODULE_AVR_REBOOT:
			Reset_AVR();
			break;

#if READVCC_CALIBRATION_CONST
		case MODULE_AVR_ATMEGA_VCC:
			if(strlen(cmd) == 4) { //1100
				AVR_InternalReferenceVoltage = atoi(cmd);
			}
			if(cmd[0] == 'r') { //reread VCC voltage
				VCC_SUPPLY_mV = getVCC();
			}
			buf[1] = VCC_SUPPLY_mV;
			addToRingBuffer(MODULE_AVR_ATMEGA_VCC, 0, (const byte*)buf, sizeof(unsigned long)*2);
			break;
#endif

#if HAS_ADC
		case MODULE_AVR_ADC:
			if(!strlen(cmd)) break;
			buf[0] = (unsigned long)atoi(cmd);
			buf[1] = getADCValue(atoi(cmd));
			addToRingBuffer(MODULE_AVR_ADC, 0, (const byte*)buf, sizeof(unsigned long)*2);
			break;
#endif

/*
#if defined(HAS_TRIGGER) || defined(HAS_TRIGGER_1)
		case MODULE_AVR_TRIGGER:
			//addToRingBuffer(MODULE_AVR_TRIGGER, 0, NULL, 0);
			cInterrupt.send(cmd, typecode);
			break;
#endif
*/

#if HAS_POWER_OPTIMIZATIONS
		case MODULE_AVR_POWERDOWN:
			if(strlen(cmd)==1) {
				AVR_Auto_LowPower = (cmd[0]=='0' ? 0 : 1);
				#if defined(STORE_CONFIGURATION)
					StoreValue((void*)&AVR_Auto_LowPower,(void*)&eeAVR_Auto_LowPower,1);
				#endif
				break;
			}

			//DS_P("PowerDown\n");
		  #if HAS_LEDs && defined(LED_ACTIVITY)
		  if(activityLEDActive) {
				LedOnOff(LED_ACTIVITY,0);
  	    DelayActivityLED=0;
  	  }
		  #endif

		  send((char*)"",MODULE_AVR_LOWPOWER);

	#if INFO_POLL_CYCLE_TIME
			for(; (AVR_LowPower_InfoPoll_Counter*8)<INFO_POLL_CYCLE_TIME; AVR_LowPower_InfoPoll_Counter++) {
				FKT_WDT_PERIODE_ON(SLEEP_8S); //if sleep time is not forever
				//FKT_WDT_PERIODE_ON(SLEEP_15Ms);
				lowPowerBodOff(SLEEP_MODE_PWR_DOWN); //with BOD_OFF only
				if(WDTCSR) break; //WDTCSR will be cleared after a WD event, otherwhise it was an ExtInterrupt
			}
			TIMING::configure(); //reset timing functionality millis,delay...

		  #if HAS_LEDs && defined(LED_ACTIVITY)
		  if(activityLEDActive) {
			  LedOnOff(LED_ACTIVITY,1);
  	  	DelayActivityLED=TIMING::millis();
  	  }
		  #endif

			//Functions for InfoPoll`s
			if( !WDTCSR && AVR_LowPower_InfoPoll_Counter*8 >= INFO_POLL_CYCLE_TIME ) {
				AVR_LowPower_InfoPoll_Counter=0;

				#if HAS_INFO_POLL && INFO_POLL_CYCLE_TIME
				infoPollCycles(1); //force InfoPoll Functions
				#endif
			}
	#else
			FKT_WDT_OFF;
			lowPowerBodOff(SLEEP_MODE_PWR_DOWN); //with BOD_OFF only

			TIMING::configure(); //reset timing functionality millis,delay...
	#endif

	#if INCLUDE_DEBUG_OUTPUT
			awakeTime = TIMING::millis();
	#endif
			idleCycles(1); //reset idle cylces
			break;

		case MODULE_AVR_LOWPOWER:
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
			//TurnOffFunktions(BOD_, ADC_, AIN_, TIMER2_ /*millis*/ ,TIMER1_OFF /* IRMP */, TIMER0_OFF, SPI_, USART0_, TWI_, WDT_);
			FKT_TIMER1_OFF;
			FKT_TIMER0_OFF;
			#endif
			#if !HAS_IR_TX
			//TurnOffFunktions(BOD_, ADC_, AIN_, TIMER2_, TIMER1_, TIMER0_OFF, SPI_, USART0_, TWI_, WDT_);
			FKT_TIMER0_OFF;
			#endif
			//TurnOffFunktions(BOD_OFF, ADC_OFF, AIN_OFF, TIMER2_ /*millis*/ ,TIMER1_ /* IRMP */, TIMER0_ /* IRSND */, SPI_OFF /* RFM69 */, USART0_ /* auto on/off tunneling */, TWI_OFF, WDT_OFF);
			FKT_BOD_OFF;
			FKT_ADC0_OFF;
			FKT_AIN_OFF;
			FKT_SPI_OFF;
			FKT_TWI_OFF;
			FKT_WDT_OFF;
		break;
#endif

#if HAS_LEDs
		case  MODULE_AVR_LED:
			if(cmd[0]=='0') {
			#ifdef LED_ACTIVITY
				if(LED_ACTIVITY==1) blockActivityLED=0;
			#endif
				LedOnOff(1,0);
			} else {
			#ifdef LED_ACTIVITY
				if(LED_ACTIVITY==1) blockActivityLED=1;
			#endif
				LedOnOff(1,1);
			}
			break;

	#ifdef LED_ACTIVITY
		case MODULE_AVR_ACTIVITYLED:
      LedOnOff(LED_ACTIVITY,0);
      DelayActivityLED=0;
      activityLEDActive = (cmd[0]=='0' ? 0 : 1);
			break;
  #endif

#endif

#if HAS_BUZZER
		case MODULE_AVR_BUZZER:
			BuzzerTime(cmd);
			break;
#endif

#if HAS_SWITCH
		case MODULE_AVR_SWITCH:
			SwitchOnOff(cmd);
			break;
#endif

//#if HAS_ROLLO_SWITCH
//		case MODULE_AVR_SWITCH:
//			SwitchUpDown(cmd);
//			break;
//#endif
	}
}

#if HAS_LEDs
void myAVR::LedOnOff (uint8_t id, uint8_t on) {
		if(id==1) {
	#ifdef LED1_PIN
		  pinMode(LED1_PIN, OUTPUT);
		  digitalWrite(LED1_PIN, on);
	#endif
		} else if(id==2) {
	#ifdef LED2_PIN
		  pinMode(LED2_PIN, OUTPUT);
		  digitalWrite(LED2_PIN, on);
	#endif
		}
}
#endif

#if HAS_LEDs && defined(LED_ACTIVITY)
void myAVR::activityLed(uint8_t on) {

  if( (!DelayActivityLED && !on) || !activityLEDActive || blockActivityLED) return;

 	unsigned long msec = TIMING::millis();
  if(!on) {
    if( DELAY_ACTIVITY_LED <= (DelayActivityLED > msec ? 0xFFFFFFFF - DelayActivityLED + msec : msec - DelayActivityLED ) ) {
      DelayActivityLED=0;
			LedOnOff(LED_ACTIVITY,on);
    }
  } else {
    DelayActivityLED=TIMING::millis();
		if(!DelayActivityLED) DelayActivityLED++; //for initialization
   	LedOnOff(LED_ACTIVITY,on);
  }
}
#endif

#if HAS_BUZZER && defined(BUZZER_PIN)
void myAVR::BuzzerTime (const char* cmd) { //2300Hz --> 217µs on/off [HEX D9]

#ifndef BUZZER_FREQUENCE
	if(strlen(cmd)<5) return;
  char s_freq[] = { cmd[0], cmd[1], cmd[2], cmd[3], 0x0 }; //max 99
  long freq = atol(s_freq); //1000000/freq/2; // calculate the delay value between transitions
	long ms = atol(cmd+4);
  long delayValue = 250000/freq; // (1/f)*1000[ms]*1000[µs]/2*0,5 //calculate the delay value between transitions
  long numCycles = freq * ms/1000; // calculate the number of cycles for proper timing
#else
	long ms = atol(cmd);
	long numCycles = BUZZER_FREQUENCE * ms; // calculate the number of cycles for proper timing
#endif

  pinMode(BUZZER_PIN, OUTPUT);
  //word now = TIMING::millis();
  //while((TIMING::millis()-now)<=ms) {
  for (long i=0; i < numCycles; i++) {
#ifndef BUZZER_FREQUENCE
    digitalWrite(BUZZER_PIN,HIGH); // write the buzzer pin high to push out the diaphram
    delayMicroseconds(delayValue); // wait for the calculated delay value
    digitalWrite(BUZZER_PIN,LOW); // write the buzzer pin low to pull back the diaphram
    delayMicroseconds(delayValue*3); // wait again or the calculated delay value
#else
    digitalWrite(BUZZER_PIN,HIGH); // write the buzzer pin high to push out the diaphram
    delayMicroseconds(BUZZER_ON_TIME); // wait for the calculated delay value
    digitalWrite(BUZZER_PIN,LOW); // write the buzzer pin low to pull back the diaphram
    delayMicroseconds(BUZZER_OFF_TIME); // wait again or the calculated delay value
#endif
  }
  pinMode(BUZZER_PIN, INPUT);
}
#endif

#if HAS_SWITCH
void myAVR::SwitchOnOff(const char* cmd) {
	if(strlen(cmd)!=2) return;

#if (defined(SWITCH1_S_PIN) && defined(SWITCH1_R_PIN))
	if(cmd[0]=='1') {
    digitalWrite(SWITCH1_S_PIN, (cmd[1]=='1'?1:0));
    digitalWrite(SWITCH1_R_PIN, (cmd[1]=='1'?0:1));
	}
#endif
#if (defined(SWITCH2_S_PIN) && defined(SWITCH2_R_PIN))
	if(cmd[0]=='2') {
    digitalWrite(SWITCH2_S_PIN, (cmd[1]=='1'?1:0));
    digitalWrite(SWITCH2_R_PIN, (cmd[1]=='1'?0:1));
	}
#endif

	TIMING::delay(50);

#if (defined(SWITCH1_S_PIN) && defined(SWITCH1_R_PIN))
	if(cmd[0]=='1') {
    digitalWrite(SWITCH1_S_PIN, 0);
    digitalWrite(SWITCH1_R_PIN, 0);
	}
#endif
#if (defined(SWITCH2_S_PIN) && defined(SWITCH2_R_PIN))
	if(cmd[0]=='2') {
    digitalWrite(SWITCH2_S_PIN, 0);
    digitalWrite(SWITCH2_R_PIN, 0);
	}
#endif
}
#endif

//#if HAS_ROLLO_SWITCH
//void myAVR::SwitchUpDown(const char* cmd) {

//	byte sdelay=0;
//#if INCLUDE_DEBUG_OUTPUT
//	if(cmd[0]==0x0) return; //only in Debug Mode setting SSDs to up/down allowed for an infinitly time
//	if(strlen(cmd)>1) {
//		sdelay = (byte)atoi(cmd+1);
//	}
//#else
//	if(strlen(cmd)<2) return;
//	sdelay = (byte)atoi(cmd+1);
//#endif


//	//Shutoff SSRs
//  digitalWrite(SWITCH_UP_PIN, 0);
//  digitalWrite(SWITCH_DOWN_PIN, 0);
//  TIMING::delay(100);//ms 50Hz supply frequence --> 0.02s
//
//	//Up/Down/off
//  switch(cmd[0]) {
//  	case '1': //Down
//	    digitalWrite(SWITCH_DOWN_PIN, 1);
//    break;
//		case '2':	//Up
//	  	digitalWrite(SWITCH_UP_PIN, 1);
//	  break;
//	  default:
//	  	return;
//	}

//	if(!sdelay) return;
//	TIMING::delay(sdelay*100);//ms (*1000/10)
//
//	//Up/Down/off
//  switch(cmd[0]) {
//  	case '1': //Down
//  		digitalWrite(SWITCH_DOWN_PIN, 0); //off
//  		TIMING::delay(100);//ms 50Hz supply frequence --> 0.02s
//
//			//SSRs safety shutoff | in case of rollo endpoint stop
//	    digitalWrite(SWITCH_UP_PIN, 1);
//	    TIMING::delay(500);//ms 50Hz supply frequence --> 0.02s, testing hat 500 ergeben ;|
//	    digitalWrite(SWITCH_UP_PIN, 0);
//    break;
//
//		case '2':	//Up
//	  	digitalWrite(SWITCH_UP_PIN, 0);
//			TIMING::delay(100);//ms 50Hz supply frequence --> 0.02s
//
//			//SSRs safety shutoff | in case of rollo endpoint stop
//	    digitalWrite(SWITCH_DOWN_PIN, 1);
//	    TIMING::delay(500);//ms 50Hz supply frequence --> 0.02s
//	    digitalWrite(SWITCH_DOWN_PIN, 0);
//	  break;
//	}
//}
//#endif

void myAVR::displayData(RecvData *DataBuffer) {

	unsigned long buf[2];

	switch(DataBuffer->ModuleID) {

		case MODULE_AVR_RAMAVAILABLE:
			DU(getAvailableRam(),0);
			break;
#if HAS_ADC
		case MODULE_AVR_ADC:
			memcpy((void*)buf,DataBuffer->Data,DataBuffer->DataSize);
			DU(buf[0],0);
			DU(buf[1],0);
			break;
#endif

		case MODULE_AVR_ATMEGA_VCC:
			memcpy((void*)buf,DataBuffer->Data,DataBuffer->DataSize);
			DU(buf[1],0);
			break;

#if defined(HAS_TRIGGER) || defined(HAS_TRIGGER_1)
		case MODULE_AVR_TRIGGER:
			//DU(triggerDetected,0);
			#ifdef HAS_TRIGGER
			cInterrupt.displayData(DataBuffer);
			#endif

			#ifdef HAS_TRIGGER_1
			cInterrupt1.displayData(DataBuffer);
			#endif

			break;
#endif

	}
}

void myAVR::printHelp() {

	DS_P(" * [RAM]     m\n");
#if HAS_POWER_OPTIMIZATIONS
	DS_P(" * [PwrDown] d<0:off,1:on>\n");
	DS_P(" * [LowPwr]  p\n");
#endif
	DS_P(" * [Reboot]  o\n");
#if HAS_ADC
	DS_P(" * [ADC]     C<pin>\n");
#endif
#if defined(HAS_TRIGGER) || defined(HAS_TRIGGER_1)
	#ifdef INCLUDE_BUTTON
		DS_P(" * [TRIGGER] N<Pin><0:low,1:pulse,2:doublePulse,3:longPulse>\n");
	#else
		DS_P(" * [TRIGGER] N<Pin><0:low,1:pulse>\n");
	#endif
#endif
#if READVCC_CALIBRATION_CONST
	DS_P(" * [VCC]     A<r:reMeasure>\n");
#endif
#if HAS_LEDs
	DS_P("\n ## LED ##\n");
	DS_P(" * [LED] l<0:off,1:on>\n");
	#ifdef LED_ACTIVITY
	DS_P(" * [Act] a<0:off,1:on>\n");
	#endif
#endif
#if HAS_SWITCH
	DS_P("\n ## SWITCH ##\n");
	DS_P(" * [SWITCH]  S<1|2><0:off,1:on>\n");
#endif
//#if HAS_ROLLO_SWITCH
//	DS_P("\n ## ROLLO-SWITCH ##\n");
//	DS_P(" * [SWITCH]  S<0:off,1:down,2:up><sec/10>\n");
//#endif

#if HAS_BUZZER
	DS_P("\n ## BUZZER ##\n");
	#ifndef BUZZER_FREQUENCE
		DS_P(" * [BEEP] b<freq[4]><ms>\n");
	#else
		DS_P(" * [BEEP] b<ms>\n");
	#endif
#endif
}
