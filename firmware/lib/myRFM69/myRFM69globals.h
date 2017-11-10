
#ifndef _MY_RFM69_GLOBALS_h
#define _MY_RFM69_GLOBALS_h

#define RF69_MAX_DATA_LEN         61 // to take advantage of the built in AES/CRC we want to limit the frame size to the internal FIFO size (66 bytes - 3 bytes overhead)

//Package Configuration (1.Byte)
#define XDATA_NOTHING							0x0000	//plain TX mode
#define XDATA_BURST								(1<<0) 	//Funktions not supported: Encryption, Manchester, DataWhitening, CRC
																				 	//In Burst mode also SYNC and Preambles if activated, will be handled/added by the send() function
																				 	//Burst is possible for a specific time (XDATA_BurstTime) or number of messages (XDATA_Repeats)
#define XDATA_BURST_INFO_APPENDIX	(1<<1)	//Adds rest of BurstTime at the end of the Payload (2bytes), used for myProtocol

#define XDATA_ACK_REQUEST					(1<<4)
#define XDATA_ACK_RECEIVED				(1<<5)
#define XDATA_CMD_REQUEST					(1<<6)
#define XDATA_CMD_RECEIVED				(1<<7)
//define XDATA_CRC								bit	<-- nicht benötigt da CRC(ON/OFF) immer für RX und TX gleich ist

//Internal Configuration (2.Byte) for TX/RX behavior
#define XDATA_CMD_ECHO						(1<<8) 	//Prints the TX command after transmission to the UART																											 	
#define XDATA_SYNC								(1<<9) 	//Sync words can be deaktived in TX mode or will be added in Burst mode after each DataFrame
#define XDATA_PREAMBLE						(1<<10)	//Preambles can be deaktived in TX mode or will be added in Burst mode after each DataFrame
#define XDATA_BURST_INFINITY			(1<<11) //In TX mode: Pushes continuously data into FIFO. No wait for TX done. Just checks the FIFO level before pushing more data


typedef struct { //ist ebenso in myRFM69protocols.h definiert!
    volatile byte DATA[RF69_MAX_DATA_LEN];          // recv/xmit buf, including hdr & crc bytes
		volatile byte PAYLOADLEN;
    volatile int RSSI; 				//most accurate RSSI during reception (closest to the reception)	
    
    byte CheckSum; 						//Debouncing: checksum of message
    //uint8_t MsgCnt; 				//Debouncing: message counter
    byte SENDERID;
    byte TARGETID;

   	uint16_t XDATA_Repeats;		//number of transmission repeats
   	uint16_t XDATA_BurstTime;	//time of continues transmission

    uint16_t XData_Config;				//TX+RX configuration
} myRFM69_DATA;

/*
HX2262: SyncOff + TXRepeats, 																					(TX&RX aber beide kein CRC, kein Preamble)
ETH200: 					TXRepeats + CMD_ECHO 																(TX&RX beide kein Sync, kein CRC, kein Preamble)
myProtocol:
	Simple:SyncOn +										 		+ ACK+CMD Request/Received +	Preamble + CRC (ohne TXRepeats)
	Burst: SyncOff+ TXRepeats							+ ACK+CMD Request/Received
FS20: kein TX
LaCrosse: kein TX
*/
#endif
