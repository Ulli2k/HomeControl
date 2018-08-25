
#ifndef _MY_DISPLAY_
#define _MY_DISPLAY_

/******** DEFINE dependencies ******
	INCLUDE_DEBUG_OUTPUT: add debug commands to built
	QUIETMODE_DEFAULT_VALUE: activates quite mode after startup
	HAS_DISPLAY_TUNNELING: Based on HAS_RADIO && HAS_RADIO_CMD_TUNNELING to add DisplayCopy function
	HAS_UART: activate low level print char function
	HAS_POWER_OPTIMIZATIONS: allowes to turn of UART (isUartTxActive)
************************************/

#define byte uint8_t

#include <Arduino.h>
#include <../myFunctions/libRingBuffer.h> //MAX_RING_DATA_SIZE

#if defined(__AVR_ATmega328P__)
	#define PRINT_TO_SERIAL     Serial
	#define isUartTxActive  		(UCSR0B & (1<<TXEN0))
	#define UART_RX							0
	#define UART_TX							1
 #else
	#define PRINT_TO_SERIAL     Serial1
	#define isUartTxActive			(PM->APBCMASK.reg & PM_APBCMASK_SERCOM0)
	#define UART_RX							PIN_SERIAL1_RX
	#define UART_TX							PIN_SERIAL1_TX
#endif

#define DC 								myDisplay::display_char
#define DS 								myDisplay::display_string
#define DS_P(a)						myDisplay::display_string_P(PSTR(a))
#define DU(a,b) 					myDisplay::display_udec(a,b,'0')
#define DI(a,b) 					myDisplay::display_int(a,b,'0')
#define DF(a,b) 					myDisplay::display_float(a,b,'0')
#define DH(a,b) 					myDisplay::display_hex(a,b,'0')
#define DH2(a) 						myDisplay::display_hex2(a)
#define DH4(a) 						myDisplay::display_hex4(a)
#define DB(a)							myDisplay::display_bin(a)
#define DNL 							myDisplay::display_nl
#define DFL								myDisplay::display_flush

#if INCLUDE_DEBUG_OUTPUT
	#define DEBUG						(!myDisplay::QuietMode)
#else
	#define DEBUG						0
#endif

#define DEBUG_ONOFF(x) 		(myDisplay::QuietMode = (x))
#define D_DS_P(a) 				{ if(DEBUG) { DS_P(a); } }
#define D_DC(a)						{ if(DEBUG) { DC(a); } }
#define D_DS(a) 					{ if(DEBUG) { DS(a); } }
#define D_DI(a,b) 				{ if(DEBUG) { DI(a,b); } }
#define D_DH2(a) 					{ if(DEBUG) { DH2(a); } }
#define D_DB(a)						{ if(DEBUG) { DB(a); } }
#define D_DU(a,b) 				{ if(DEBUG) { DU(a,b); } }

#ifndef QUIETMODE_DEFAULT_VALUE
	#define QUIETMODE_DEFAULT_VALUE		1
#endif

#if HAS_RADIO && HAS_RADIO_CMD_TUNNELING==2 //Satellite
	#define HAS_DISPLAY_TUNNELING
#endif

#define ASSERT(cond)			{ if(!(cond)) myDisplay::hal_failed(__FILE__, __LINE__); }

class myDisplay {

protected:
    #ifdef HAS_DISPLAY_TUNNELING
    	static byte DisplayCopy;
    	static char cDisplayCopy[MAX_RING_DATA_SIZE];
    #endif

	  static char lastPrintChar; //save last printed character in "display_char". -->Always print \r\n at the end of lines

public:
	myDisplay() { };
	static byte QuietMode;

	/*********************************************************/
  /******************* Tunneling Functions *******************/
  #ifdef HAS_DISPLAY_TUNNELING
  	static void setDisplayCopy(byte on, int8_t c = -1) { //string is always terminated
  		if(on) {
  			DisplayCopy=1;
  			if(c==-1) {
  				cDisplayCopy[0] = 0x0;
					// memset(cDisplayCopy,0,MAX_RING_DATA_SIZE);
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

	/*********************************************************/
  /******************* Failure Functions *******************/
	static void hal_failed (const char *file, uint16_t line) {
		DS("FAILURE <");
		DS(file);DC(':');DU(line,0);DS(">");DNL();
		DFL();DFL();
		noInterrupts();
		while(1);
	}

  /*********************************************************/
  /******************* Display Functions *******************/
  	static void display_flush() {
  		if(isUartTxActive) {
				PRINT_TO_SERIAL.flush();
  		}
  	}

  	static void display_char(char s) {
  	#if HAS_UART
  		#if HAS_POWER_OPTIMIZATIONS
  			if(isUartTxActive)
  		#endif
  			{
  				if(lastPrintChar!='\r' && s=='\n')
  					PRINT_TO_SERIAL.print('\r');
  				PRINT_TO_SERIAL.print(s);
  				lastPrintChar=s;
  			}
  	#endif
  #ifdef HAS_DISPLAY_TUNNELING
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
			#if defined(__AVR_ATmega328P__)
				char c;
	  		while((c = __LPM(s))) {
					display_char(c);
	  			s++;
	  		}
			#else
				display_string(s);
			#endif
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
  	static void display_hex4(uint16_t h)   { display_hex2(h>>8); display_hex2(h); };
  	static void display_hex2(uint8_t h)    { display_hex(h, 2, '0'); }
  	static void display_nl(void)           { display_string("\r\n"); }

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