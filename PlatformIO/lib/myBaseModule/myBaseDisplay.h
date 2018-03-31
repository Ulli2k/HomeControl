
#ifndef _MY_DISPLAY_
#define _MY_DISPLAY_

#define byte uint8_t

#include <RingBuffer.h> //MAX_RING_DATA_SIZE

extern Print* pSerial;

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


#if defined(__AVR_ATmega328P__)
	#define isUartTxActive  	(UCSR0B & (1<<TXEN0))
#else
	#define isUartTxActive		1
#endif

class myDisplay {

protected:
    #if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING==2 //Satellite
    	static byte DisplayCopy;
    	static char cDisplayCopy[MAX_RING_DATA_SIZE];
    #endif

	  static char lastPrintChar; //save last printed character in "display_char". -->Always print \r\n at the end of lines

public:
	myDisplay() { };
	static byte QuietMode;

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
#if defined(__AVR_ATmega328P__)
  		while((c = __LPM(s))) {
#else
			while(c = (uint8_t)s[0]) {
#endif
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
