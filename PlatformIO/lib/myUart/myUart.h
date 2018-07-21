
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

	void initialize();
  void send(char *cmd, uint8_t typecode=0);
  bool poll();

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
