#ifndef _MY_ANALOGPIN_h
#define _MY_AN

//TODO:: Static ist nicht wirklich static

#include <myBaseModule.h>

template <uint8_t Pin, uint8_t id=Pin, uint32_t calConst=0>
class analogPin : public myBaseModule {

public:
  // static uint8_t firstInitializedAnalogPinID;
  static unsigned int VCC_mV;

  #ifdef READVCC_CALIBRATION_CONST
    static uint16_t AVR_InternalReferenceVoltage;
    static bool initialVCCSample;
  #endif

public:

    analogPin() { }
	   const char* getFunctionCharacter() { return "AC"; };    
    void initialize() {
      // if(!firstInitializedAnalogPinID) {
      //   DS("first is");DU(id,0);DNL();
      //   firstInitializedAnalogPinID = id;
      // }
    }

    bool poll() {
    	bool ret=0;

    #ifdef READVCC_CALIBRATION_CONST
    	if(initialVCCSample) {
    		initialVCCSample=false;
    		VCC_mV = getVCC();
    		if(DEBUG) { DS_P("VCC(cal): ");DU(VCC_mV,0);DS_P("mV\n"); }
        ret=1;
    	}
    #endif

      return ret;
    }

    void infoPoll(byte prescaler) {
    	char buf[3];
    	buf[1]=0x0;buf[2]=0x0;

    	#ifdef INFO_POLL_PRESCALER_ADC
    	if(!(prescaler % INFO_POLL_PRESCALER_ADC))
    	#endif
    	{
    			buf[0] = '0' + id / 10 ;
          buf[1] = '0' + id % 10;
    			send(buf,MODULE_ANALOG_PIN_SAMPLE);
    	}

    #ifdef INFO_POLL_PRESCALER_VCC
      // if(firstInitializedAnalogPinID!=id) return;
    	if(!(prescaler % INFO_POLL_PRESCALER_VCC)) {
    		buf[0]='r';
    		send(buf,MODULE_ANALOG_PIN_VCC); //poll VCC only in POWER_OPTIMIZATION  Mode
    	}
    #endif
    }

    void send(char *cmd, uint8_t typecode) {
    	unsigned long buf[2];

    	switch(typecode) {

    		case MODULE_ANALOG_PIN_VCC:
          // if(firstInitializedAnalogPinID!=id) break;
        #if READVCC_CALIBRATION_CONST
    			if(strlen(cmd) == 4) { //1100
    				AVR_InternalReferenceVoltage = atoi(cmd);
    			}
    			if(cmd[0] == 'r') { //reread VCC voltage
    				VCC_mV = getVCC();
    			}
        #endif

          buf[1] = VCC_mV;
    			addToRingBuffer(MODULE_ANALOG_PIN_VCC, 0, (const byte*)buf, sizeof(unsigned long)*2);
    			break;


    		case MODULE_ANALOG_PIN_SAMPLE:
    			if(!strlen(cmd) || id != atoi(cmd)) break;
    			buf[0] = (unsigned long)id;
    			buf[1] = getADCValue(atoi(cmd));
    			addToRingBuffer(MODULE_ANALOG_PIN_SAMPLE, 0, (const byte*)buf, sizeof(unsigned long)*2);
    			break;
    	}
    }

    #if !defined(__AVR_ATmega328P__)
    static unsigned long getVCC(void) {
      return VCC_mV;
    }
    #else
    static unsigned long getVCC(void) { // gibt tatsächlichen Wert Vcc x 100 aus.

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
      delay(20); // Wait for Vref to settle

    	//first measurement is most of the time slightly wrong
      ADCSRA |= (1<<ADSC);
      while(ADCSRA & (1<<ADSC)); //measuring
      delay(50); // Wait for Vref to settle
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


    unsigned long getADCValue(uint8_t aPin) {

    	//PowerOptimization(BOD_,ADC0_ON,AIN_,TIMER2_,TIMER1_,TIMER0_,SPI_,USART0_,TWI_,WDT_);
    	PowerOpti_ADC0_ON;
    	unsigned long ADCReading = analogRead(aPin);
    	VCC_mV = getVCC();
    	unsigned long ADCmV = (((float)ADCReading*VCC_mV)/(1023));//((float)ADCReading * 3.3 * 9)/(1023*2.976);
    	//PowerOptimization(BOD_,ADC0_OFF,AIN_,TIMER2_,TIMER1_,TIMER0_,SPI_,USART0_,TWI_,WDT_);
    	PowerOpti_ADC0_OFF;

      if(calConst) {
        ADCmV = ADCmV*(calConst/1000);
      }
    	return ADCmV;
    }

    void displayData(RecvData *DataBuffer) {

    	unsigned long buf[2];
    	switch(DataBuffer->ModuleID) {

    		case MODULE_ANALOG_PIN_SAMPLE:
    			memcpy((void*)buf,DataBuffer->Data,DataBuffer->DataSize);
    			DU(buf[0],2);
    			DU(buf[1],0);
    			break;

    		case MODULE_ANALOG_PIN_VCC:
          // if(firstInitializedAnalogPinID!=id) break;
    			memcpy((void*)buf,DataBuffer->Data,DataBuffer->DataSize);
    			DU(buf[1],0);
    			break;
    	}
    }

    void printHelp() {

      DS_P("\n ## ANALOG PIN - ID:");DU(id,0);DS(" ##\n");

    	DS_P(" * [ANALOG]  C<ID>\n");
      // if(firstInitializedAnalogPinID==id) {
      #if READVCC_CALIBRATION_CONST
      	DS_P(" * [VCC]     A<r:reMeasure>\n");
      #else
        DS_P(" * [VCC]     A\n");
      #endif
      // }
  }
};

template <uint8_t Pin, uint8_t id, uint32_t calConst>
unsigned int analogPin<Pin, id, calConst>::VCC_mV = 3300;

// template <uint8_t Pin, uint8_t id, uint32_t calConst>
// uint8_t analogPin<Pin, id, calConst>::firstInitializedAnalogPinID = 0;

#ifdef READVCC_CALIBRATION_CONST
  template <uint8_t Pin, uint8_t id, uint32_t calConst>
  uint16_t analogPin<Pin, id, calConst>::AVR_InternalReferenceVoltage = 1150;

  template <uint8_t Pin, uint8_t id, uint32_t calConst>
  bool analogPin<Pin, id, calConst>::initialVCCSample = true;
#endif

#endif
