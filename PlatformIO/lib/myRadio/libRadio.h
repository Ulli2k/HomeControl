
//TODO: RF69_Config.setHighPower wird manchmal überschrieben, eigentlich sollte aber isHighPower genutzt werden (Module ändert sich nicht!)

#ifndef _MY_LIB_RADIO_h
#define _MY_LIB_RADIO_h

#include <myBaseModule.h>

#include <myRadioRegisters.h>
#include <myRadioGlobals.h>
#include <myRadioProtocols.h>

#include <libSPI.h>

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
/* -------------------- */

//#define HAS_RADIO_POWER_ADJUSTABLE		0 //activate functions to adjust TX power
#define HAS_RADIO_TEMPERATURE_READ		0 //activates possiblity to read RFM Module temperature
#if HAS_RADIO_TEMPERATURE_READ
	#define COURSE_TEMP_COEF     			-90 // puts the temperature reading in the ballpark, user can fine tune the returned value
#endif

/* DEFAULT VALUES */
#define RADIO_Default_MSG_DebounceTime		 200

#ifndef RADIO_CMD_TUNNELING_DEFAULT_VALUE
	#define RADIO_CMD_TUNNELING_DEFAULT_VALUE						false
#endif
#ifndef RADIO_CMD_TUNNELING_HOSTID_DEFAULT_VALUE
	#define RADIO_CMD_TUNNELING_HOSTID_DEFAULT_VALUE		DEVICE_ID_BROADCAST
#endif
#ifndef RADIO_TXonly_DEFAULT_VALUE
	#define RADIO_TXonly_DEFAULT_VALUE									false
#endif
#ifndef RADIO_LISTENMODE_DEFAULT_VALUE
	#define RADIO_LISTENMODE_DEFAULT_VALUE							false
#endif
#ifndef RADIO_POWER_DEFAULT_VALUE
	#define RADIO_POWER_DEFAULT_VALUE		(RF69_Config.setHighPower ? 32 : 0)
#endif
/* -------------------- */

template <uint8_t RadioIndex, class SPIType, uint8_t rstPin, bool isHighPower=true>
class LibRadio : public myTiming {

private:
  SPIType spi;
  uint8_t intPin;
  myRADIO_DATA DataStruct;

	#if HAS_RADIO_CMD_TUNNELING==2 //Satellite
	    #define HAS_RADIO_TX_FORCED_DELAY		5	// [ms] Delay between TX Frames, needed to safely receiving commands from the host
	    byte safeHostID4Tunneling;					// ID which is used by Tunneling Command results back to Host
	#endif

	#if HAS_RADIO_TX_FORCED_DELAY
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
	#if HAS_RADIO_LISTENMODE
		bool ListenModeActive:1; 						// Value-Range: 0-1 || is RFM in ListenMode or not
	#endif
	#if HAS_RADIO_TXonly
		bool TXonly:1;											// Value-Range: 0-1 || with RX or not
	#endif

	//Cmd/Msg Tunnelling for myProtocol with Satellites
	#if HAS_RADIO_CMD_TUNNELING
		bool TunnelingCMDQuery:1; 						// Value-Range: 0-1 || Satellite: CMD Tunnel activate and select RFM Module to send back Querys.
																				//									|| Host: Select RFM Module to foreward other DeviceIDs
	#endif
	#if HAS_RADIO_CMD_TUNNELING==1 //Host
		bool TunnelingSendBurst:1; 					// Value-Range: 0-1
	#endif

