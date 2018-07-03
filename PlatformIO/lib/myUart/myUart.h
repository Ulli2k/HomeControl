
#ifndef MYDISPLAY_h
#define MYDISPLAY_h

#include "myBaseModule.h"

#include "Print.h"
//#include <SoftwareSerial.h>

#define HW_UART			0
#define SW_UART			1

#define MAX_UART_BUFFER  			(MAX_RING_DATA_SIZE) // Achtung auf Ringbuffergröße
#define UART_HEX_INTERPRETER	'#'

class myUart : public myBaseModule {

public:
  myUart(long baudrate, byte type);
	//~mySerial() {};

	void initialize();
  void send(char *cmd, uint8_t typecode=0);
  bool poll();
  //void displayData(RecvData *DataBuffer);

	//static char* subStr(char* str, char *delim, int index);

	// Wrap Serial Functions to myRemote Class
/*	size_t println(const __FlashStringHelper *ifsh) { return pSerial->println(ifsh); }
	size_t println(void) { return pSerial->println(); }
	size_t println(const String &s) { return pSerial->println(s); }
	size_t println(const char c[]) { return pSerial->println(c); }
	size_t println(char c) { return pSerial->println(c); }
	size_t println(unsigned char b, int base = DEC) { return pSerial->println(b,base); }
	size_t println(int num, int base = DEC) { return pSerial->println(num,base); }
	size_t println(unsigned int num, int base = DEC) { return pSerial->println(num, base); }
	size_t println(long num, int base = DEC) { return pSerial->println(num,base); }
	size_t println(unsigned long num, int base = DEC) { return pSerial->println(num,base); }
	size_t println(double num, int digits = DEC) { return pSerial->println(num, digits); }
	size_t println(const Printable& x) { return pSerial->println(x); }

	size_t print(const __FlashStringHelper *ifsh) { return pSerial->print(ifsh); }
	size_t print(const String &s) { return pSerial->print(s); }
	size_t print(const char str[]) { return pSerial->print(str); }
	size_t print(char c) { return pSerial->print(c); }
	size_t print(unsigned char b, int base = DEC) { return pSerial->print(b, base); }
	size_t print(int n, int base = DEC) { return pSerial->print(n, base); }
	size_t print(unsigned int n, int base = DEC) { return pSerial->print( n, base); }
	size_t print(long n, int base = DEC) { return pSerial->print(n,base); }
	size_t print(unsigned long n, int base = DEC) { return pSerial->print(n,base); }
	size_t print(double n, int digits = DEC) { return pSerial->print(n, digits); }
	size_t print(const Printable& x) { return pSerial->print(x); }
*/
	size_t print(char c) { return pSerial->print(c); }
	static void flush() { PRINT_TO_SERIAL.flush(); }

private:
//	Print* pSerial; //global initialize in ino Project file
#ifdef SWUART
	byte Type;
#endif
	long BaudRate;

	uint8_t indexBuffer;
	char UartBuffer[MAX_UART_BUFFER];

	uint8_t hexInterpreter;
};

#endif
