
#ifndef MY_UART_h
#define MY_UART_h

#include <myBaseModule.h>
#include <Print.h>

#define MAX_UART_BUFFER  			(MAX_RING_DATA_SIZE) // Achtung auf Ringbuffergröße
#define UART_HEX_INTERPRETER	'#'

#ifndef PRINT_TO_SERIAL
	#define PRINT_TO_SERIAL     Serial
#endif

Print* pSerial;

class myUart : public myBaseModule {

public:

  myUart(long baudrate) {
    indexBuffer = 0;
  	hexInterpreter=0;
  	BaudRate=baudrate;
  }

	void initialize() {
    PRINT_TO_SERIAL.begin(BaudRate);
  	pSerial = &PRINT_TO_SERIAL;
  	while(!PRINT_TO_SERIAL); // wait till connection is established
  	PRINT_TO_SERIAL.flush();

  	memset(UartBuffer,0,sizeof(UartBuffer));
  	indexBuffer = 0;
  	hexInterpreter=0;
  }

  bool poll() {
    char inByte = 0;

  	PRINT_TO_SERIAL.flush();
  	//if(indexBuffer) { UartBuffer[indexBuffer] = '\0'; DS("<<UartBuffer: ");DU(indexBuffer,0);DS("<");DS(UartBuffer);DS(">\n");; }
  	if (PRINT_TO_SERIAL.available() > 0) {
  		inByte = PRINT_TO_SERIAL.read();
  		//Invalid Bytes
  		if(!hexInterpreter && (!(inByte=='\r' || inByte=='\n' || (inByte>=0x20 && inByte<=0x7E) )) ) {
  			indexBuffer = 0;
  			hexInterpreter = 0;
  			return 1; // skip if there is an invalid character
  		}


  		if ((!hexInterpreter && (inByte != '\n' && inByte != '\r' && inByte != '#')) || (hexInterpreter && inByte != '#')) {           // no linefeed or CR so keep reading
  			if(hexInterpreter==2) {
  				UartBuffer[indexBuffer-1] = (HexChar2uint8(UartBuffer[indexBuffer-1]) << 4) | (HexChar2uint8(inByte));
  				hexInterpreter=1;
  			} else {
  				UartBuffer[indexBuffer] = inByte;
  				indexBuffer++;
  				if (indexBuffer >= MAX_UART_BUFFER) indexBuffer = MAX_UART_BUFFER -1;  // Buffer overflow !
  				if(hexInterpreter) hexInterpreter++;
  			}

  		} else if (inByte == '#') { //Hex interpreter
  			if(hexInterpreter) {
  				hexInterpreter = 0;
  			} else {
  				hexInterpreter = 1;
  			}
  		} else {
  			UartBuffer[indexBuffer] = '\0';
  			hexInterpreter = 0;
  			if(!indexBuffer) return 0;
  			addToRingBuffer(MODULE_DATAPROCESSING, MODULE_DATAPROCESSING_DEVICEID, (const byte*)UartBuffer, indexBuffer);

  // PRINT_TO_SERIAL.print("UARTData: ");
  // for (uint8_t l = 0; l < indexBuffer ; l++) {
  // 	DH2(UartBuffer[l]);
  // }
  // PRINT_TO_SERIAL.println();

  			indexBuffer = 0;
  		}
  		return 1;
  	}
  	return 0;
  }

  void send(char *cmd, uint8_t typecode=0) {
	   PRINT_TO_SERIAL.flush();
  }

	size_t print(char c) { return pSerial->print(c); }
	static void flush() { PRINT_TO_SERIAL.flush(); }

private:
	long BaudRate;

	uint8_t indexBuffer;
	char UartBuffer[MAX_UART_BUFFER];

	uint8_t hexInterpreter;
};

#endif
