
#include <myUart.h>

Print* pSerial; //extern Print* pSerial;

#if (defined(SOFTWARE_UART) && defined(SW_UART_RX_PIN) && defined(SW_UART_TX_PIN))
	SoftwareSerial SWUart(SW_UART_RX_PIN, SW_UART_TX_PIN); // RX, TX4
	#define SWUART
#endif

myUart::myUart(long baudrate, byte type) {

	indexBuffer = 0;
	hexInterpreter=0;
#ifdef SWUART
	Type = type;
#endif
	BaudRate=baudrate;
}

void myUart::initialize() {

#ifdef SWUART
	if(Type == HW_UART) {
#endif
		PRINT_TO_SERIAL.begin(BaudRate);
		pSerial = &PRINT_TO_SERIAL;
		while(!PRINT_TO_SERIAL); // wait till connection is established
		PRINT_TO_SERIAL.flush();
#ifdef SWUART
	} else if(Type == SW_UART) {
		SWUart.begin(BaudRate);
		pSerial = &SWUart;
	}
#endif
	memset(UartBuffer,0,sizeof(UartBuffer));
	indexBuffer = 0;
	hexInterpreter=0;
}

bool myUart::poll() {

	char inByte = 0;

#ifdef SWUART
	switch(Type) {
		case HW_UART:
#endif
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
#ifdef SWUART
	#error kein hexInterpreter integriert
			break;
		case SW_UART:
			SWUart.flush();
			if (SWUart.available() > 0) {
				inByte = SWUart.read();

				UartBuffer[indexBuffer] = inByte;
				if (inByte != '\n' && inByte != '\r') {           // no linefeed or CR so keep reading
					indexBuffer++;
					if (indexBuffer >= MAX_UART_BUFFER) indexBuffer = MAX_UART_BUFFER -1;  // Buffer overflow !
				} else {
					UartBuffer[indexBuffer] = '\0';
					indexBuffer = 0;
					if(!strlen(UartBuffer)) return 0;

					addToRingBuffer(MODULE_DATAPROCESSING, MODULE_DATAPROCESSING_DEVICEID, (const byte*)UartBuffer, strlen(UartBuffer));

				}
				return 1;
			}
		break;
	}
#endif
	return 0;
}


void myUart::send(char *cmd, uint8_t typecode) {
	PRINT_TO_SERIAL.flush();
#ifdef SWUART
	SWUart.flush();
#endif
}

/*
// Function to return a substring defined by a delimiter at an index
char* myUart::subStr(char* str, char *delim, int index) {
   char *act, *sub, *ptr;
   static char copy[MAX_UART_BUFFER];
   int i;
	 sub = NULL;

   // Since strtok consumes the first arg, make a copy
   strncpy(copy, str,MAX_UART_BUFFER);

   for (i = 1, act = copy; i <= index; i++, act = NULL) {
      sub = strtok_r(act, delim, &ptr);
      if (sub == NULL) break;
   }
   return sub;

}
*/