		//TX-Power config
		bool setHighPower:1;								// Value-Range: 0-1 || selection of Module type
	#if HAS_RADIO_POWER_ADJUSTABLE
		byte powerLevel;										// Value-Range: 0-51 || this contains just the linear control part of the power level
		bool powerBoost:1;									// Value-Range: 0-1 || this controls whether we need to turn on the highpower regs based on the setPowerLevel input
		byte PA_Reg;												// Value-Range: 0-255 || saved and derived PA control bits so we don't have to spend time reading back from SPI port
	#endif
	} RF69_Config;

  /****************************************************************************************************************************/
  /************************************** FUNCTIONS ***************************************************************************/
  /****************************************************************************************************************************/

  /****************************************************************************************************************************/
  /************************************** OUTPUT POWER LEVEL ******************************************************************/
  /****************************************************************************************************************************/
  void setMinPowerLevel() {

  	if (!RF69_Config.setHighPower) {
  		spi.writeReg(REG_PALEVEL, (spi.readReg(REG_PALEVEL) & 0xE0));
  	} else {
  	  spi.writeReg(REG_OCP, RF_OCP_ON);
      spi.writeReg(REG_PALEVEL, (0x20 & 0x0f) | 0x20);
  	}
  }

  #if HAS_RADIO_POWER_ADJUSTABLE
	// set output power: 0=min, 31=max (for RFM69W or RFM69CW), 0-31 or 32->51 for RFM69HW (see below)
	// this results in a "weaker" transmitted signal, and directly results in a lower RSSI at the receiver
	void setPowerLevel(byte powerLevel) {
		// TWS Update: allow power level selections above 31.  Select appropriate PA based on the value
	  RF69_Config.powerBoost = (powerLevel >= 50);

		if (!RF69_Config.isRFM69HW || powerLevel < 32) {			// use original code without change
	    RF69_Config.powerLevel = (powerLevel > 31 ? 31 : powerLevel);
	    spi.writeReg(REG_PALEVEL, (spi.readReg(REG_PALEVEL) & 0xE0) | RF69_Config.powerLevel);
	  } else {
	  	// the allowable range of power level value, if >31 is: 32 -> 51, where...
	  	// 32->47 use PA2 only and sets powerLevel register 0-15,
	  	// 48->49 uses both PAs, and sets powerLevel register 14-15,
	  	// 50->51 uses both PAs, sets powerBoost, and sets powerLevel register 14-15.
	  	RF69_Config.powerLevel = (powerLevel > 51 ? 51 : powerLevel);
	  	if (powerLevel < 48) {
	      powerLevel = powerLevel & 0x0f;	// just use 4 lower bits when in high power mode
	      RF69_Config.PA_Reg = 0x20;
	    } else {
		    RF69_Config.PA_Reg = 0x60;
	    	if (powerLevel < 50) {
		    	powerLevel = powerLevel - 34;  // leaves 14-15
		    } else {
		    	if (powerLevel > 51)
		    		powerLevel = 51;  // saturate
		      powerLevel = powerLevel - 36;  // leaves 14-15
		    }
		  }
		  spi.writeReg(REG_OCP, (RF69_Config.PA_Reg==0x60) ? RF_OCP_OFF : RF_OCP_ON);
	    spi.writeReg(REG_PALEVEL, powerLevel | RF69_Config.PA_Reg);
	  }
	}
  void setHighPower(bool onOff=true, byte PA_ctl=0x60) {
    RF69_Config.setHighPower = onOff;

    spi.writeReg(REG_OCP, (RF69_Config.setHighPower && PA_ctl==0x60) ? RF_OCP_OFF : RF_OCP_ON);
    if (RF69_Config.setHighPower) { //turning ON based on module type
    	RF69_Config.powerLevel = readReg(REG_PALEVEL) & 0x1F; // make sure internal value matches reg
    	RF69_Config.powerBoost = (PA_ctl == 0x60);
    	RF69_Config.PA_Reg = PA_ctl;
      spi.writeReg(REG_PALEVEL, RF69_Config.powerLevel | PA_ctl ); //TWS: enable selected P1 & P2 amplifier stages
    } else {
      RF69_Config.PA_Reg = RF_PALEVEL_PA0_ON;				// TWS: save to reflect register value
      spi.writeReg(REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF69_Config.powerLevel); //enable P0 only
    }
  }
  #else
  // for RFM69HW only: you must call setHighPower(true) after initialize() or else transmission won't work
  void setHighPower(bool onOff=true, byte PA_ctl=0x60) {
    RF69_Config.setHighPower = onOff;
    spi.writeReg(REG_OCP, RF69_Config.setHighPower ? RF_OCP_OFF : RF_OCP_ON);
    if (RF69_Config.setHighPower) // turning ON
      spi.writeReg(REG_PALEVEL, (spi.readReg(REG_PALEVEL) & 0x1F) | RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON); // enable P1 & P2 amplifier stages
    else
      spi.writeReg(REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | 31); // enable P0 only with max TX powerLevel
  }
  #endif

  void setHighPowerRegs(bool onOff) {
  #if HAS_RADIO_POWER_ADJUSTABLE
  	if ((0x60 != (spi.readReg(REG_PALEVEL) & 0xe0)) || !RF69_Config.powerBoost)		// TWS, only set to high power if we are using both PAs... and boost range is requested.
  		onOff = false;
  #endif
    spi.writeReg(REG_TESTPA1, onOff ? 0x5D : 0x55);
    spi.writeReg(REG_TESTPA2, onOff ? 0x7C : 0x70);
  }

	#if HAS_RADIO_TEMPERATURE_READ
	byte readTemperature(byte calFactor)  //returns centigrade
	{
	  setMode(RF69_MODE_STANDBY);
	  writeReg(REG_TEMP1, RF_TEMP1_MEAS_START);
	  while ((readReg(REG_TEMP1) & RF_TEMP1_MEAS_RUNNING));
	  return ~readReg(REG_TEMP2) + COURSE_TEMP_COEF + calFactor; //'complement'corrects the slope, rising temp = rising val
	}												   	  // COURSE_TEMP_COEF puts reading in the ballpark, user can add additional correction
	#endif

  /****************************************************************************************************************************/

  void setMode(byte newMode, bool forceReset=false, bool saveMode=true) {

  	if (newMode == RF69_Config.mode && !forceReset) return; //TODO: can remove this?

  #if HAS_RADIO_LISTENMODE
  	//ListenMode deaktivieren wenn ListenModeActive an ist und aber ein anderer Modus als RX gewünscht wird
  	//ListeMode deaktivieren wenn ListeModeActive abgeschalten wurde aber ListenMode noch aktiv ist.
  	if( (RF69_Config.ListenModeActive && newMode!=RF69_MODE_RX) || (!RF69_Config.ListenModeActive && ((spi.readReg(REG_OPMODE) & 0x40) != 0x0)) ) {
  		//DS("ListenOFF\n");
  		spi.writeReg(REG_OPMODE, (spi.readReg(REG_OPMODE) & 0x83) | RF_OPMODE_LISTEN_OFF | RF_OPMODE_LISTENABORT | RF_OPMODE_SLEEP);
  		spi.writeReg(REG_OPMODE, (spi.readReg(REG_OPMODE) & 0x83) | RF_OPMODE_LISTEN_OFF | RF_OPMODE_SLEEP);
  	}
  #endif

  //0xE3: Behält unused Bits, Abort, Listen, Sequencer bei
  	switch (newMode) {
  		case RF69_MODE_TX:
  			spi.writeReg(REG_OPMODE, (spi.readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_TRANSMITTER);
        if (RF69_Config.setHighPower) setHighPowerRegs(true);
        //DS("TX\n");
  			break;

  		case RF69_MODE_RX:
  		#if HAS_RADIO_LISTENMODE
  			if(!RF69_Config.ListenModeActive)
  		#endif
  			{
  				spi.writeReg(REG_OPMODE, (spi.readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_RECEIVER); //DS("RX\n");
  			}
        if (RF69_Config.setHighPower) setHighPowerRegs(false);
  	  #if HAS_RADIO_LISTENMODE
  			if(RF69_Config.ListenModeActive) {
  				spi.writeReg(REG_OPMODE, (spi.readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY);
  				spi.writeReg(REG_OPMODE, (spi.readReg(REG_OPMODE) & 0x83) | RF_OPMODE_SLEEP | RF_OPMODE_LISTEN_ON);
  				//DS("ListenOn\n");
  			}
  	  #endif
  			break;
  		case RF69_MODE_SYNTH:
  			spi.writeReg(REG_OPMODE, (spi.readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_SYNTHESIZER);
  			//DS("SYNTH\n");
  			break;
  		case RF69_MODE_STANDBY:
  			spi.writeReg(REG_OPMODE, (spi.readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY);
  			//DS("STBY\n");
  			break;
  		case RF69_MODE_SLEEP:
  			spi.writeReg(REG_OPMODE, (spi.readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_SLEEP);
  			//DS("SLEEP\n");
  			break;
  		default: return;
  	}

  	// we are using packet mode, so this check is not really needed
    // but waiting for mode ready is necessary when going from sleep because the FIFO may not be immediately available from previous mode
  	while (RF69_Config.mode == RF69_MODE_SLEEP && (spi.readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady

  	if(saveMode)
  		RF69_Config.mode = newMode;
  }

  int readRSSI(bool forceTrigger=false) {
    int rssi = 0;
    if (forceTrigger) {
      //RSSI trigger not needed if DAGC is in continuous mode
      spi.writeReg(REG_RSSICONFIG, RF_RSSI_START);
      while ((spi.readReg(REG_RSSICONFIG) & RF_RSSI_DONE) == 0x00); // Wait for RSSI_Ready
    }
    rssi = - (spi.readReg(REG_RSSIVALUE));
    rssi >>= 1; // division: /2
    return rssi;
  }

  bool ACKReceived(byte senderID, bool ACKcheck, bool CMDcheck) {
    receiveDone(); //process messages

  	if( ( (ACKcheck && (DataStruct.XData_Config & XDATA_ACK_RECEIVED)) ||
  			  (CMDcheck && (DataStruct.XData_Config & XDATA_CMD_RECEIVED)))
  			&& DataStruct.SENDERID == senderID) {
  		receiveDone(); //quickly reset radio to RX	in case of OutputTunneling
  		return true;
  	}
    return false;
  }

  /********************* Transmitter Functions *********************/
  //canSend Erkennung ist besser bei hohen REG_RSSITHRESH
  bool canSend() {

  	#if HAS_RADIO_TX_FORCED_DELAY
  		if(millis_since(lastRadioFrame) < HAS_RADIO_TX_FORCED_DELAY) {
  			//Auskommentiert um ggf. keinen laufenden Empfang abzubrechen.
  			//#if HAS_RADIO_LISTENMODE //Satellite --> delay between TX needed
  		 	//setMode(RF69_MODE_SLEEP);
  		 	//TIMING::delay(HAS_RADIO_TX_FORCED_DELAY-(TIMING::millis() - lastRadioFrame));
  		  //#endif
  			return false;
  		}
  	#endif

    if ( RF69_Config.mode == RF69_MODE_RX && DataStruct.PAYLOADLEN == 0) { //&& readRSSI() < CSMA_LIMIT ) { //if signal stronger than -100dBm is detected assume channel activity
    	int rssi=0;

  /* RSSI Measurement
    	oldRSSI=rssi;
  		unsigned long now = TIMING::millis();
    	receiveBegin();
    	noInterrupts();
  		while(1) {
  			rssi=readRSSI();
  			if((oldRSSI-rssi)<=-20 || (oldRSSI-rssi)>=20) { //change
  				if((oldRSSI-rssi)<=-20) {
  					DS(" | ");
  				} else {
  					DS("|  ");
  				}
  				DU(TIMING::millis()-now,0);DNL();
  				now = TIMING::millis();
  				oldRSSI = rssi;
  			}
  		}
  */
  		rssi=readRSSI();
  		if(!( (rssi < RF69_CSMA_LIMIT) || ((RF69_Config.oldRSSI-rssi)>=RF69_CSMA_DELTA_LIMIT && RF69_Config.oldRSSI!=0) ) && rssi != 0 /* needed for ListenMode */) {
  		//if(!( (rssi < RF69_CSMA_LIMIT) ) && rssi != 0 /* needed for ListenMode */) {
  			//DS("noise: "); DI(RF69_Config.oldRSSI,0); DS("-");DI(rssi,0);DNL();
  			if(rssi>RF69_Config.oldRSSI || RF69_Config.oldRSSI==0) RF69_Config.oldRSSI = rssi;  //save min RSSI for comparisson with next rssi value
  			return false;
  		}

  	 #if HAS_RADIO_LISTENMODE || HAS_RADIO_TXonly
  	 	setMode(RF69_MODE_SLEEP);
  	 #else
  		setMode(RF69_MODE_STANDBY);
  	 #endif

     // DS_P("CanSend: ");DI(RF69_Config.oldRSSI,0); DS(" ");DI(rssi,0);DS("\n");
      RF69_Config.oldRSSI=0;
      return true;
    }
    return false;
  }
  // should be called immediately after reception in case sender wants ACK
  void sendACK(byte senderID, bool CMD=false, bool ACK=true) {

  	char target[3];
  	target[0] = '0' + senderID/10;
  	target[1] = '0' + senderID%10;
  	target[2] = 0x0;
  	myRADIO_DATA lDataStruct;// = {0};
  	lDataStruct.XData_Config	= (CMD ? XDATA_CMD_RECEIVED : XDATA_NOTHING) | (ACK ? XDATA_ACK_RECEIVED : XDATA_NOTHING);
  	//lDataStruct.MsgCnt = (_RADIOTXMsgCounter==0xFF ? 1 : ++_RADIOTXMsgCounter);

    //int _RSSI = DataStruct.RSSI; // save payload received RSSI value
   	if(	ProtocolInfo[RF69_Config.Protocol-1].transformTxData != NULL) {
  		ProtocolInfo[RF69_Config.Protocol-1].transformTxData((char*)target,&lDataStruct);//((char*)"", (unsigned char*)lData, (unsigned char *)&lDataLen, RF69_DataOption_sendACK, senderID);
  		send(&lDataStruct);
  		//send((const void*)lDataStruct.DATA,lDataStruct.PAYLOADLEN, lDataStruct.XDATA_Repeats);
  	} else {
  		DS_P("sendACK not supported.\n");
  	}
    //DataStruct.RSSI = _RSSI; // restore payload RSSI
  }

public:
  void send(myRADIO_DATA *lDataStruct) {

  	byte *b = (byte*)lDataStruct->DATA;
  	if(DEBUG) {
  		DS_P("TX: ");
  		for(uint8_t i=0; i<lDataStruct->PAYLOADLEN; i++) {
  			DH2(b[i]);
  		}
  		DS_P(" <");DU(((lDataStruct->XDATA_Repeats==0)?lDataStruct->XDATA_BurstTime:lDataStruct->XDATA_Repeats),0);DC(lDataStruct->XData_Config & XDATA_BURST ? 'b' : 's');DS_P(">\n");
  	}

  #if HAS_RADIO_LISTENMODE
  	bool _ListenMode=0;
  	_ListenMode = RF69_Config.ListenModeActive;
  	RF69_Config.ListenModeActive=0;
  	setMode(RF69_MODE_RX); //Switching to continues RX..needed for RSSI reading in canSend()
  #endif

  	//prepare to send and wait till RSSI is just noise, no other transmissions active
    spi.writeReg(REG_PACKETCONFIG2, (spi.readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks

  #if HAS_RADIO_LISTENMODE
    //increase RSSIThreshold for better noise detection
    byte rssiThreshold = readReg(REG_RSSITHRESH);
    spi.writeReg(REG_RSSITHRESH,RF69_RX_MAX_RSSITHRESH);
  #endif

  	unsigned long startTime = millis();
    while (!canSend() && millis_since(startTime) < RF69_CSMA_LIMIT_MS) receiveDone(); //receiveBegin(); //??Begin führt zu einem kleineren Delay als receiveDone(); besonders ohne ListenMode//
  	if(DEBUG & (millis_since(startTime) >= RF69_CSMA_LIMIT_MS_DEBUG)) { DS_P("CSMA ");DU(millis_since(startTime),0);DS("ms\n"); }

  #if HAS_RADIO_LISTENMODE
  	spi.writeReg(REG_RSSITHRESH,rssiThreshold); //reset Threshold value
  #endif

  	bool gotACK=false;
    for (uint8_t i = 0; i <= RF69_SEND_RETRIES; i++) {

  #if HAS_RADIO_ADC_NOISE_REDUCTION
  	  FKT_ADC_OFF;
  #endif
  		sendFrame(lDataStruct); //sent a BURST or SINGLE Message
  #if HAS_RADIO_ADC_NOISE_REDUCTION
  		FKT_ADC_ON;
  #endif

  		if(lDataStruct->XData_Config & (XDATA_ACK_REQUEST | XDATA_CMD_REQUEST)) {	//wait for ACK_RECEIVED or CMD_RECEIVED
  			//if(!i) startTime = TIMING::millis();
  		  //unsigned long sentTime = TIMING::millis();
  		  startTime = millis();
  			while (millis_since(startTime) <= RF69_SEND_RETRY_WAITTIME) {
  			 	if(ACKReceived(lDataStruct->TARGETID,lDataStruct->XData_Config & XDATA_ACK_REQUEST, lDataStruct->XData_Config & XDATA_CMD_REQUEST)) {
  			   	//DU(lDataStruct->TARGETID,2);(lDataStruct->XData_Config & XDATA_ACK_REQUEST) ? DS_P("ACK") : DS_P("CMD");
  			   	//DU(lDataStruct->CheckSum,0);DC(' ');DU(TIMING::millis()-startTime,0);DS("ms-");DU(i,0);DNL();
  			   	DU(lDataStruct->TARGETID,2);(lDataStruct->XData_Config & XDATA_ACK_REQUEST) ? DS_P(" ACK ") : DS_P(" CMD ");DU(millis_since(startTime),0);DS("ms-");DU(i,0);DNL();
  			   	gotACK=true;
  			   	break;
  			 	}
  			}
  		} else {
  			gotACK=true;
  		}
  		if(gotACK) i=RF69_SEND_RETRIES+1;
    }

  	if(!gotACK) {
  	 	DU(lDataStruct->TARGETID,2);
  	 	(lDataStruct->XData_Config & XDATA_ACK_REQUEST) ? DS_P(" ACK") : DS_P(" CMD");
  	 	DS_P(" missed"); DS(" [");
  	 	for(uint8_t i=0; i<lDataStruct->PAYLOADLEN; i++)
  	 		DH2(lDataStruct->DATA[i]);
  		DS("]\n");
  	}

  #if HAS_RADIO_LISTENMODE
  	RF69_Config.ListenModeActive=_ListenMode;
  #endif
  }

private:
  /*
   XDATA_BURST																											--> LongFrame mit XDATA_Repeats
   XDATA_SYNC XDATA_PREAMBLE 																				--> simples senden mit XDATA_Repeats
  */
  void sendFrame(myRADIO_DATA *lDataStruct) {

  //	byte BurstMode	= (lDataStruct->XData_Config & XDATA_BURST);
  //	byte BurstTime = (lDataStruct->XDATA_BurstTime>0);
  	byte bufferSize	=	lDataStruct->PAYLOADLEN;

  	if((lDataStruct->XData_Config & XDATA_BURST) && (lDataStruct->XData_Config & XDATA_BURST_INFO_APPENDIX)) bufferSize-=2; //add two BurstDelay bytes to payloadlenght byte
  	unsigned long txStart=0;
  	unsigned long txBusy=0;

    //turn off receiver to prevent reception while filling fifo
   	setMode(RF69_MODE_STANDBY);  // no sleep mode possible due to not working BURST TX in SleepMode

  	while ((spi.readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady
  	spi.writeReg(REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN); //clear FIFO
   	spi.writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00); // DIO0 is "Packet Sent"
    if (bufferSize > RF69_MAX_DATA_LEN) bufferSize = RF69_MAX_DATA_LEN;


    //Turn OFF Sync Words if requested
    if(!(lDataStruct->XData_Config & XDATA_SYNC)) spi.writeReg(REG_SYNCCONFIG,spi.readReg(REG_SYNCCONFIG) & (~RF_SYNC_ON));
    //Turn OFF Preamble Words if requested
  	uint16_t nPreamble	=	0; //number of preamble bytes
    if(!(lDataStruct->XData_Config & XDATA_PREAMBLE)) {
    	nPreamble = (spi.readReg(REG_PREAMBLEMSB) << 8) | spi.readReg(REG_PREAMBLELSB); //number of Preamble bytes
    	spi.writeReg(REG_PREAMBLEMSB,0x00);
    	spi.writeReg(REG_PREAMBLELSB,0x00);
    }

    //send Frame
    uint16_t cycles=0;
    txStart = millis();
    uint16_t diff = (uint16_t) lDataStruct->XDATA_BurstTime - millis_since(txStart);

  	while(cycles <= ( (lDataStruct->XDATA_BurstTime>0) ? lDataStruct->XDATA_BurstTime : lDataStruct->XDATA_Repeats)) { //also valid for burst mode because one cycle always takes more than 1ms

      spi.writeBurstKeepOpen(REG_FIFO,(uint8_t*)lDataStruct->DATA,bufferSize);

  		//BurstMode add delay info at the end of the payload for ListenMode clients
  		if((lDataStruct->XData_Config & XDATA_BURST) && (lDataStruct->XData_Config & XDATA_BURST_INFO_APPENDIX)) {
        byte buffer[] = {(byte)(diff & 0xff) , (byte)(diff >> 8)};
        spi.writeBurstWithoutOpen(buffer,2);
  			// mySpi.xfer_byte((byte)(diff & 0xff));
  			// mySpi.xfer_byte((byte)(diff >> 8));
  		} else {
        spi.writeBurstWithoutOpen(NULL,0);
      }

  		if(!cycles || !(lDataStruct->XData_Config & XDATA_BURST)) {
  			setMode(RF69_MODE_TX); //push first data frame into FIFO while rfm69 is in standby
  		}

  		txBusy=millis();
  		if((lDataStruct->XData_Config & XDATA_BURST) && (lDataStruct->XData_Config & XDATA_BURST_INFINITY)) {
  			//needed for ETH, infinity PackageMode. Fillup FIFO before it is empty!
  			while ((spi.readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_FIFOLEVEL) != 0x00 && millis_since(txBusy) < RF69_TX_LIMIT_MS);
  		} else {
  			while(digitalRead(/*mySpi.getIntPin(RF69_Config.spi_id)*/intPin) == 0 && millis_since(txBusy) < RF69_TX_LIMIT_MS); //wait till TX is done
  		}
  		if(DEBUG & (millis_since(txBusy) >= RF69_CSMA_LIMIT_MS_DEBUG)) { DS_P("TX ");DU(millis_since(txBusy),0);DS("ms\n"); }

  		//done?
  		if((lDataStruct->XData_Config & XDATA_BURST) && (lDataStruct->XDATA_BurstTime>0)) {
  			if( (diff = millis_since(txStart)) > lDataStruct->XDATA_BurstTime )  //check time during burst
  				break;
  			diff = (uint16_t) lDataStruct->XDATA_BurstTime - diff;
  		} else {
  			if(cycles < lDataStruct->XDATA_Repeats) { //no TX finished check on last Frame
  			 	if(!(lDataStruct->XData_Config & XDATA_BURST)) setMode(RF69_MODE_STANDBY);
  			} else {
  				break;
  			}
  		}
  		cycles++;
  	}

   #if HAS_RADIO_TX_FORCED_DELAY
  	lastRadioFrame = millis();
   #endif

   #if HAS_RADIO_LISTENMODE || HAS_RADIO_TXonly
   	setMode(RF69_MODE_SLEEP);
   #else
    setMode(RF69_MODE_STANDBY);
   #endif

   //Turn back on Sync Words if requested
    if(!(lDataStruct->XData_Config & XDATA_SYNC)) spi.writeReg(REG_SYNCCONFIG,spi.readReg(REG_SYNCCONFIG) | RF_SYNC_ON);
   //Turn back on Preamble Words if requested
    if(!(lDataStruct->XData_Config & XDATA_PREAMBLE) && nPreamble) {
  		spi.writeReg(REG_PREAMBLEMSB,(byte)(nPreamble>>8));
    	spi.writeReg(REG_PREAMBLELSB,(byte)nPreamble);
    }
  }

public:
  LibRadio () {
    RF69_Config.Protocol = RADIO_DEFAULT_PROTOCOL;
    RF69_Config.mode = RF69_MODE_STANDBY;
    RF69_Config.setHighPower = isHighPower;

		#if HAS_RADIO_POWER_ADJUSTABLE
		  RF69_Config.powerLevel = RADIO_POWER_DEFAULT_VALUE; //min power level
		  RF69_Config.powerBoost = false;   // require someone to explicitly turn boost on!
		#endif

		#if HAS_RADIO_LISTENMODE
		  RF69_Config.ListenModeActive=RADIO_LISTENMODE_DEFAULT_VALUE;
		#endif

		#if HAS_RADIO_TXonly
			RF69_Config.TXonly=RADIO_TXonly_DEFAULT_VALUE;
		#if defined(STORE_CONFIGURATION)
			getStoredValue((void*)&RF69_Config.TXonly, (const void*)&eeTXonly,1);
		#endif
		#endif

		#if HAS_RADIO_TX_FORCED_DELAY
			lastRadioFrame=0; //delay between TX & RX->TX needed
		#endif

		#if HAS_RADIO_CMD_TUNNELING
			RF69_Config.TunnelingCMDQuery=RADIO_CMD_TUNNELING_DEFAULT_VALUE;
		#if defined(STORE_CONFIGURATION)
			getStoredValue((void*)&RF69_Config.TunnelingCMDQuery, (const void*)&eeTunnelingCMDQuery,1);
		#endif
		#endif

		#if HAS_RADIO_CMD_TUNNELING==1 //Host
		  RF69_Config.TunnelingSendBurst=false;
		#endif

		#if HAS_RADIO_CMD_TUNNELING==2 //Satellite
			safeHostID4Tunneling=RADIO_CMD_TUNNELING_HOSTID_DEFAULT_VALUE;
		#if defined(STORE_CONFIGURATION)
			getStoredValue((void*)&safeHostID4Tunneling, (const void*)&eesafeHostID4Tunneling,1);
		#endif
		#endif

			RF69_Config.mode = RF69_MODE_STANDBY;
  }

	//Reset Radio
  void resetRadio() {
  	// pinMode(rstPin, OUTPUT);
		pinMode(rstPin, INPUT_PULLUP);

  	digitalWrite(rstPin, HIGH);
  	delayMicroseconds(100);
		digitalWrite(rstPin, LOW);
  	delay(100);
  	pinMode(rstPin, INPUT); //set to input for reducing power consumption
  }

	char getRadioIndex() {
		return '0'+RadioIndex;
	}
  uint8_t getProtocol() {
    return RF69_Config.Protocol;
  }
  uint8_t getPowerLevel() {
    return RF69_Config.powerLevel;
  }
  bool is433Module() {
    return (spi.readReg(REG_FRFMSB)==0x6C ? true : false);
  }
	bool isTunnelingActive() {
			return (RF69_Config.TunnelingCMDQuery ? true : false);
	}
	void setTunneling(bool active) {
		RF69_Config.TunnelingCMDQuery = active;
	}
	byte getTunnelingHostID(void) {
		#if HAS_RADIO_CMD_TUNNELING==2 //Satellite
		return safeHostID4Tunneling;
		#endif
		return DEVICE_ID_BROADCAST;
	}
	void setTunnelingHostID(byte hostID) {
		#if HAS_RADIO_CMD_TUNNELING==2 //Satellite
		safeHostID4Tunneling = hostID;
		#endif
	}
	void resetForcedDelayTimer() {
		#ifdef HAS_RADIO_TX_FORCED_DELAY
		lastRadioFrame = millis();
		#endif
	}

  int getRSSI() {
    return DataStruct.RSSI;
  }

  const byte* getPayloadDATA() {
    return (const byte*) DataStruct.DATA;
  }

  uint8_t getPayloadLen() {
    return DataStruct.PAYLOADLEN;
  }

  uint8_t getSenderID() {
    return DataStruct.SENDERID;
  }
  byte* getPayloadPointer() {
    return (byte*)DataStruct.DATA;
  }

  uint8_t getPayloadCheckSum() {
    return DataStruct.CheckSum;
  }

  bool isPayloadFlag_CMD_REQ() {
    return (DataStruct.XData_Config & XDATA_CMD_REQUEST ? 1 : 0);
  }
  bool isPayloadFlag_ACK_REQ() {
    return (DataStruct.XData_Config & XDATA_ACK_REQUEST ? 1 : 0);
  }
  bool isPayloadFlag_CMD_RECV() {
    return (DataStruct.XData_Config & XDATA_CMD_RECEIVED ? 1 : 0);
  }
  bool isPayloadFlag_ACK_RECV() {
    return (DataStruct.XData_Config & XDATA_ACK_RECEIVED ? 1 : 0);
  }
  bool isPayloadFlagSet() {
    return ((DataStruct.XData_Config & (XDATA_CMD_RECEIVED | XDATA_CMD_REQUEST | XDATA_ACK_RECEIVED | XDATA_ACK_REQUEST)) ? 1 : 0);
  }

  uint16_t getPayloadAttribute() {
    return DataStruct.XData_Config;
  }

  void waitBurstTime() {  //wait till burst is over
  	if(DataStruct.XDATA_BurstTime) {
  	#if HAS_RADIO_LISTENMODE
  		if(RF69_Config.ListenModeActive) {
  			DFL();
  			FKT_POWERDOWN(SLEEP_15Ms,(DataStruct.XDATA_BurstTime/15)+1); //+1 safety
  		} else
  	#endif
  		{
  		    delay(DataStruct.XDATA_BurstTime);
  		}
  	}
  }

  void attachRSSI2DATA() { // add RSSI value at the end of the buffer

    if (DataStruct.PAYLOADLEN < RF69_MAX_DATA_LEN) {
      DataStruct.DATA[DataStruct.PAYLOADLEN] = ((-(DataStruct.RSSI)) << 1); //transform int to byte and add to DATA BUFFER
      DataStruct.PAYLOADLEN++;
    }
  }

  void sendACK() {
    sendACK(DataStruct.SENDERID); //send ACK after processing command, due to clearing DataStruct
  }

  void initialize(uint8_t _intPin) {
    intPin = _intPin;
    spi.initialize();
    configure(RF69_Config.Protocol);
  }

  void configure(uint8_t protocol=RADIO_DEFAULT_PROTOCOL) {
    const unsigned char (*config)[2];

    if(protocol > ProtocolInfoLines || protocol <= 0) {
      DS_P("Protocol <"); DU(protocol,2); DS_P("> unknown."); DNL();
      return;
    }

    resetRadio();

    //rcCalibration(); //will automatically calibrated after RFM restart
    RF69_Config.Protocol = protocol;
    config = ProtocolInfo[protocol-1].config;

    do spi.writeReg(REG_SYNCVALUE1, 0xAA); while (spi.readReg(REG_SYNCVALUE1) != 0xAA);
    do spi.writeReg(REG_SYNCVALUE1, 0x55); while (spi.readReg(REG_SYNCVALUE1) != 0x55);

    for (byte i = 0; config[i][0] != 255; i++)
      spi.writeReg(config[i][0], config[i][1]);

    encrypt(0); // Encryption deactivated because no continues TX Mode (Burst) for Satellites(ListenMode) is possible.

    #if HAS_RADIO_POWER_ADJUSTABLE
    	uint8_t power = RF69_Config.powerLevel;
    	setHighPower(RF69_Config.setHighPower); //called regardless if it's a RFM69W or RFM69HW
    	RF69_Config.powerLevel = power;
    	setPowerLevel(RF69_Config.powerLevel);
    #else
    	setHighPower(RF69_Config.setHighPower); //called regardless if it's a RFM69W or RFM69HW
    	setMinPowerLevel();
    #endif

    	RF69_Config.PacketFormatVariableLength = (spi.readReg(REG_PACKETCONFIG1) & RF_PACKET1_FORMAT_VARIABLE) ? true : false;

     #if HAS_RADIO_LISTENMODE || HAS_RADIO_TXonly
     	setMode(RF69_MODE_SLEEP);
     #else
      setMode(RF69_MODE_STANDBY);
     #endif
    	while ((spi.readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady
  }

  void encrypt(const char* key) {
    setMode(RF69_MODE_STANDBY); //must be in Standby
    if (key!=0) {
        spi.writeBurst(REG_AESKEY1,(uint8_t*)key,16);
    }
    spi.writeReg(REG_PACKETCONFIG2, (spi.readReg(REG_PACKETCONFIG2) & 0xFE) | (key ? 1 : 0));
  }

  void toggleRadioMode(uint8_t protocol, uint16_t *config) {

  	if(!(*config)) { //save
  		*config = (RF69_Config.Protocol << 8) ;
  #if HAS_RADIO_LISTENMODE
  		*config |= (RF69_Config.ListenModeActive ? (1<<7): 0) ;
  #endif
  #if HAS_RADIO_CMD_TUNNELING
  		*config |= (RF69_Config.TunnelingCMDQuery ? (1<<5):0);
  #endif
  #if HAS_RADIO_CMD_TUNNELING==1 //Host
  		*config |= (RF69_Config.TunnelingSendBurst ? (1<<6): 0);
  #endif

  		configure(protocol);

  	} else { //restore
  		protocol = ((*config) >> 8);
  		configure(protocol);

  #if HAS_RADIO_CMD_TUNNELING
  		RF69_Config.TunnelingCMDQuery = (((*config) & (1<<5)) ? true : false);
  #endif
	// in Radio Module verschoben wegen addtoRingBuffer
  // #if HAS_RADIO_CMD_TUNNELING==2 //Satellite
  // 		if(RF69_Config.TunnelingCMDQuery)
  // 			addToRingBuffer(MODULE_DATAPROCESSING,MODULE_DATAPROCESSING_OUTPUTTUNNEL,(const byte*)"1",1);
  // #endif
  //		DU(protocol,0);DS_P(" Tunnel:");DU(RF69_Config.TunnelingCMDQuery,0);
  #if HAS_RADIO_CMD_TUNNELING==1 //Host
  		RF69_Config.TunnelingSendBurst = (((*config) & (1<<6)) ? true : false);
  //		DS_P(" burst:");DS((RF69_Config.TunnelingSendBurst?"1":"0"));
  #endif


  #if HAS_RADIO_LISTENMODE
  		RF69_Config.ListenModeActive = (((*config) & (1<<7)) ? true : false);
  //		DS_P(" listen:");DS((RF69_Config.ListenModeActive?"1":"0"));
  #endif
  //		DS("]\n");
  	}
  }

  bool transformData() {
    bool valid=1;

    if(DEBUG) { //speed up
			DS_P("RAW["); DU(RadioIndex,0);/*DC(mySpi.getIdent(RF69_Config.spi_id));*/DS_P("] ");
			for(uint8_t i=0; i<DataStruct.PAYLOADLEN; i++) {
				DH2(DataStruct.DATA[i]);
			}
			DS_P(" [");DU(DataStruct.PAYLOADLEN,0);DS_P("]\n");
		}

    if(	ProtocolInfo[RF69_Config.Protocol-1].transformRxData != NULL &&
  			ProtocolInfo[RF69_Config.Protocol-1].transformRxData(&DataStruct) == false) {//(DataStruct.DATA, &DataStruct.PAYLOADLEN, &DataOptions, &DataStruct.SENDERID) == false) {
			valid=0;
  	}
    return valid;
  }

  bool receiveDone() {
    #ifdef HAS_RADIO_TXonly
      if(RF69_Config.TXonly) return;
    #endif

  // ATOMIC_BLOCK(ATOMIC_FORCEON)
  // {
    noInterrupts(); //re-enabled in unselect() via setMode() or via receiveBegin()
  //  cfgInterrupt(this, mySpi.getIntPin(RF69_Config.spi_id), Interrupt_Block); //im SPI Module enthalten

    if (RF69_Config.mode == RF69_MODE_RX && DataStruct.PAYLOADLEN>0 ) {
     	//enables interrupts
  	 #if HAS_RADIO_LISTENMODE || HAS_RADIO_TXonly
  	 	setMode(RF69_MODE_SLEEP);
  	 #else
  		setMode(RF69_MODE_STANDBY);
  	 #endif
  //	 	cfgInterrupt(this, mySpi.getIntPin(RF69_Config.spi_id), Interrupt_Release); //im SPI Module enthalten
  		return true;
    } else if (RF69_Config.mode == RF69_MODE_RX) { //already in RX no payload yet
      interrupts(); //explicitly re-enable interrupts
  //    cfgInterrupt(this, mySpi.getIntPin(RF69_Config.spi_id), Interrupt_Release); //im SPI Module enthalten
      return false;
    }
    receiveBegin();
  //  cfgInterrupt(this, mySpi.getIntPin(RF69_Config.spi_id), Interrupt_Release); //im SPI Module enthalten
    return false;
  //}
  }


  void receiveBegin() {
  	memset(&DataStruct,0,sizeof(myRADIO_DATA));
  	RF69_Config.oldRSSI=0;

  	if (spi.readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY)
  	  spi.writeReg(REG_PACKETCONFIG2, (spi.readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks
  	spi.writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01); //set DIO0 to "PAYLOADREADY" in receive mode

    setMode(RF69_MODE_RX);
  }

  void fetchData() {
    myRADIO_DATA *data = &DataStruct;

  	if(RF69_Config.mode != RF69_MODE_RX) return;

  //	byte irq2 = readReg(REG_IRQFLAGS2);
  //	D_DS_P("Int ");D_DH2(irq2);D_DS_P("\n");

  	//INFO PinChange: im PinChg Interrupt gibt es kurz hintereinander ein PayloadReady interrupt und danach ein FifoNotEmpty interrupt. Der zweite löscht den Dateninhalt wieder -> 00000 Daten
  #if HAS_RADIO_LISTENMODE
    if (spi.readReg(REG_IRQFLAGS2) /*irq2*/ & (RF_IRQFLAGS2_PAYLOADREADY | RF_IRQFLAGS2_FIFONOTEMPTY) ) {
  #else
  	if (spi.readReg(REG_IRQFLAGS2) /*irq2*/ & RF_IRQFLAGS2_PAYLOADREADY ) {
  #endif

  //  if (irq2 & (RF_IRQFLAGS2_PAYLOADREADY | RF_IRQFLAGS2_FIFONOTEMPTY) ) {
  		data->RSSI = readRSSI();

  		#if HAS_RADIO_LISTENMODE
  		if(!RF69_Config.ListenModeActive)
  		#endif
  		{
  		 #if HAS_RADIO_LISTENMODE || HAS_RADIO_TXonly
  		 	setMode(RF69_MODE_SLEEP);
  		 #else
  			setMode(RF69_MODE_STANDBY);
  		 #endif
  		}
  		#if HAS_RADIO_LISTENMODE
  		 else {
  			setMode(RF69_MODE_SLEEP,true,false); //set RFM to sleep without changing current global mode. Necessary for ReceiveDone
  		}
  		#endif

  		//DS("Standby\n");TIMING::delay(5000);

			spi.readReg(REG_FIFO,true,false,true,false);
      data->PAYLOADLEN = (RF69_Config.PacketFormatVariableLength ? spi.readReg(0,false,false,false,true) : ProtocolInfo[RF69_Config.Protocol-1].PayloadLength ); //ProtocolInfo[Protocol-1].PayloadLength == 0xFF
  		data->PAYLOADLEN = data->PAYLOADLEN > RF69_MAX_DATA_LEN ? RF69_MAX_DATA_LEN : data->PAYLOADLEN; //precaution

			spi.readBurst(0,(uint8_t*)data->DATA,data->PAYLOADLEN,false,true,false);

      #if HAS_RADIO_LISTENMODE
  		if(!RF69_Config.ListenModeActive)
  		#endif
  		{
  			setMode(RF69_MODE_RX);
  		}

  		//data->RSSI = readRSSI();
  		//DS("END - ");DS_P("Int ");DU(RF69_Config.mode,0);DS_P(" ");DU(data->PAYLOADLEN,0);DS_P("\n");//TIMING::delay(5000);
    }
  }

	#if INCLUDE_DEBUG_OUTPUT
	void readAllRegs(byte addr=0) {
	  byte regVal;

		if(addr!=0) {
	  	regVal = spi.readReg(addr);
		  DH(addr, 2); DS(" - ");
		  DH(regVal,2); DS(" - ");
		  DB(regVal);	DNL();

		} else {
			for (byte regAddr = 1; regAddr <= 0x4F; regAddr++) {
		    regVal = spi.readReg(regAddr);
			  DH(regAddr, 2);	DS(" - ");
			  DH(regVal,2); DS(" - ");
			  DB(regVal); DNL();
			}
		}
	}

	void writeRegs(uint8_t addr, uint8_t val) {
		spi.writeReg(addr,val);
	}
	#endif

};

#endif
