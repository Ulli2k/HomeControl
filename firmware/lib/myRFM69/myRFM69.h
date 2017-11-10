
#ifndef _MY_RFM69_MODULE_h
#define _MY_RFM69_MODULE_h

#include <myBaseModule.h>
#include "../mySPI/mySPI.h"

//#define HAS_RFM69_POWER_ADJUSTABLE		0 //activate functions to adjust TX power
#define HAS_RFM69_TEMPERATURE_READ		0 //activates possiblity to read RFM Module temperature
#if HAS_RFM69_TEMPERATURE_READ
	#define COURSE_TEMP_COEF     			-90 // puts the temperature reading in the ballpark, user can fine tune the returned value
#endif

#define BASIC_SPI	0	//activate basic spi functionality, without mySPI Library
#if BASIC_SPI==1
	#include <SPI.h>
	byte _SPCR;
	byte _SPSR;
#endif

#if MAX_SPI_SLAVES > 0x04
	#error MAX_SPI_SLAVES can not be higher than 4 due to TunnelingModuleID,spi_id memory size.
#endif

//RFM Modes
#define RF69_MODE_SLEEP       0 // XTAL OFF
#define	RF69_MODE_STANDBY     1 // XTAL ON
#define RF69_MODE_SYNTH	      2 // PLL ON
#define RF69_MODE_RX          3 // RX MODE
#define RF69_MODE_TX		      4 // TX MODE
#define RF69_MODE_LISTEN		  5 // Listen MODE

#define XTALFREQ							32000000UL
#define F_STEPS								XTALFREQ/2^19

//TX - Timeouts and RSSI limits
#define RF69_CSMA_LIMIT          			 -90 // upper RX signal sensitivity threshold in dBm for carrier sense access
#define RF69_CSMA_DELTA_LIMIT						20 // RX signal sensitivity has changed. Delta in dBm is DELTA_LIMIT
#define RF69_CSMA_BURST_WAITING_MS			10 // used in canSend() to check if message is part of a burst and the burst is still running
#define RF69_CSMA_LIMIT_MS 						1000 //ms
#define	RF69_CSMA_LIMIT_MS_DEBUG	  		50 //ms (Report canSend() waiting time)
#define RF69_TX_LIMIT_MS 							1000 //wait till msg was sent sendFrame(), Timout
#define	RF69_TX_LIMIT_MS_DEBUG	  			50 //ms (Report send() waiting time after setMode(RF69_MODE_TX))
#define RF69_REQUESTS_DELAY							50 //ms delay before answer for ACK/CMD will be sent back to requester. Needed to be sure that Requester is in RX mode after TX.

/* sendWithRetry */
#define RF69_SEND_RETRY_WAITTIME			RF69_CSMA_LIMIT_MS//+200 ///Burst wurde durch 3 geteilt um Replay time zu reduzieren, daher kurzere Wartezeit
#define RF69_SEND_RETRIES							3
/* ------------- */

/* LISTEN FUNCTIONALITY */
#define RF69_RX_MAX_RSSITHRESH				0xE4 //for better noise/TXend detection in ListenMode, REG_RSSITHRESH will be set to maximum for canSend
//#define RF69_LISTEN_FREQ_SHIFT_ACTIV		 1
//#define RF69_RX_LISTEN_FRQ_SHIFT				75 //[kHz] max<1MHz ||  radio frequence will be shifted for ListenMode clients and therefore shifted for Burst TX
//#define RF69_RX_LISTEN_FRQ_SHIFT_OFFSET	(RF69_RX_LISTEN_FRQ_SHIFT*1000*(2^19/32000000UL))

/* -------------------- */

#define RFM69_Default_MSG_DebounceTime		 200

/*
#ifdef HAS_RFM69_TXonly
	#undef 	HAS_RFM69_CMD_TUNNELING //Satellite tunneling in TXonly mode
	#define	HAS_RFM69_CMD_TUNNELING		2
	
//	#undef 	HAS_RFM69_LISTENMODE //no listen mode allowed. RX continuously off
#endif
*/

#include <myRFM69globals.h>
#include <myRFM69protocols.h>

class myRFM69  : public myBaseModule {

  public:
    myRFM69(uint8_t spi_id, uint8_t resetPin=0, bool isRFM69HW=false, uint8_t protocol=RFM69_DEFAULT_PROTOCOL);

		void initialize();
	
		void displayData(RecvData *DataBuffer);
		bool validdisplayData(RecvData *DataBuffer);
		void printHelp();
		bool poll();
		void send(char *cmd, uint8_t typecode);

  protected:
		myRFM69_DATA DataStruct;

