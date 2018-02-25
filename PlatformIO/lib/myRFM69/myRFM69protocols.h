
#ifndef _MY_RFM69_PROTOCOLS_h
#define _MY_RFM69_PROTOCOLS_h

//#include <Arduino.h>
#include <RFM69registers.h>
#include <myBaseModule.h>
#include <myRFM69globals.h>

//#define RFM69_NO_OTHER_PROTOCOLS
#define RFM69_DEFAULT_PROTOCOL		RFM69_PROTOCOL_MyProtocol

//enum RFM69SendType { LargePackage, FixPackage };
typedef const struct ProtocolInfos_ {
    const unsigned char (*config)[2];
    uint8_t 						PayloadLength;
    bool 								(*transformRxData)	(myRFM69_DATA *);
    void 								(*transformTxData)	(char *, myRFM69_DATA *);
} ProtocolInfos;

/*********************************************************************************************/
/****************************** DATA Transform Functions *************************************/
/*********************************************************************************************/
//Protocol DataOptions
/*
#define RF69_DataOption_nothing				0
#define RF69_DataOption_sendACK				(1<<0)
#define RF69_DataOption_requestACK		(1<<1)
#define RF69_DataOption_sendCMD				(1<<2)
#define RF69_DataOption_requestCMD		(1<<3)
#define RF69_getModeDataOptionChar(x) ( (x & RF69_DataOption_sendACK) ? 'a' : (x & RF69_DataOption_requestACK) ? 'A' : (x & RF69_DataOption_requestCMD) ? 'C' : (x & RF69_DataOption_sendCMD) ? 'c' : '0' )
#define RF69_getModeDataOptionNum(x) ( (x=='a') ? RF69_DataOption_sendACK : (x=='A') ? RF69_DataOption_requestACK : (x=='C') ? RF69_DataOption_requestCMD : (x=='c') ? RF69_DataOption_sendCMD : RF69_DataOption_nothing )
*/

// RECEIVE
bool transformRxData_MyProtocol (myRFM69_DATA *sData);
#if !defined(RFM69_NO_OTHER_PROTOCOLS)
bool transformRxData_HX2262			(myRFM69_DATA *sData);
bool transformRxData_LaCrosse		(myRFM69_DATA *sData);
bool transformRxData_FS20				(myRFM69_DATA *sData);
bool transformRxData_HomeMatic	(myRFM69_DATA *sData);
#endif

// TRANSFER
void transformTxData_MyProtocol (char *cmd, myRFM69_DATA *sData);
#if !defined(RFM69_NO_OTHER_PROTOCOLS)
void transformTxData_HX2262			(char *cmd, myRFM69_DATA *sData);
void transformTxData_ETH				(char *cmd, myRFM69_DATA *sData);
void transformTxData_HomeMatic	(char *cmd, myRFM69_DATA *sData);
#endif

/**************************** HELPER Functions **************************/
void reverseBits(byte *data, uint8_t len);
uint8_t _crc8(volatile uint8_t *data, uint8_t len);
uint16_t calcCRC16r( uint16_t c,uint16_t crc, uint16_t mask);
uint16_t calcCRC16hm (uint8_t *data, uint8_t length);
void xOr_PN9(uint8_t *buf, uint8_t len);
int parity(unsigned char x);

/*********************************************************************************************/
/****************************** CONFIGs for Protocols ****************************************/
/*********************************************************************************************/
//Payload lenght for TX AND RX!
#define RFM69_PROTOCOL_MyProtocol										0x01
	#define RFM69_PROTOCOL_MYPROTOCOL_PAYLOADLENGTH		  66 //max length --> RF69_MAX_DATA_LEN=61
#define RFM69_PROTOCOL_HX2262 											0x02 //ID must match to ProtocolInfo Struct id/line
	#define RFM69_PROTOCOL_HX2262_PAYLOADLENGTH		 			12+4 //+4 for sync in TX mode
#define RFM69_PROTOCOL_FS20													0x03
	#define RFM69_PROTOCOL_FS20_PAYLOADLENGTH 					30
#define RFM69_PROTOCOL_LaCrosse 										0x04
	#define RFM69_PROTOCOL_LACROSSE_PAYLOADLENGTH			 	 5
#define RFM69_PROTOCOL_ETH			 										0x05
	#define RFM69_PROTOCOL_ETH_PAYLOADLENGTH		 		 		 0 //Muss =0,da TX LargeFrame!
	#define RFM69_PROTOCOL_ETH_REPEATS								 320
#define RFM69_PROTOCOL_HomeMatic										0x06
	#define RFM69_PROTOCOL_HOMEMATIC_PAYLOADLENGTH		  30+1+2 // Standard=30; HAS_FUP=50 (Firmware Update Tool) | +1 (LengthByte) | +2 (CRC16)
//ACHTUNG Maximale Anzahl an Protokolle durch Datengröße von RF69_Config.Protokoll:3 begrenzt!

	
// derzeit darf es maximal 7 protocolle geben aufgrund der variablen Speicher größe
	
