#ifndef _MY_FIFO_H_
#define _MY_FIFO_H_

#include <stdint.h>
#include <Arduino.h> //type byte

// Vorsicht der Buffer benötigt viel RAM
#if defined(__AVR_ATmega328P__)
  #define MAX_RING_BUFFER 				8  // muss 2^n betragen (4, 8, 16, 32, 64 ...)
																	   // >8 notwendig für z.B: 4xRFM-Module da Querys wie "00f0" erst einmal alles in den RingBuffer lädt
#else
  #define MAX_RING_BUFFER 				32
#endif
#define MAX_RING_DATA_SIZE			30 // IR: irmp_data struct muss reinpassen | Homematic: Payload 30 Bytes

typedef struct {
  byte ModuleID; // Uart, RFM12, IR
  byte DataTypeCode; // RFM: Decoder/Encloder Type ID
  byte Data[MAX_RING_DATA_SIZE];  // RFM: 25 , Uart: 25(Vorsicht \0 -->+1), IR: 15
  uint8_t DataSize;
} RecvData;

typedef struct {
	volatile uint8_t _ready;
	volatile uint8_t _read;
	volatile uint8_t _write;
	RecvData _buffer[MAX_RING_BUFFER];
} DataFIFO_t;

/* memset(&(fifo),0,sizeof(&(fifo)));*/
#define FIFO_init(fifo)		{ /*memset(&(fifo),0,sizeof(&(fifo)));*/ fifo._read = 0; fifo._write = 0; fifo._ready = 1; }

#define FIFO_available(fifo)	( fifo._read != fifo._write )

#define FIFO_ready(fifo) ( fifo._ready == 1 )

#define FIFO_block(fifo) ( fifo._ready = 0 )
#define FIFO_release(fifo) ( fifo._ready = 1 )

#define FIFO_read(fifo, size) (						\
	(FIFO_available(fifo)) ?					\
	&(fifo._buffer[fifo._read = (fifo._read + 1) & (size-1)]) : NULL	\
)

#define FIFO_write(fifo, data, size) {								\
	while(!FIFO_ready(fifo)); \
	FIFO_block(fifo); \
	uint8_t tmphead = ( fifo._write + 1 ) & (size-1); /* calculate buffer index */	\
	if(tmphead != fifo._read) {				/* if buffer is not full */	\
		fifo._write = tmphead;				/* store new index */		\
		memcpy(&fifo._buffer[tmphead],&data,sizeof(RecvData)); /* store data in buffer */	\
	} else { \
		Serial.println(PSTR("FIFO!")); \
	} \
	FIFO_release(fifo); \
}


#define DataFIFO_read(fifo)			FIFO_read(fifo, MAX_RING_BUFFER)
#define DataFIFO_write(fifo, data)	FIFO_write(fifo, data, MAX_RING_BUFFER)

/*
#define FIFO16_read(fifo)			FIFO_read(fifo, 16)
#define FIFO16_write(fifo, data)		FIFO_write(fifo, data, 16)

#define FIFO64_read(fifo)			FIFO_read(fifo, 64)
#define FIFO64_write(fifo, data)		FIFO_write(fifo, data, 64)

#define FIFO128_read(fifo)			FIFO_read(fifo, 128)
#define FIFO128_write(fifo, data)		FIFO_write(fifo, data, 128)
*/

#endif /*FIFO_H_*/
