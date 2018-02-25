
#ifndef _MY_BASE_MODULE_
#define _MY_BASE_MODULE_

#include <avr/pgmspace.h> //PSTR() und phm_read_byte
#include <RingBuffer.h>
#include <myTiming.h>

#if defined(STORE_CONFIGURATION)
	#include <avr/eeprom.h>
#endif

//https://github.com/GreyGnome/PinChangeInt
#ifndef PinChangeInt_h
// #define NO_PORTB_PINCHANGES // to indicate that port b will not be used for pin change interrupts
 #define NO_PORTC_PINCHANGES // to indicate that port c will not be used for pin change interrupts
 #define NO_PORTD_PINCHANGES // to indicate that port d will not be used for pin change interrupts
 #define NO_PIN_STATE        // to indicate that you don't need the pinState
 #define NO_PIN_NUMBER       // to indicate that you don't need the arduinoPin
 #define DISABLE_PCINT_MULTI_SERVICE
 #define LIBCALL_PINCHANGEINT
 #include "PinChangeInt.h"
#endif

//#include "../../src/HomeControl/board.h"	//!!!! ACHTUNG include auf board.h !!!!

/*********************** Declare Module Commands ***********************/
#define MODULE_DATAPROCESSING    									0x00
	#define MODULE_DATAPROCESSING_QUIET	 								0x10
	#define MODULE_DATAPROCESSING_VERSION								0x20
	#define MODULE_DATAPROCESSING_HELP									0x30	
	#define MODULE_DATAPROCESSING_FIRMWARE							0x40
	#define MODULE_DATAPROCESSING_WAKE_SIGNAL						0x50
	#define MODULE_DATAPROCESSING_DEVICEID							0xE0	//internal use, no official command	
	#define MODULE_DATAPROCESSING_OUTPUTTUNNEL					0xF0	//internal use, no official command
#define MODULE_AVR							 									0x01
	#define MODULE_AVR_RAMAVAILABLE			 								0x11
	#define MODULE_AVR_POWERDOWN												0x21
	#define MODULE_AVR_REBOOT														0x31
	#define MODULE_AVR_LOWPOWER													0x41
	#define MODULE_AVR_ADC															0x51
	#define MODULE_AVR_ATMEGA_VCC												0x61
	#define MODULE_AVR_LED															0x71
	#define MODULE_AVR_ACTIVITYLED											0x81
	#define MODULE_AVR_BUZZER														0x91
	#define MODULE_AVR_SWITCH														0xA1
	#define MODULE_AVR_TRIGGER													0xB1
#define MODULE_SERIAL           	 								0x02
#define MODULE_IRMP             									0x03
#define MODULE_RFM12            	 								0x04
	#define MODULE_RFM12_OOK        								 		0x14
	#define MODULE_RFM12_FSK         								 		0x24
	#define MODULE_RFM12_TUNE        								 		0x34
	#define MODULE_RFM12_RAW         								 		0x44
	#define MODULE_RFM12_MODEQUERY   								 		0x54			
#define MODULE_SPI																0x05
#define MODULE_RFM69															0x06
	#define MODULE_RFM69_OPTIONS												0x16
	#define MODULE_RFM69_MODEQUERY											0x26
	#define MODULE_RFM69_TUNNELING											0x36
	#define MODULE_RFM69_SENDACK												0xF6 //internal use, no official command
	#define MODULE_RFM69_OPTION_SEND										0xE6 //internal use, no official command
	#define MODULE_RFM69_OPTION_POWER										0xD6 //internal use, no official command
	#define MODULE_RFM69_OPTION_TEMP										0xC6 //internal use, no official command
#define MODULE_BME280															0x07
	#define MODULE_BME280_ENVIRONMENT										0x17
#define MODULE_POWERMONITOR												0x08
	#define MODULE_POWERMONITOR_POWER										0x18
#define MODULE_ROLLO															0x09
	#define MODULE_ROLLO_CMD														0x19
	#define MODULE_ROLLO_EVENT													0x29									