#if !defined(RFM69_NO_OTHER_PROTOCOLS)
/***********************************************************************/
/************************** OOK 868MHz - FS20 **************************/
/***********************************************************************/
/*
		http://fhz4linux.info/tiki-index.php?page=FS20+Protocol
			bit 0: 400µs High, 400µs Low 600-1000µs Periodendauer
			bit 1: 600µs High, 600µs Low 1000-1450µs Periodendauer	
			--> Bitrate 5 kbits/s für ein kleinstes gemeinsames Vielfaches
			
		 -> bei 200µs Bitrate sind es 4/6 bits pro eigentliches bit 0 -> 1100 und 1 -> 111000
				length worst case 5bytes(Daten)*8bits *6bits
				--> 30 bytes max length

		Daten: 5/6 Bytes (aktuell aber nur 5 Bytes für Standard protocol)
		Hauscode	16 bit
		Adresse	8 bit
		Befehl	8 bit (16bit, wenn im ersten Befehlsbyte das Erweiterungsbit gesetzt ist.)
		Quersumme	8 bit

		TX: derzeit nicht unterstützt
*/
  const byte RFM69_CONFIG_FS20[][2] =
  {
  	//* 0x00 */ Fifo
		/* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY }, //Sequencer on | Standby Mode
  	/* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_OOK | RF_DATAMODUL_MODULATIONSHAPING_00 }, //no shaping
 		/* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_5000},
 		/* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_5000},
    
    //* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_90000}, //default:5khz, (FDEV + BitRate/2 <= 500Khz) /FDEV = frequency deviation
    //* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_90000}, // rfm12 90kHz Deviation -> (90kHz + 17,24kbps/2) <= 500kHz i.O.
    
   	/* 0x07 */ { REG_FRFMSB, 0xD9 }, //FRF_MSB }, //868,345 MHz aus Versuchen mit FS20
   	/* 0x08 */ { REG_FRFMID, 0x16 }, //FRF_MID },
   	/* 0x09 */ { REG_FRFLSB, 0x14 }, //FRF_LSB },    
        
    //* 0x0A */ calibration of the RC oscillator, trigger and read
		//* 0x0B */ { REG_AFCCTRL, RF_AFCCTL_LOWBETA_OFF }, // Improved AFC // needed when index<2  modulation index 0.5 <= beta = 2*Fdev/BitRate <=10 -> Standard AFC routine
		//* 0x0C */ unused
		//* 0x0D - 0x10 */ RegListen deaktivated
		
		/* Transmitter Registers */
		// looks like PA1 and PA2 are not implemented on RFM69W, hence the max output power is 13dBm
		// +17dBm and +20dBm are possible on RFM69HW
		// +13dBm formula: Pout=-18+OutputPower (with PA0 or PA1**)
		// +17dBm formula: Pout=-14+OutputPower (with PA1 and PA2)**
		// +20dBm formula: Pout=-11+OutputPower (with PA1 and PA2)** and high power PA settings (section 3.3.7 in datasheet)
		//* 0x11 */ { REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF_PALEVEL_OUTPUTPOWER_11111}, //set by setHighPower function
		//* 0x12 */ { REG_PARAMP, RF_PARAMP_40 }, //Rise/Fall time of ramp up/down in FSK
		//* 0x13 */ { REG_OCP, RF_OCP_ON | RF_OCP_TRIM_95 }, //over current protection (default is 95mA), set by setHighPower function

		
		/* Receiver Registers */
		//* 0x14 - 0x17 */ unused
		//* 0x18 */ { REG_LNA,  RF_LNA_ZIN_50| RF_LNA_GAINSELECT_AUTO }, // 200Ohm default / rfm12 MAX-LNA Gain setting
		/* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_20 | RF_RXBW_EXP_3 }, //(BitRate < 2 * RxBw) -> (17,24kbs < 2* rfm12 200 kHz OOK
		//* 0x1A */ RegAfcBw
		
		//RF_OOKPEAK_THRESHTYPE_PEAK | RF_OOKPEAK_PEAKTHRESHSTEP_001 | RF_OOKPEAK_PEAKTHRESHDEC_001
		//* 0x1B */ { REG_OOKPEAK, RF_OOKPEAK_THRESHTYPE_AVERAGE | RF_OOKPEAK_PEAKTHRESHSTEP_000 | RF_OOKPEAK_PEAKTHRESHDEC_000}, // OOK Data Mode
		/* 0x1C */ { REG_OOKAVG, RF_OOKAVG_AVERAGETHRESHFILT_10 }, //  OOK Data Mode
		//* 0x1D */ { REG_OOKFIX, RF_OOKFIX_FIXEDTHRESH_VALUE }, // OOK Data Mode		
		//* 0x1E */ { REG_AFCFEI, RF_AFCFEI_AFCAUTO_OFF | RF_AFCFEI_AFC_CLEAR | RF_AFCFEI_AFCAUTOCLEAR_ON}, // FSK ONLY
		//* 0x1E - 0x22 */ Afc Setting
		//* 0x23 0x24 */ RSSI 
		
		/* IRQ and Pin Mapping */
		/* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01 }, //DIO0 is the only IRQ we're using
		/* 0x26 */ { REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF }, //ClockOut
		//* 0x27 */ RegIrqFlags1 read only
		/* 0x28 */ { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN }, // Writing to this bit ensures the FIFO & status flags are reset
  	/* 0x29 */ { REG_RSSITHRESH, 0xE4 /*MAX*/ }, //(90*2) rfm -91dBm //must be set to dBm = (-Sensitivity / 2) - default is 0xE4=228 so -114dBm
		//* 0x2A-0x2B */ Timeout after switching to Rx mode if Rssi interrupt doesn’t occur
		
		/* Packet Engine Registers */
		/* 0x2C */ { REG_PREAMBLEMSB, 0 /*RF_PREAMBLESIZE_MSB_VALUE*/ },
		/* 0x2D */ { REG_PREAMBLELSB, 0 /*RF_PREAMBLESIZE_LSB_VALUE*/ }, // default 3 preamble bytes 0xAAAAAA
		/* 0x2E */ { REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_3 | RF_SYNC_TOL_0 },
    /* 0x2F */ { REG_SYNCVALUE1, 0x33 },
    /* 0x30 */ { REG_SYNCVALUE2, 0x33 }, 
    /* 0x30 */ { REG_SYNCVALUE3, 0x38 },
    //* 0x30 */ { REG_SYNCVALUE4, 0x33 },        
    //* 0x30 */ { REG_SYNCVALUE5, 0x33 },
    //* 0x30 */ { REG_SYNCVALUE6, 0x33 },    
    //* 0x30 */ { REG_SYNCVALUE7, 0x38 },       
    //* 0x31 - 0x36 */ possible SyncValues
    /* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_FIXED | RF_PACKET1_DCFREE_OFF |
    						 RF_PACKET1_CRC_OFF | RF_PACKET1_CRCAUTOCLEAR_OFF | 
    						 RF_PACKET1_ADRSFILTERING_OFF },
    /* 0x38 */ { REG_PAYLOADLENGTH, RFM69_PROTOCOL_FS20_PAYLOADLENGTH }, //max0x40 in variable length mode: the max frame size, not used in TX
    //* 0x39 */ { REG_NODEADRS, nodeID }, //turned off because we're not using address filtering  
  	//* 0x3A */ { REG_BROADCASTADRS, 0 }, //0 is the broadcast address
  	//* 0x3B */ {  REG_AUTOMODES, 0 }, // Automatic Modes, currently not needed
  	//* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE }, //TX on FIFO not empty
    /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, //RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    //* 0x3E - 0x4D */ AesKey
    
    /* Temperature Sensor Registers */
    /* 0x4E - 0x4F */
    
    /* Test Registers */
    /* 0x58 - 0x71 */
		//{ REG_TESTPA1, 0x5d }, //set by setHighPowerRegs
		//{ REG_TESTPA2, 0x7c }, //set by setHighPowerRegs    
    /* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 }, // run DAGC continuously in RX mode, recommended default for AfcLowBetaOn=0   
    {255, 0}
  };

/***************************************************************************/
/************************** OOK 433MHz - HX2262 **************************/
/***************************************************************************/  
/*
		http://homeeasyhacking.wikia.com/wiki/Simple_Protocol
			0: 320μs hight + 960μs low + 320μs hight + 960μs low
			1: 960μs hight + 375μs low + 960μs hight + 320μs low
			F (floating): 320μs hight + 960μs low + 960μs hight + 320μs low
			Sync: 320µs high + 9920µs low
			--> Bitrate 3,125 kbits/s für ein kleinstes gemeinsames Vielfaches
			--> bei 320µs Bitrate sind es 4 bits pro eigentliches bit 0 -> 1000 und 1 -> 1110
05R3s20FF0FFFF0F0F  (Don)
05R3s20FF0FFFF0FF0  (Doff)
05R3s20FF0FF0FFF0F		(Bon)
05R3s20FF0FF0FFFF0		(Boff)

		TX: 10 Widerholungen; Keine Preamble; SyncWord für RX Packet detection notwendig. SyncWord für TX abschalten.
				Repeats: 10
				NoSnyc
				NoContinues
*/		
  const byte RFM69_CONFIG_HX2262[][2] =
  {
  	//* 0x00 */ Fifo
		/* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY }, //Sequencer on | Standby Mode
  	/* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_OOK | RF_DATAMODUL_MODULATIONSHAPING_00 }, //no shaping
     		
 		/* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_3125 },//3125 /*2667*/}, //HX2272
 		/* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_3125 },// /*2667*/},
    
    //* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_90000}, //default:5khz, (FDEV + BitRate/2 <= 500Khz) /FDEV = frequency deviation
    //* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_90000}, // rfm12 90kHz Deviation -> (90kHz + 17,24kbps/2) <= 500kHz i.O.
    
   	/* 0x07 */ { REG_FRFMSB, 0x6C }, //FRF_MSB }, //433,92 MHz HX2272
   	/* 0x08 */ { REG_FRFMID, 0x7A }, //FRF_MID },
   	/* 0x09 */ { REG_FRFLSB, 0xE1 }, //FRF_LSB },    
        
    //* 0x0A */ calibration of the RC oscillator, trigger and read
		//* 0x0B */ { REG_AFCCTRL, RF_AFCCTL_LOWBETA_OFF }, // Improved AFC // needed when index<2  modulation index 0.5 <= beta = 2*Fdev/BitRate <=10 -> Standard AFC routine
		//* 0x0C */ unused
		//* 0x0D - 0x10 */ RegListen deaktivated
		
		/* Transmitter Registers */
		// looks like PA1 and PA2 are not implemented on RFM69W, hence the max output power is 13dBm
		// +17dBm and +20dBm are possible on RFM69HW
		// +13dBm formula: Pout=-18+OutputPower (with PA0 or PA1**)
		// +17dBm formula: Pout=-14+OutputPower (with PA1 and PA2)**
		// +20dBm formula: Pout=-11+OutputPower (with PA1 and PA2)** and high power PA settings (section 3.3.7 in datasheet)
		//* 0x11 */ { REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF_PALEVEL_OUTPUTPOWER_11111}, //set by setHighPower function
		//* 0x12 */ { REG_PARAMP, RF_PARAMP_40 }, //Rise/Fall time of ramp up/down in FSK
		//* 0x13 */ { REG_OCP, RF_OCP_ON | RF_OCP_TRIM_95 }, //over current protection (default is 95mA), set by setHighPower function

		
		/* Receiver Registers */
		//* 0x14 - 0x17 */ unused
		//* 0x18 */ { REG_LNA,  RF_LNA_ZIN_50| RF_LNA_GAINSELECT_AUTO }, // 200Ohm default / rfm12 MAX-LNA Gain setting
		/* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_20 | RF_RXBW_EXP_1 }, //(BitRate < 2 * RxBw) -> (17,24kbs < 2* rfm12 200 kHz OOK
		//* 0x1A */ RegAfcBw
		
		//RF_OOKPEAK_THRESHTYPE_PEAK | RF_OOKPEAK_PEAKTHRESHSTEP_001 | RF_OOKPEAK_PEAKTHRESHDEC_001
		//* 0x1B */ { REG_OOKPEAK, RF_OOKPEAK_THRESHTYPE_AVERAGE | RF_OOKPEAK_PEAKTHRESHSTEP_001 | RF_OOKPEAK_PEAKTHRESHDEC_001}, // OOK Data Mode
		/* 0x1C */ { REG_OOKAVG, RF_OOKAVG_AVERAGETHRESHFILT_10 }, //  OOK Data Mode
		//* 0x1D */ { REG_OOKFIX, RF_OOKFIX_FIXEDTHRESH_VALUE /*6dB*/ }, // OOK Data Mode		"Floor threshold" -> Rauschen
		//* 0x1E */ { REG_AFCFEI, RF_AFCFEI_AFCAUTO_OFF | RF_AFCFEI_AFC_CLEAR | RF_AFCFEI_AFCAUTOCLEAR_ON}, // FSK ONLY
		//* 0x1E - 0x22 */ Afc Setting
		//* 0x23 0x24 */ RSSI 
		
		/* IRQ and Pin Mapping */
		/* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01 }, //DIO0 is the only IRQ we're using
		/* 0x26 */ { REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF }, //ClockOut
		//* 0x27 */ RegIrqFlags1 read only
		/* 0x28 */ { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN }, // Writing to this bit ensures the FIFO & status flags are reset
  	/* 0x29 */ { REG_RSSITHRESH, 0xE4 /*0xE4*/ /*MAX*/ }, //(90*2) rfm -91dBm //must be set to dBm = (-Sensitivity / 2) - default is 0xE4=228 so -114dBm
		//* 0x2A-0x2B */ Timeout after switching to Rx mode if Rssi interrupt doesn’t occur
		
		/* Packet Engine Registers */
		/* 0x2C */ { REG_PREAMBLEMSB, 0/*RF_PREAMBLESIZE_MSB_VALUE*/ },
		/* 0x2D */ { REG_PREAMBLELSB, 0/*RF_PREAMBLESIZE_LSB_VALUE*/ }, // default 3 preamble bytes 0xAAAAAA
		/* 0x2E */ { REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_4 | RF_SYNC_TOL_0 },
		/* 0x2F */ { REG_SYNCVALUE1, 0x80 }, //<- Sync: 375µs high + 11625µs low
		/* 0x30 */ { REG_SYNCVALUE2, 0x00 }, 
		/* 0x31 */ { REG_SYNCVALUE3, 0x00 },
		/* 0x32 */ { REG_SYNCVALUE4, 0x00 },        
    //* 0x33 */ { REG_SYNCVALUE5, 0xEE },
    //* 0x34 */ { REG_SYNCVALUE6, 0x88 },    
    //* 0x35 */ { REG_SYNCVALUE7, 0x38 },
    //* 0x36 */ { REG_SYNCVALUE8, 0x38 },              
    //* 0x31 - 0x36 */ possible SyncValues
    /* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_FIXED | RF_PACKET1_DCFREE_OFF |
    						 RF_PACKET1_CRC_OFF | RF_PACKET1_CRCAUTOCLEAR_OFF | 
    						 RF_PACKET1_ADRSFILTERING_OFF },
		/* 0x38 */ { REG_PAYLOADLENGTH, RFM69_PROTOCOL_HX2262_PAYLOADLENGTH }, //max0x40 in variable length mode: the max frame size, not used in TX
    //* 0x39 */ { REG_NODEADRS, nodeID }, //turned off because we're not using address filtering  
  	//* 0x3A */ { REG_BROADCASTADRS, 0 }, //0 is the broadcast address
  	//* 0x3B */ {  REG_AUTOMODES, 0 }, // Automatic Modes, currently not needed
  	/* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE }, //TX on FIFO not empty
    /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, //RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    //* 0x3E - 0x4D */ AesKey
    
    /* Temperature Sensor Registers */
    /* 0x4E - 0x4F */
    
    /* Test Registers */
    /* 0x58 - 0x71 */
		//{ REG_TESTPA1, 0x5d }, //set by setHighPowerRegs
		//{ REG_TESTPA2, 0x7c }, //set by setHighPowerRegs
    //* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 }, // run DAGC continuously in RX mode, recommended default for AfcLowBetaOn=0   
    {255, 0}
  };
  
  