    //Msg Debouncing
    uint8_t _RFM69TXMsgCounter;		// Value-Range: 0-255
    word _tDebCmd;								// Value-Range: 0-255
    byte _tDebCRC;								// Value-Range: 0-255
    uint8_t _bDebMsgCnt;					// Value-Range: 0-255
    uint16_t _minDebounceTime;		// Value-Range: 0-65535
    
#if HAS_RFM69_CMD_TUNNELING==2 //Satellite
    #define HAS_RFM69_TX_FORCED_DELAY		5	// [ms] Delay between TX Frames, needed to safely receiving commands from the host
    byte safeHostID4Tunneling;					// ID which is used by Tunneling Command results back to Host   
#endif

#if HAS_RFM69_TX_FORCED_DELAY
		unsigned long lastRadioFrame;					// Time of last TX/RX-Frame. (delay of TX after RX, or between two TX
#endif

    //RFM Config
    struct {
    	int oldRSSI; 												// Value-Range: 0-255 || needed for CSMA_DELTA_LIMIT canSend()    
		  uint8_t spi_id:3; 									// Value-Range: 0-2 || array index of "typeSPItable SPITab" --> ID-1 || value range typeSPItable.id !
		  uint8_t ResetPin:5; 								// Value-Range: 0-21 (max. A7 in Atmega328p)
		  volatile byte mode:3;								// Value-Range: 0-5 || RFM Mode (TX,RX,Sleep,Standby)
			uint8_t Protocol:3;									// Value-Range: 0-6 || safe current active Protocol
			bool PacketFormatVariableLength:1;  // Value-Range: 0-1 || safe info if protocol works in Variable/Fix PackageLength
		#if HAS_RFM69_LISTENMODE
			bool ListenModeActive:1; 						// Value-Range: 0-1 || is RFM in ListenMode or not
		#endif
		#if HAS_RFM69_TXonly
			bool TXonly:1;											// Value-Range: 0-1 || with RX or not
		#endif	
		
    //Cmd/Msg Tunnelling for myProtocol with Satellites
		#if HAS_RFM69_CMD_TUNNELING
   		bool TunnelingCMDQuery:1; 						// Value-Range: 0-1 || Satellite: CMD Tunnel activate and select RFM Module to send back Querys. 
    																			//									|| Host: Select RFM Module to foreward other DeviceIDs
		#endif
		#if HAS_RFM69_CMD_TUNNELING==1 //Host
    	bool TunnelingSendBurst:1; 					// Value-Range: 0-1
		#endif
		
			//TX-Power config		
			bool isRFM69HW:1;										// Value-Range: 0-1 || selection of Module type
		#if HAS_RFM69_POWER_ADJUSTABLE
		  byte powerLevel;										// Value-Range: 0-51 || this contains just the linear control part of the power level
		  bool powerBoost:1;									// Value-Range: 0-1 || this controls whether we need to turn on the highpower regs based on the setPowerLevel input
		  byte PA_Reg;												// Value-Range: 0-255 || saved and derived PA control bits so we don't have to spend time reading back from SPI port
		#endif		
    } RF69_Config;
  
		void resetRFM69();
		void configure(uint8_t protocol=RFM69_DEFAULT_PROTOCOL);
    void setMode(byte mode, bool forceReset=false, bool saveMode=true);
    void encrypt(const char* key);
//#if HAS_RFM69_CMD_TUNNELING    
//    void frequenceShift(bool activ);
//#endif
    //void rcCalibration(); //calibrate the internal RC oscillator for use in wide temperature variations - see datasheet section [4.3.5. RC Timer Accuracy]
		//void AFCCalibration();
		
		// TX
		void toggleRadioMode(uint8_t protocol, uint16_t *config); //toggles Protocols if an other protocol is configured and another have to be transmitted
		inline bool ACKReceived(byte senderID, bool ACKcheck, bool CMDcheck);
   	inline bool canSend();
    void sendACK(byte senderID, bool CMD=false, bool ACK=true);
    void send(myRFM69_DATA *sDataStruct);
    inline void sendFrame(myRFM69_DATA *lDataStruct);

    void setHighPowerRegs(bool onOff);
		void setHighPower(bool onOFF=true, byte PA_ctl=0x60); //have to call it after initialize for RFM69HW
		void setMinPowerLevel();
#if HAS_RFM69_POWER_ADJUSTABLE		
    void setPowerLevel(byte level); //reduce/increase transmit power level
#endif

		// RX
	  void interrupt();
//#if !HAS_RFM69_TXonly
    void receiveBegin();
    bool receiveDone();
    bool DebounceRecvData(myRFM69_DATA *DataStruct); //return true/false if message is valid due to debouncing
//#endif
    int readRSSI(bool forceTrigger=false);
	
#if HAS_RFM69_TEMPERATURE_READ
    byte readTemperature(byte calFactor=0); //get CMOS temperature (8bit)
#endif
		
		// Hardware
    byte readReg(byte addr);
    void writeReg(byte addr, byte val);
    void readAllRegs(byte addr=0);    
#if BASIC_SPI==1    
    void select();
    void unselect();
#endif    
};

#endif