#define MODULE_TYP(m) 						(uint8_t)(m & 0x0F)
#define MODULE_PROTOCOL(m)				(m > 0x0F ? ( (m >> 4) -1) : 0)
#define MODULE_COMMAND_CHAR(m,c)  ( { char b='-'; const typeModuleInfo* pmt = ModuleTab; while(pmt->typecode >= 0) { if(pmt->typecode==(m)) { b=(((uint8_t)strlen(pmt->name) >= (uint8_t)((c)>>4)) ? (pmt->name[((c)>>4)-1]) : '-'); break; } pmt++; } b; } )


/*********************** Declare Display Functions for all Modules ***********************/
#define isUartTxActive  (UCSR0B & (1<<TXEN0))
#define DC 			display_char
#define DS 			display_string
#define DS_P(a)	display_string_P(PSTR(a))
#define DU(a,b) display_udec(a,b,'0')
#define DI(a,b) display_int(a,b,'0')
#define DF(a,b) display_float(a,b,'0')
#define DH(a,b) display_hex(a,b,'0')
#define DH2(a) 	display_hex2(a)
#define DH4(a) 	display_hex4(a)
#define DB(a)		display_bin(a)
#define DNL 		display_nl 
#define DFL			display_flush

#if INCLUDE_DEBUG_OUTPUT
	#define DEBUG			(!myBaseModule::QuietMode)
#else
	#define DEBUG			0
#endif
#define DEBUG_ONOFF(x) (myBaseModule::QuietMode = (x))
#define D_DS_P(a) { if(DEBUG) { DS_P(a); } }
#define D_DC(a)		{ if(DEBUG) { DC(a); } }
#define D_DS(a) 	{ if(DEBUG) { DS(a); } }  
#define D_DI(a,b) { if(DEBUG) { DI(a,b); } }  
#define D_DH2(a) 	{ if(DEBUG) { DH2(a); } }
#define D_DB(a)		{ if(DEBUG) { DB(a); } }  
#define D_DU(a,b) { if(DEBUG) { DU(a,b); } }

//

/*********************** Power Saving Functions *******************************/
#include <TurnOffFunctions.h>
#define REDUCED_FUNCTION_OPTIMIZATION		__attribute__((optimize("-O1")))

/*********************** Global Helper Functions *******************************/

#define	ULONG_MAX	4294967295UL 	/* max value of "unsigned long int" */

#define HexStr2uint8(x) ((uint8_t) (HexChar2uint8((x)[0])*16 + HexChar2uint8((x)[1])))
#define HexStr2uint16(x) ((uint16_t) ( (HexStr2uint8(x) << 8) + (HexStr2uint8(x+2)) ) )
uint8_t HexChar2uint8(char data);
void shiftCharRight(byte *data, uint8_t len, uint8_t bytes);
void shiftCharLeft(byte *data, uint8_t start, uint8_t len, uint8_t bytes);

/*********************** Declare Global Module Funktions/Variables *************/
extern DataFIFO_t DataRing;
extern Print* pSerial;

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
extern myBaseModule *INT0Module;
extern myBaseModule *INT1Module;
extern myBaseModule *PinChgModule;
extern myBaseModule *PinChgModule1;

typedef struct {
    char typecode;
    const char* name;
    myBaseModule* module;
} typeModuleInfo;
extern const typeModuleInfo ModuleTab[];

#if HAS_AVR	&& defined(READVCC_CALIBRATION_CONST)
	#define VCC_SUPPLY_mV (myBaseModule::VCC_Supply_mV)
#else 
	#define VCC_SUPPLY_mV 3300
#endif

/********************************************************************************/
class myBaseModule {

protected:

#if HAS_POWER_OPTIMIZATIONS
	static unsigned long tIdleCycles; //static default value defined in .cpp
#endif

#if HAS_INFO_POLL
	static unsigned long tInfoPoll; //static default value defined in .cpp
	#define	INFO_POLL_PRESCALER_MAX		0xFF
	static byte preScaler;
#endif

#if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING==2 //Satellite
	static byte DisplayCopy;
	static char cDisplayCopy[MAX_RING_DATA_SIZE];
#endif
	//static bool UartActive;
	
	static char lastPrintChar; //save last printed character in "display_char". -->Always print \r\n at the end of lines
		
public:
	static byte QuietMode;
	
#if HAS_AVR	
	static unsigned int VCC_Supply_mV;
#endif