/***************************************************************************/
/************************** FSK 868MHz - LaCrosse **************************/
/***************************************************************************/

  const byte RFM69_CONFIG_LaCrosse[][2] =
  {
  	//* 0x00 */ Fifo
		/* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY }, //Sequencer on | Standby Mode
	  /* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00 }, //no shaping
	  /* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_17240}, //LaCrosse 17.24 kbps
    /* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_17240},
    
    /* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_90000}, //default:5khz, (FDEV + BitRate/2 <= 500Khz) /FDEV = frequency deviation
    /* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_90000}, // rfm12 90kHz Deviation -> (90kHz + 17,24kbps/2) <= 500kHz i.O.
  
    /* 0x07 */ { REG_FRFMSB, 0xD9 }, //FRF_MSB }, //868,3 MHz
    /* 0x08 */ { REG_FRFMID, 0x13 }, //FRF_MID },
    /* 0x09 */ { REG_FRFLSB, 0x33 }, //FRF_LSB },    
        
    //* 0x0A */ calibration of the RC oscillator, trigger and read
		//* 0x0B */ { REG_AFCCTRL, RF_AFCCTL_LOWBETA_OFF }, // Improved AFC // needed when index<2  modulation index 0.5 <= beta = 2*Fdev/BitRate <=10 -> Standard AFC routine
		//* 0x0C */ unused
		//* 0x0D */ { REG_LISTEN1, RF_LISTEN1_CRITERIA_RSSI }, // match >minRSSI & AddressSync
		//* 0x0D - 0x10 */ RegListen deaktivated
		
		/* Transmitter Registers */
		// looks like PA1 and PA2 are not implemented on RFM69W, hence the max output power is 13dBm
		// +17dBm and +20dBm are possible on RFM69HW
		// +13dBm formula: Pout=-18+OutputPower (with PA0 or PA1**)
		// +17dBm formula: Pout=-14+OutputPower (with PA1 and PA2)**
		// +20dBm formula: Pout=-11+OutputPower (with PA1 and PA2)** and high power PA settings (section 3.3.7 in datasheet)
		//* 0x11 */ { REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF_PALEVEL_OUTPUTPOWER_11111}, //set by setHighPower function
		//* 0x12 */ { REG_PARAMP, RF_PARAMP_40 }, //Rise/Fall time of ramp up/down in FSK
		//* 0x13 */ { REG_OCP, RF_OCP_ON | RF_OCP_TRIM_95 }, //over current protection (default is 95mA), set by setHighPower function

		
		/* Receiver Registers */
		//* 0x14 - 0x17 */ unused
		//* 0x18 */ { REG_LNA,  RF_LNA_ZIN_50 | RF_LNA_GAINSELECT_AUTO }, // 200Ohm default / rfm12 MAX-LNA Gain setting
		/* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_1 }, //(BitRate < 2 * RxBw) -> (17,24kbs < 2* rfm12 166,7 kHz
		//* 0x1A */ RegAfcBw
		//* 0x1B */ OOK Data Mode
		//* 0x1C */ OOK Data Mode
		//* 0x1D */ OOK Data Mode		
		//* 0x1E */ { REG_AFCFEI, RF_AFCFEI_AFCAUTO_ON | RF_AFCFEI_AFC_CLEAR | RF_AFCFEI_AFCAUTOCLEAR_ON | RF_AFCFEI_FEI_START },
		//* 0x1E - 0x22 */ Afc Setting
		//* 0x23 0x24 */ RSSI 
		
		/* IRQ and Pin Mapping */
		/* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01 }, //DIO0 is the only IRQ we're using
		/* 0x26 */ { REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF }, //ClockOut
		//* 0x27 */ RegIrqFlags1 read only
		/* 0x28 */ { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN }, // Writing to this bit ensures the FIFO & status flags are reset
    /* 0x29 */ { REG_RSSITHRESH, 0xE4 /*MAX*/ }, //(97*2) rfm -91dBm //must be set to dBm = (-Sensitivity / 2) - default is 0xE4=228 so -114dBm
		//* 0x2A-0x2B */ Timeout after switching to Rx mode if Rssi interrupt doesn’t occur
		
		/* Packet Engine Registers */
		/* 0x2C */ { REG_PREAMBLEMSB, RF_PREAMBLESIZE_MSB_VALUE },
		/* 0x2D */ { REG_PREAMBLELSB, RF_PREAMBLESIZE_LSB_VALUE }, // default 3 preamble bytes 0xAAAAAA
		/* 0x2E */ { REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_3 | RF_SYNC_TOL_0 },		
    /* 0x2F */ { REG_SYNCVALUE1, 0xAA },      //attempt to make this compatible with sync1 byte of RFM12B lib
    /* 0x30 */ { REG_SYNCVALUE2, 0x2D }, //NETWORK ID LaCrosse 0xD4
		/* 0x30 */ { REG_SYNCVALUE3, 0xD4 },    

    //* 0x31 - 0x36 */ possible SyncValues
    /* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_FIXED | RF_PACKET1_DCFREE_OFF |
    						 RF_PACKET1_CRC_OFF | RF_PACKET1_CRCAUTOCLEAR_OFF | 
    						 RF_PACKET1_ADRSFILTERING_OFF },
    /* 0x38 */ { REG_PAYLOADLENGTH, RFM69_PROTOCOL_LACROSSE_PAYLOADLENGTH}, //max0x40 in variable length mode: the max frame size, not used in TX
    //* 0x39 */ { REG_NODEADRS, nodeID }, //turned off because we're not using address filtering  
  	//* 0x3A */ { REG_BROADCASTADRS, 0 }, //0 is the broadcast address
  	//* 0x3B */ {  REG_AUTOMODES, 0 }, // Automatic Modes, currently not needed
  	//* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | /*0x7F=MAX*/ RF_FIFOTHRESH_VALUE }, //TX on FIFO not empty
    /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, //RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    //* 0x3E - 0x4D */ AesKey
    
    /* Temperature Sensor Registers */
    /* 0x4E - 0x4F */
    
    /* Test Registers */
    /* 0x58 - 0x71 */
		//{ REG_TESTPA1, 0x5d }, //set by setHighPowerRegs
		//{ REG_TESTPA2, 0x7c }, //set by setHighPowerRegs    
    /* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 }, // run DAGC continuously in RX mode, recommended default for AfcLowBetaOn=0   
		//* 0x58 */ { REG_TESTLNA, RF_LNA_BOOST_HIGH },
    {255, 0}
  };