  myBaseModule() { 
  	FIFO_init(DataRing); 
  	TIMING::initialize(); 
  };
  //~BaseModule() { };

/*
void printRam() {
	extern int __heap_start, *__brkval;
	int v;
	Serial.println((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
}
*/

/**************************************************************/
/******************* Basic Module Functions *******************/
  virtual void send(char *, uint8_t) { };
  virtual bool poll(void) { return 0; };
  virtual void infoPoll(byte prescaler) { };
  virtual void initialize(void) { };
  virtual void displayData(RecvData *) { };
  virtual bool validdisplayData(RecvData *) { return 1; };
  
#if INCLUDE_HELP  
  virtual void printHelp() { };
#endif

#if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING==2 //Satellite
	static void setDisplayCopy(byte on, char c = -1) { //string is always terminated
		if(on) { 
			DisplayCopy=1;
			if(c==-1) {
				cDisplayCopy[0] = 0x0;
			} else {
				if(strlen(cDisplayCopy)>=(MAX_RING_DATA_SIZE-1)) { D_DS_P("DisplayTunnel string to big for variable."); return; }
				cDisplayCopy[strlen(cDisplayCopy)+1]=0x0;
				cDisplayCopy[strlen(cDisplayCopy)]=c;
			}
		} else {
			DisplayCopy=0;
		}
	}
	
	static char* getDisplayCopy() {
		return cDisplayCopy;
	}
#endif
	
#if HAS_POWER_OPTIMIZATIONS
	//check if idle time is over and FIFO is empty
	static uint8_t idleCycles(int8_t plus=0, unsigned long timeLimit=0) {
		if(plus==1) {
			tIdleCycles=TIMING::millis();
			return 0;
		} else {
			if(tIdleCycles==0) tIdleCycles=TIMING::millis()+1;
			//DS("idle ");Serial.print(TIMING::millis()-tIdleCycles);DNL();Serial.flush();
			return ( ( (TIMING::millis_since(tIdleCycles)>=timeLimit) && !FIFO_available(DataRing)) ? 1 : 0);
		}
		
	}
#endif

#if HAS_INFO_POLL && INFO_POLL_CYCLE_TIME
	static bool infoPollCycles(bool force=false, byte *ppreScaler=NULL) {
		const typeModuleInfo* pmt = ModuleTab;
		if(force || (TIMING::millis_since(tInfoPoll) >= (unsigned long)(INFO_POLL_CYCLE_TIME*1000UL))) {
 			if(!ppreScaler) {
	 			while(pmt->typecode >= 0) {
					pmt->module->infoPoll(preScaler);
					pmt++;
				}
			}
			*ppreScaler = preScaler;
			preScaler = (preScaler<INFO_POLL_PRESCALER_MAX ? preScaler+1 : 0);
			tInfoPoll=TIMING::millis();
			return true;
		}
		return false;
	}
#endif	
	
	// Append a new data item to the outgoing packet buffer (if there is room)
	static void addToRingBuffer (byte module, byte code, const byte* buf, byte len) {
	
		RecvData rBuf;

		if(len >= MAX_RING_DATA_SIZE) {  //immer 1 zeichen für die Terminierung sicher stellen
			/*if(DEBUG) */ { 
				DS_P("Ringbuffer overrun <");  
				for(uint8_t c=0; c<=len; c++) { DH2(buf[c]); DC(' '); } DS_P("|"); 
				DH2(module);DS_P("|"); 
				DU(code,0);DS_P("|"); 
				DU(len,0); DS_P(">\n"); 
			}
			len = MAX_RING_DATA_SIZE-1;
		}
		rBuf.ModuleID = module;
		rBuf.DataTypeCode = code;
		if(buf!=NULL && len!=0)
			memcpy(rBuf.Data,buf,len); 
		rBuf.DataSize = len;
		rBuf.Data[len] = 0x0; //Terminierung; +1 ist sicher gestellt durch len check
		
		DataFIFO_write(DataRing,rBuf);
	}

#if defined(STORE_CONFIGURATION)
	static void getStoredValue(void *value, const void *p, size_t len) {
		eeprom_read_block(value, p, len);
	}
#else
	#define getStoredValue
#endif	

#if defined(STORE_CONFIGURATION)
	static void StoreValue(void *value, void *p, size_t len) {
		eeprom_write_block(value, p, len);
	}
#else
	#define StoreValue
#endif	

/***********************************************************/
/******************* Interrupt Functions *******************/
  virtual void interrupt() { };
  static void INT0_interrupt();
  static void INT1_interrupt();
  static void PinChg_interrupt();
	static void PinChg_interrupt1();
	static uint8_t PinChg_irqPin;