/****************************************************************************/
/************************** FSK 868MHz - Homematic **************************/
/****************************************************************************/
/*
  BidCos Packet - General Information: https://homegear.eu/index.php/BidCoS_Packet_-_General_Information
  
	Die Homematic Komponenten verwenden alle den CC1100 von TI.
	Einstellung:
	RF-Registereinstellungen für den CC1100
	Einstellungen im SmartRF® Studio
	X-tal frequency: 26.000000 MHz
	RF output power: +10dBm, kein PA ramping
	Deviation: 19.042969 kHz
	Datarate: 9.992599 kBaud
	Modulation: 2-FSK, kein Manchester-Codec
	RF frequency: 868.299866 MHz
	Channel: 199.951172 kHz, Channel Number: 0
	RX filterbandwidth: 101.562500 kHz
*/
  const byte RFM69_CONFIG_HomeMatic[][2] =
  {
  	//* 0x00 */ Fifo
		/* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY }, //Sequencer on | Standby Mode
	  /* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00 }, //no shaping
	  /* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_10000},
    /* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_10000},
    
    /* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_20000}, //default:5khz, (FDEV + BitRate/2 <= 500Khz) /FDEV = frequency deviation
    /* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_20000}, // rfm12 90kHz Deviation -> (90kHz + 17,24kbps/2) <= 500kHz i.O.
  
    /* 0x07 */ { REG_FRFMSB, 0xD9 }, //FRF_MSB }, //868,3 MHz
    /* 0x08 */ { REG_FRFMID, 0x13 }, //FRF_MID },
    /* 0x09 */ { REG_FRFLSB, 0x33 }, //FRF_LSB },    
        
    //* 0x0A */ calibration of the RC oscillator, trigger and read
		//* 0x0B */ { REG_AFCCTRL, RF_AFCCTL_LOWBETA_OFF }, // Improved AFC // needed when index<2  modulation index 0.5 <= beta = 2*Fdev/BitRate <=10 -> Standard AFC routine
		//* 0x0C */ unused
		//* 0x0D */ { REG_LISTEN1, RF_LISTEN1_CRITERIA_RSSI }, // match >minRSSI & AddressSync
		//* 0x0D - 0x10 */ RegListen deaktivated
		
		/* Transmitter Registers */
		// looks like PA1 and PA2 are not implemented on RFM69W, hence the max output power is 13dBm
		// +17dBm and +20dBm are possible on RFM69HW
		// +13dBm formula: Pout=-18+OutputPower (with PA0 or PA1**)
		// +17dBm formula: Pout=-14+OutputPower (with PA1 and PA2)**
		// +20dBm formula: Pout=-11+OutputPower (with PA1 and PA2)** and high power PA settings (section 3.3.7 in datasheet)
		//* 0x11 */ { REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF_PALEVEL_OUTPUTPOWER_11111}, //set by setHighPower function
		//* 0x12 */ { REG_PARAMP, RF_PARAMP_40 }, //Rise/Fall time of ramp up/down in FSK
		//* 0x13 */ { REG_OCP, RF_OCP_ON | RF_OCP_TRIM_95 }, //over current protection (default is 95mA), set by setHighPower function

		
		/* Receiver Registers */
		//* 0x14 - 0x17 */ unused
		//* 0x18 */ { REG_LNA,  RF_LNA_ZIN_50 | RF_LNA_GAINSELECT_AUTO }, // 200Ohm default / rfm12 MAX-LNA Gain setting
		/* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_20 | RF_RXBW_EXP_2 }, //(BitRate < 2 * RxBw) -> (17,24kbs < 2* rfm12 166,7 kHz
		//* 0x1A */ RegAfcBw
		//* 0x1B */ OOK Data Mode
		//* 0x1C */ OOK Data Mode
		//* 0x1D */ OOK Data Mode		
		//* 0x1E */ { REG_AFCFEI, RF_AFCFEI_AFCAUTO_ON | RF_AFCFEI_AFC_CLEAR | RF_AFCFEI_AFCAUTOCLEAR_ON | RF_AFCFEI_FEI_START },
		//* 0x1E - 0x22 */ Afc Setting
		//* 0x23 0x24 */ RSSI 
		
		/* IRQ and Pin Mapping */
		/* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01 }, //DIO0 is the only IRQ we're using
		/* 0x26 */ { REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF }, //ClockOut
		//* 0x27 */ RegIrqFlags1 read only
		/* 0x28 */ { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN }, // Writing to this bit ensures the FIFO & status flags are reset
    /* 0x29 */ { REG_RSSITHRESH, 0xE4 /*MAX*/ }, //(97*2) rfm -91dBm //must be set to dBm = (-Sensitivity / 2) - default is 0xE4=228 so -114dBm
		//* 0x2A-0x2B */ Timeout after switching to Rx mode if Rssi interrupt doesn’t occur
		
		/* Packet Engine Registers */
		/* 0x2C */ { REG_PREAMBLEMSB, RF_PREAMBLESIZE_MSB_VALUE },
		/* 0x2D */ { REG_PREAMBLELSB, 3 }, // Homematic 4 Sync Words. TX already has AA in the SyncWord
		/* 0x2E */ { REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_5 | RF_SYNC_TOL_0 },	
		/* 0x30 */ { REG_SYNCVALUE1, 0xAA },	
    /* 0x30 */ { REG_SYNCVALUE2, 0xE9 }, //NETWORK ID HomeMatic 0xE9 0xCA (twice)
		/* 0x30 */ { REG_SYNCVALUE3, 0xCA },
    /* 0x30 */ { REG_SYNCVALUE4, 0xE9 },
		/* 0x30 */ { REG_SYNCVALUE5, 0xCA },    

    //* 0x31 - 0x36 */ possible SyncValues
    /* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_FIXED | RF_PACKET1_DCFREE_OFF |
    						 RF_PACKET1_CRC_OFF | RF_PACKET1_CRCAUTOCLEAR_OFF | 
    						 RF_PACKET1_ADRSFILTERING_OFF },
    /* 0x38 */ { REG_PAYLOADLENGTH, RFM69_PROTOCOL_HOMEMATIC_PAYLOADLENGTH},
    //* 0x39 */ { REG_NODEADRS, nodeID }, //turned off because we're not using address filtering  
  	//* 0x3A */ { REG_BROADCASTADRS, 0 }, //0 is the broadcast address
  	//* 0x3B */ {  REG_AUTOMODES, 0 }, // Automatic Modes, currently not needed
  	/* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | /*0x7F=MAX*/ RF_FIFOTHRESH_VALUE }, //TX on FIFO not empty
    /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, //RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    //* 0x3E - 0x4D */ AesKey
    
    /* Temperature Sensor Registers */
    /* 0x4E - 0x4F */
    
    /* Test Registers */
    /* 0x58 - 0x71 */
		//{ REG_TESTPA1, 0x5d }, //set by setHighPowerRegs
		//{ REG_TESTPA2, 0x7c }, //set by setHighPowerRegs    
    /* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 }, // run DAGC continuously in RX mode, recommended default for AfcLowBetaOn=0   
		//* 0x58 */ { REG_TESTLNA, RF_LNA_BOOST_HIGH },
    {255, 0}
  };
      