	/*inline*/static void cfgInterrupt(myBaseModule *module, uint8_t irqPin, byte state) {

		switch(irqPin) {
			case INTZero:
				if(state==Interrupt_Off) {
					detachInterrupt(0); 
					INT0Module= NULL;
				
				} else if(state==Interrupt_Block) {
					bitClear(EIMSK, INT0);
					if(module!=NULL) INT0Module= NULL;	
					
				} else if(state==Interrupt_Release) {	
					if(module!=NULL) INT0Module = module;			
					if(INT0Module!=NULL) bitSet(EIMSK, INT0); //sicher gehen das Interrupt überhaut aktiviert wurde

				} else {
					INT0Module = module;
					detachInterrupt(0); //be sure that nobody else already attached the interrupt
					attachInterrupt(0, INT0_interrupt, (state==Interrupt_Low ? LOW : state==Interrupt_High ? HIGH : state==Interrupt_Falling ? FALLING : state==Interrupt_Rising ? RISING : CHANGE));				

				}
			break;
			
			case INTOne:
				if(state==Interrupt_Off) {
					detachInterrupt(1); 
					INT1Module = NULL;

				} else if(state==Interrupt_Block) {
					bitClear(EIMSK, INT1);
					if(module!=NULL) INT1Module= NULL;	

				} else if(state==Interrupt_Release) {	
					if(module!=NULL) INT1Module = module;			
					if(INT1Module!=NULL) bitSet(EIMSK, INT1); //sicher gehen das Interrupt überhaut aktiviert wurde

				} else {
					INT1Module = module;
					detachInterrupt(1); //be sure that nobody else already attached the interrupt
					attachInterrupt(1, INT1_interrupt, (state==Interrupt_Low ? LOW : state==Interrupt_High ? HIGH : state==Interrupt_Falling ? FALLING : state==Interrupt_Rising ? RISING : CHANGE));				

				}
			break;			
			
			case 6:
				if (state==Interrupt_Off) {
					detachPinChangeInterrupt(irqPin);
					PinChgModule1 = NULL;
					
				} else if (state==Interrupt_Block) {
					if (irqPin < 8) {
						  bitClear(PCICR, PCIE2);
					} else if (irqPin < 14) {
						  bitClear(PCICR, PCIE0);
					} else {
						  bitClear(PCICR, PCIE1);
					} 				
					if(module!=NULL) PinChgModule1 = NULL;
			
				} else if (state==Interrupt_Release) {
					if(module!=NULL) PinChgModule1 = module;
					if(PinChgModule1!=NULL) {//sicher gehen das Interrupt überhaut aktiviert wurde
						if (irqPin < 8) {
								bitSet(PCICR, PCIE2);
						} else if (irqPin < 14) {
								bitSet(PCICR, PCIE0);
						} else {
								bitSet(PCICR, PCIE1);
						} 				
					}	
			
				} else {
					PinChgModule1 = module;
					detachPinChangeInterrupt(irqPin);
					attachPinChangeInterrupt(irqPin,PinChg_interrupt1,(state==Interrupt_Falling ? FALLING : state==Interrupt_Rising ? RISING : CHANGE));
				}				
				
			break;
							
			default://case PINCHG:
				if (state==Interrupt_Off) {
					detachPinChangeInterrupt(irqPin);
					PinChgModule = NULL;
					
				} else if (state==Interrupt_Block) {
					if (irqPin < 8) {
						  bitClear(PCICR, PCIE2);
					} else if (irqPin < 14) {
						  bitClear(PCICR, PCIE0);
					} else {
						  bitClear(PCICR, PCIE1);
					} 				
					if(module!=NULL) PinChgModule = NULL;
			
				} else if (state==Interrupt_Release) {
					if(module!=NULL) PinChgModule = module;
					if(PinChgModule!=NULL) {//sicher gehen das Interrupt überhaut aktiviert wurde
						if (irqPin < 8) {
								bitSet(PCICR, PCIE2);
						} else if (irqPin < 14) {
								bitSet(PCICR, PCIE0);
						} else {
								bitSet(PCICR, PCIE1);
						} 				
					}	
			
				} else {
					PinChgModule = module;
					detachPinChangeInterrupt(irqPin);
					attachPinChangeInterrupt(irqPin,PinChg_interrupt,(state==Interrupt_Falling ? FALLING : state==Interrupt_Rising ? RISING : CHANGE));
				}				
				
			break;
		}
	}


/*********************************************************/        
/******************* Display Functions *******************/
	static void display_flush() {
		if(isUartTxActive) {
			Serial.flush();
		}
	}

	static void display_char(char s) {
	#if HAS_UART
		#if HAS_POWER_OPTIMIZATIONS
			if(isUartTxActive) 
		#endif
			{
				if(lastPrintChar!='\r' && s=='\n')
					pSerial->print('\r');	
				pSerial->print(s);
				lastPrintChar=s;
			}
	#endif
#if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING==2 //Satellite
		if(DisplayCopy) {
			setDisplayCopy(1,s);
		}
	#endif
	}
	
	static void display_string(char *s) { 
		while(*s) display_char(*s++); 
	}
	
	static void display_string(const char *s) { 
			display_string((char*)s);
	}	
	
	static void display_string_P(const char *s) {
		uint8_t c;
		while((c = __LPM(s))) {
			display_char(c);
			s++;
		}
	}
	
	static void display_int(int16_t d, int8_t pad, uint8_t padc) {

 		char buf[7];
		int8_t i=7;
		bool negativ=false;
		
		buf[--i] = 0;
		if(d<0) {
			d *= -1;
			negativ=true;
		}
		do {
			buf[--i] = d%10 + '0';
			d /= 10;
			pad--;
		} while(d && i);
		if(negativ) buf[--i] = '-';
		while(--pad >= 0 && i > 0) 
			buf[--i] = padc;
		DS(buf+i);
	}
	
	static void display_udec(uint16_t d, int8_t pad, uint8_t padc) {
	
 		char buf[6];
		int8_t i=6;
		
		buf[--i] = 0;
		do {
			buf[--i] = d%10 + '0';
			d /= 10;
			pad--;
		} while(d && i);
		while(--pad >= 0 && i > 0) 
			buf[--i] = padc;
		DS(buf+i); 
	}

	static void display_float(float f, int8_t pad, uint8_t padc) {
	
		char buf[8]; //max -000,00
		int8_t i=8;
		int16_t d;
		bool negativ=false;
		d = (int16_t)(f*1000); //shift to 0000. (int16)
		d = ((d % 10)>=5) ? d/10+1 : d/10; //ggf. Aufrunden
		
		//identisch mit display_int(...)
		buf[--i] = 0;
		if(d<0) {
			d *= -1;
			negativ=true;
		}
		do {
			buf[--i] = d%10 + '0';
			d /= 10;
			pad--;
			if(i==5) buf[--i] = '.';
		} while(d && i);
		if(negativ) buf[--i] = '-';
		while(--pad >= 0 && i > 0) 
			buf[--i] = padc;
		DS(buf+i);		
	}
	
	static void display_hex(uint16_t h, int8_t pad, uint8_t padc) {
		
		char buf[5];
		int8_t i=5;
		
		buf[--i] = 0;
		do {
			uint8_t m = h%16;
			buf[--i] = (m < 10 ? '0'+m : 'A'+m-10);
			h /= 16;
			pad--;
		} while(h);
		while(--pad >= 0 && i > 0) 
			buf[--i] = padc;
		DS(buf+i); 
	}
	static void display_hex4(uint16_t h) { display_hex2(h>>8); display_hex2(h); };
	static void display_hex2(uint8_t h) { display_hex(h, 2, '0'); }
	static void display_nl(void) { display_string("\r\n"); }
	
	static void display_bin(byte data) {
		//char bits[9];
	
	  // Extract the bits
		for (int8_t i = 7; i >= 0; i--) {
    	// Mask each bit in the byte and store it
    	//bits[i] = (data & (0x01 << i)) == 0 ? '0' : '1';
    	DC((data & (0x01 << i)) == 0 ? '0' : '1');
		}
		//bits[8]=0x0;
		//DS(bits);
	}
};

#endif