/***************************************************************************/
/************************** FSK 868MHz - ETH200 ****************************/
/***************************************************************************/

  const byte RFM69_CONFIG_ETH[][2] =
  {
  	//* 0x00 */ Fifo
		/* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY }, //Sequencer on | Standby Mode
	  /* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00 }, //no shaping
	  /* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_9600}, //ETH 9.6 kbps
    /* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_9600},
    
    /* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_25000}, //default:5khz, (FDEV + BitRate/2 <= 500Khz) /FDEV = frequency deviation
    /* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_25000}, // rfm12 90kHz Deviation -> (90kHz + 17,24kbps/2) <= 500kHz i.O.
    
    /* 0x07 */ { REG_FRFMSB, 0xD9 }, //FRF_MSB }, //ETH 868,3 MHz
    /* 0x08 */ { REG_FRFMID, 0x13 }, //FRF_MID },
    /* 0x09 */ { REG_FRFLSB, 0x33 }, //FRF_LSB },    
        
    //* 0x0A */ calibration of the RC oscillator, trigger and read
		//* 0x0B */ { REG_AFCCTRL, RF_AFCCTL_LOWBETA_OFF }, // Improved AFC // needed when index<2  modulation index 0.5 <= beta = 2*Fdev/BitRate <=10 -> Standard AFC routine
		//* 0x0C */ unused
		//* 0x0D */ { REG_LISTEN1, RF_LISTEN1_CRITERIA_RSSI }, // match >minRSSI & AddressSync
		//* 0x0D - 0x10 */ RegListen deaktivated

		/* Transmitter Registers */
		// looks like PA1 and PA2 are not implemented on RFM69W, hence the max output power is 13dBm
		// +17dBm and +20dBm are possible on RFM69HW
		// +13dBm formula: Pout=-18+OutputPower (with PA0 or PA1**)
		// +17dBm formula: Pout=-14+OutputPower (with PA1 and PA2)**
		// +20dBm formula: Pout=-11+OutputPower (with PA1 and PA2)** and high power PA settings (section 3.3.7 in datasheet)
		//* 0x11 */ { REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF_PALEVEL_OUTPUTPOWER_11111}, //set by setHighPower function
		//* 0x12 */ { REG_PARAMP, RF_PARAMP_40 }, //Rise/Fall time of ramp up/down in FSK
		//* 0x13 */ { REG_OCP, RF_OCP_ON | RF_OCP_TRIM_95 }, //over current protection (default is 95mA), set by setHighPower function
		
		/* Receiver Registers */
		//* 0x14 - 0x17 */ unused
		//* 0x18 */ { REG_LNA,  RF_LNA_ZIN_50 | RF_LNA_GAINSELECT_AUTO }, // 200Ohm default / rfm12 MAX-LNA Gain setting
		/* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_1 }, //(BitRate < 2 * RxBw) -> (17,24kbs < 2* rfm12 166,7 kHz
		//* 0x1A */ RegAfcBw
		//* 0x1B */ OOK Data Mode
		//* 0x1C */ OOK Data Mode
		//* 0x1D */ OOK Data Mode		
		//* 0x1E */ { REG_AFCFEI, RF_AFCFEI_AFCAUTO_ON | RF_AFCFEI_AFC_CLEAR | RF_AFCFEI_AFCAUTOCLEAR_ON | RF_AFCFEI_FEI_START },
		//* 0x1E - 0x22 */ Afc Setting
		//* 0x23 0x24 */ RSSI 
		
		/* IRQ and Pin Mapping */
		/* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01 }, //DIO0 is the only IRQ we're using
		/* 0x26 */ { REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF }, //ClockOut
		//* 0x27 */ RegIrqFlags1 read only
		/* 0x28 */ { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN }, // Writing to this bit ensures the FIFO & status flags are reset
    /* 0x29 */ { REG_RSSITHRESH, 0xE4 /*MAX*/ }, //(97*2) rfm -91dBm //must be set to dBm = (-Sensitivity / 2) - default is 0xE4=228 so -114dBm
		//* 0x2A-0x2B */ Timeout after switching to Rx mode if Rssi interrupt doesn’t occur
		
		/* Packet Engine Registers */
		/* 0x2C */ { REG_PREAMBLEMSB, 0/*RF_PREAMBLESIZE_MSB_VALUE*/ },
		/* 0x2D */ { REG_PREAMBLELSB, 0/*RF_PREAMBLESIZE_LSB_VALUE*/ }, // default 3 preamble bytes 0xAAAAAA
		/* 0x2E */ { REG_SYNCCONFIG, RF_SYNC_OFF | RF_SYNC_FIFOFILL_AUTO },// | RF_SYNC_SIZE_2 | RF_SYNC_TOL_0}, //wegen TX kein Sync
    //* 0x2F */ { REG_SYNCVALUE1, 0x6A }, //NETWORK ID ETH 0x6A
    //* 0x30 */ { REG_SYNCVALUE2, 0xA9 }, //NETWORK ID ETH 0xA9
    //* 0x31 - 0x36 */ possible SyncValues
    /* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_FIXED | RF_PACKET1_DCFREE_OFF |
    						 RF_PACKET1_CRC_OFF | RF_PACKET1_CRCAUTOCLEAR_OFF | 
    						 RF_PACKET1_ADRSFILTERING_OFF },
    /* 0x38 */ { REG_PAYLOADLENGTH, RFM69_PROTOCOL_ETH_PAYLOADLENGTH }, //max0x40 in variable length mode: the max frame size, not used in TX
    //* 0x39 */ { REG_NODEADRS, nodeID }, //turned off because we're not using address filtering  
  	//* 0x3A */ { REG_BROADCASTADRS, 0 }, //0 is the broadcast address
  	//* 0x3B */ {  REG_AUTOMODES, 0 }, // Automatic Modes, currently not needed
  	/* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFOTHRESH | RF_FIFOTHRESH_VALUE }, //TX on FIFO not empty
    /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, //RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    //* 0x3E - 0x4D */ AesKey
    
    /* Temperature Sensor Registers */
    /* 0x4E - 0x4F */
    
    /* Test Registers */
    /* 0x58 - 0x71 */
		//{ REG_TESTPA1, 0x5d }, //set by setHighPowerRegs
		//{ REG_TESTPA2, 0x7c }, //set by setHighPowerRegs    
    /* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA1 }, // run DAGC continuously in RX mode, recommended default for AfcLowBetaOn=0   
    {255, 0}
  };  
 
#endif

/***************************************************************************/
/************************** FSK 868MHz - MyProtocol ************************/
/***************************************************************************/
  const byte RFM69_CONFIG_MyProtocol[][2] =
  {
  	//* 0x00 */ Fifo
		/* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY }, //Sequencer on | Standby Mode
	  /* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00 }, //no shaping
	  /* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_55555},
    /* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_55555},
    
    /* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_50000}, //default:5khz, (FDEV + BitRate/2 <= 500Khz) /FDEV = frequency deviation
    /* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_50000}, // rfm12 90kHz Deviation -> (90kHz + 17,24kbps/2) <= 500kHz i.O.
    
    /* 0x07 */ { REG_FRFMSB, 0xD9 }, //FRF_MSB }, //868,0 MHz
    /* 0x08 */ { REG_FRFMID, 0x0  }, //FRF_MID },
    /* 0x09 */ { REG_FRFLSB, 0x0	}, //FRF_LSB },    
   
    //* 0x0A */ calibration of the RC oscillator, trigger and read
		//* 0x0B */ { REG_AFCCTRL, RF_AFCCTL_LOWBETA_ON }, // dafault:On // Improved AFC // needed when index<2  modulation index 0.5 <= beta = 2*Fdev/BitRate <=10 -> Standard AFC routine
		//* 0x0C */ unused
		//* 0x0D */ { REG_LISTEN1, RF_LISTEN1_CRITERIA_RSSI }, // match >minRSSI & AddressSync
		//* 0x0D - 0x10 */ RegListen deaktivated
  	/* 0x0D */ { REG_LISTEN1, RF_LISTEN1_RESOL_IDLE_4100 | RF_LISTEN1_RESOL_RX_64 | RF_LISTEN1_CRITERIA_RSSI | RF_LISTEN1_END_10 }, //0x94 10/*4.1ms*/ 01/*64µs*/ 0/*RSSIThreshold*/ 10/*End*/ 0
  	/* 0x0E */ { REG_LISTEN2, 0x40 }, //0x40: 64*4.1ms~262ms idle
  	/* 0x0F */ { REG_LISTEN3, 0x05 }, //0x20: 32*64us~2ms RX
		/* 0x2A */ { REG_RXTIMEOUT1, 0x11 }, //Timeout begrenzt Zeit inder ein Überschreiten des RSSI Threshould nach Umschaltung auf RX erkannt wird
  	/* 0x2B */ { REG_RXTIMEOUT2, 0x43 }, //Timout begrenzt die maximale Paketlänge im ListenMode! Paket muss fertig gesendet sein vor dem Timout -> 2x MaxLength
  																			 //needed otherwise will always be in RX mode		

		/* Transmitter Registers */
		// looks like PA1 and PA2 are not implemented on RFM69W, hence the max output power is 13dBm
		// +17dBm and +20dBm are possible on RFM69HW
		// +13dBm formula: Pout=-18+OutputPower (with PA0 or PA1**)
		// +17dBm formula: Pout=-14+OutputPower (with PA1 and PA2)**
		// +20dBm formula: Pout=-11+OutputPower (with PA1 and PA2)** and high power PA settings (section 3.3.7 in datasheet)
		//* 0x11 */ { REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF_PALEVEL_OUTPUTPOWER_11111}, //set by setHighPower function
		//* 0x12 */ { REG_PARAMP, RF_PARAMP_40 }, //Rise/Fall time of ramp up/down in FSK
		//* 0x13 */ { REG_OCP, RF_OCP_ON | RF_OCP_TRIM_95 }, //over current protection (default is 95mA), set by setHighPower function

		
		/* Receiver Registers */
		//* 0x14 - 0x17 */ unused
		//* 0x18 */ { REG_LNA,  RF_LNA_ZIN_50 | RF_LNA_GAINSELECT_AUTO }, // default: Auto + 200Ohm default / rfm12 MAX-LNA Gain setting
		/* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_1 },
		//* 0x1A */ RegAfcBw
		//* 0x1B */ OOK Data Mode
		//* 0x1C */ OOK Data Mode
		//* 0x1D */ OOK Data Mode		
		//* 0x1E */ { REG_AFCFEI, RF_AFCFEI_AFC_CLEAR | RF_AFCFEI_AFC_START | RF_AFCFEI_AFCAUTO_ON | RF_AFCFEI_AFCAUTOCLEAR_ON /*default*/ },
		//* 0x1E - 0x22 */ Afc Setting
		//* 0x23 0x24 */ RSSI 
		
		/* IRQ and Pin Mapping */
		/* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01 }, //DIO0 is the only IRQ we're using
		/* 0x26 */ { REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF }, //ClockOut
		//* 0x27 */ RegIrqFlags1 read only
		/* 0x28 */ { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN }, // Writing to this bit ensures the FIFO & status flags are reset
    //* 0x29 */ { REG_RSSITHRESH, 0xE4 /*MAX*/ }, //(97*2) rfm -91dBm //must be set to dBm = (-Sensitivity / 2) - default is 0xE4=228 so -114dBm
#if HAS_RFM69_LISTENMODE 	//Reduce RSSIThreshold for ListenMode Clients to reduce wake ups due to phantom packages or other clients
		/* 0x29 */ { REG_RSSITHRESH, 160 /*-90=180(0xB4); -100=200(0xC8) 0xE4=MAX*/ }, //changed for ListenMode
#else
    /* 0x29 */ { REG_RSSITHRESH, 220 /*-90=180; 0xE4=MAX*/ }, //changed for ListenMode
#endif
		//* 0x2A-0x2B */ Timeout after switching to Rx mode if Rssi interrupt doesn’t occur
		
		/* Packet Engine Registers */
		/* 0x2C */ { REG_PREAMBLEMSB, RF_PREAMBLESIZE_MSB_VALUE },
		/* 0x2D */ { REG_PREAMBLELSB, RF_PREAMBLESIZE_LSB_VALUE }, // default 3 preamble bytes 0xAAAAAA
		/* 0x2E */ { REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_3 | RF_SYNC_TOL_0 },
    /* 0x2F */ { REG_SYNCVALUE1, 0xAA },
    //* 0x2F */ { REG_SYNCVALUE2, 0x2D },
    //* 0x30 */ { REG_SYNCVALUE3, 0xE4 },
    /* 0x2F */ { REG_SYNCVALUE2, 0x5A }, //ListenMode Optimierung inkl. DCFREE_WHITENING function --> Improve Signal detection
    /* 0x30 */ { REG_SYNCVALUE3, 0x5A },    
    
    //* 0x31 - 0x36 */ possible SyncValues
    /* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_VARIABLE | RF_PACKET1_DCFREE_WHITENING /*RF_PACKET1_DCFREE_OFF*/ |
    						 RF_PACKET1_CRC_ON | RF_PACKET1_CRCAUTOCLEAR_ON
#if not defined(RFM69_NO_BROADCASTS) && not defined(HAS_RFM69_SNIFF)
    						 | RF_PACKET1_ADRSFILTERING_NODEBROADCAST
#elif not defined(HAS_RFM69_SNIFF)
								 | RF_PACKET1_ADRSFILTERING_NODE
#endif
    					 },
    /* 0x38 */ { REG_PAYLOADLENGTH, RFM69_PROTOCOL_MYPROTOCOL_PAYLOADLENGTH }, //max0x40 in variable length mode: the max frame size, not used in TX
#ifndef HAS_RFM69_SNIFF
    /* 0x39 */ { REG_NODEADRS, DEVICE_ID }, //using address filtering  
#endif
  	/* 0x3A */ { REG_BROADCASTADRS, DEVICE_ID_BROADCAST}, //0 is the broadcast address
  	//* 0x3B */ {  REG_AUTOMODES, 0 }, // Automatic Modes, currently not needed
  	/* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE }, //TX on FIFO not empty
    //* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_1BIT /*Default*/ | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, //RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    //* 0x3E - 0x4D */ AesKey
    
    /* Temperature Sensor Registers */
    /* 0x4E - 0x4F */
    
    /* Test Registers */
    /* 0x58 - 0x71 */
		//{ REG_TESTPA1, 0x5d }, //set by setHighPowerRegs
		//{ REG_TESTPA2, 0x7c }, //set by setHighPowerRegs    
    /* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 }, // run DAGC continuously in RX mode, recommended default for AfcLowBetaOn=0   
    {255, 0}
  };
 
/*********************************************************************************************/
/****************************** CONFIGs for Protocols ****************************************/
/*********************************************************************************************/

#define ProtocolInfoLines ((uint8_t)(sizeof(ProtocolInfo)/sizeof(ProtocolInfo[0])))
ProtocolInfos ProtocolInfo[] = {  // Index must match to the protocol number -1
//{  CONFIG ARRAY						, DATA LENGTH															, Receive Transform Fkt					, Transfer Transform Fkt }, 
	{ RFM69_CONFIG_MyProtocol	, RFM69_PROTOCOL_MYPROTOCOL_PAYLOADLENGTH	,	&(transformRxData_MyProtocol)	, &(transformTxData_MyProtocol)	},
#if !defined(RFM69_NO_OTHER_PROTOCOLS)
	{ RFM69_CONFIG_HX2262			, RFM69_PROTOCOL_HX2262_PAYLOADLENGTH			, &(transformRxData_HX2262)			, &(transformTxData_HX2262)			},
	{ RFM69_CONFIG_FS20 			, RFM69_PROTOCOL_FS20_PAYLOADLENGTH				, &(transformRxData_FS20) 			, NULL													},
	{ RFM69_CONFIG_LaCrosse		, RFM69_PROTOCOL_LACROSSE_PAYLOADLENGTH		, &(transformRxData_LaCrosse)		, NULL													},
	{ RFM69_CONFIG_ETH				, RFM69_PROTOCOL_ETH_PAYLOADLENGTH				, NULL													, &(transformTxData_ETH)				},
	{ RFM69_CONFIG_HomeMatic	, RFM69_PROTOCOL_HOMEMATIC_PAYLOADLENGTH	, &(transformRxData_HomeMatic)	, &(transformTxData_HomeMatic)	},
#endif
};

#endif
 
