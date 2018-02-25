
//#TODO: Tunneled Querrys gehen immer als Boadcast zurück nur das ACK/CMD geht an den richtigen

#include <myRFM69.h>
#include <RFM69registers.h>
//#include <util/crc16.h>  //_crc16_update

#if defined(STORE_CONFIGURATION) 
	#if HAS_RFM69_CMD_TUNNELING
	byte EEMEM eeTunnelingCMDQuery = false;
	#endif
	#if HAS_RFM69_CMD_TUNNELING==2
	byte EEMEM eesafeHostID4Tunneling = DEVICE_ID_BROADCAST;
	#endif
	#if HAS_RFM69_TXonly
	byte EEMEM eeTXonly = false;
	#endif
#endif 

#ifndef RFM69_CMD_TUNNELING_DEFAULT_VALUE
	#define RFM69_CMD_TUNNELING_DEFAULT_VALUE						false
#endif
#ifndef RFM69_CMD_TUNNELING_HOSTID_DEFAULT_VALUE
	#define RFM69_CMD_TUNNELING_HOSTID_DEFAULT_VALUE		DEVICE_ID_BROADCAST
#endif
#ifndef RFM69_TXonly_DEFAULT_VALUE
	#define RFM69_TXonly_DEFAULT_VALUE									false
#endif
#ifndef RFM69_LISTENMODE_DEFAULT_VALUE
	#define RFM69_LISTENMODE_DEFAULT_VALUE							false
#endif
#ifndef RFM69_POWER_DEFAULT_VALUE
	#define RFM69_POWER_DEFAULT_VALUE		(RF69_Config.isRFM69HW ? 32 : 0)
#endif
extern mySPI mySpi; //used for getIdent()

/*
// Carrierfrequenz in Hz
#define FREQUENCY			868300000LL //  868.3 MHz = 0xD91300
#define BITRATE				17240LL

// Don't change anything from here 
#define FRF						((FREQUENCY*524288LL + (XTALFREQ/2)) / XTALFREQ) //FSTEP = FXOSC/2^19
#define FRF_MSB				((FRF>>16) & 0xFF)
#define FRF_MID				((FRF>>8) & 0xFF)
#define FRF_LSB				((FRF) & 0xFF)

#define DATARATE			((XTALFREQ + (BITRATE/2)) / BITRATE)
#define DATARATE_MSB	(DATARATE>>8)
#define DATARATE_LSB	(DATARATE & 0xFF)
*/

myRFM69::myRFM69(uint8_t spi_id, uint8_t resetPin, bool isRFM69HW, uint8_t protocol) {
	
	RF69_Config.spi_id = mySpi.getClassID(spi_id); // transform defined id to real class index
	RF69_Config.Protocol = protocol;
	RF69_Config.isRFM69HW = isRFM69HW;
	RF69_Config.ResetPin = resetPin;

  _tDebCmd=0;
  _tDebCRC = 0;
  _bDebMsgCnt = 0;
  _RFM69TXMsgCounter=0;
	_minDebounceTime = RFM69_Default_MSG_DebounceTime;
	
#if HAS_RFM69_POWER_ADJUSTABLE
  RF69_Config.powerLevel = RFM69_POWER_DEFAULT_VALUE; //min power level
  RF69_Config.powerBoost = false;   // require someone to explicitly turn boost on!
#endif

#if HAS_RFM69_LISTENMODE
  RF69_Config.ListenModeActive=RFM69_LISTENMODE_DEFAULT_VALUE;
#endif
  
#if HAS_RFM69_TXonly
	RF69_Config.TXonly=RFM69_TXonly_DEFAULT_VALUE;
#if defined(STORE_CONFIGURATION) 
	getStoredValue((void*)&RF69_Config.TXonly, (const void*)&eeTXonly,1);
#endif
#endif
  
#if HAS_RFM69_TX_FORCED_DELAY
	lastRadioFrame=0; //delay between TX & RX->TX needed
#endif

#if HAS_RFM69_CMD_TUNNELING
	RF69_Config.TunnelingCMDQuery=RFM69_CMD_TUNNELING_DEFAULT_VALUE;
#if defined(STORE_CONFIGURATION) 
	getStoredValue((void*)&RF69_Config.TunnelingCMDQuery, (const void*)&eeTunnelingCMDQuery,1);
#endif
#endif

#if HAS_RFM69_CMD_TUNNELING==1 //Host
  RF69_Config.TunnelingSendBurst=false;
#endif

#if HAS_RFM69_CMD_TUNNELING==2 //Satellite
	safeHostID4Tunneling=RFM69_CMD_TUNNELING_HOSTID_DEFAULT_VALUE;
#if defined(STORE_CONFIGURATION) 
	getStoredValue((void*)&safeHostID4Tunneling, (const void*)&eesafeHostID4Tunneling,1);
#endif
	addToRingBuffer(MODULE_DATAPROCESSING,MODULE_DATAPROCESSING_OUTPUTTUNNEL,(const byte*)(RF69_Config.TunnelingCMDQuery ? "1" : "0"),1);
#endif
	
	RF69_Config.mode = RF69_MODE_STANDBY;
}

void myRFM69::resetRFM69() {

	if(RF69_Config.ResetPin==0) return;
	
	//Reset RFM69
	pinMode(RF69_Config.ResetPin, OUTPUT);
	
	digitalWrite(RF69_Config.ResetPin, HIGH);
	TIMING::delayMicroseconds(100);
	digitalWrite(RF69_Config.ResetPin, LOW);
	TIMING::delay(100);
	
	pinMode(RF69_Config.ResetPin, INPUT); //set to input for reducing power consumption
}

void myRFM69::initialize() {
	
	TIMING::configure();
	
 //set RFM69 SPI settings
 #if BASIC_SPI==1
  pinMode(SS, OUTPUT);
  SPI.begin(); 
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV4); //decided to slow down from DIV2 after SPI stalling in some instances, especially visible on mega1284p when RFM69 and FLASH chip both present 	
 #else
	//already initialized via mySpi Class
 #endif

	configure(RF69_Config.Protocol);
  cfgInterrupt(this, mySpi.getIntPin(RF69_Config.spi_id), mySpi.getIntState(RF69_Config.spi_id));

	#if HAS_RFM69_CMD_TUNNELING==2 && RFM69_CMD_TUNNELING_DEFAULT_VALUE //Satellite send initializing command to Host
	addToRingBuffer(MODULE_DATAPROCESSING_WAKE_SIGNAL, 0, NULL, 0);
	#endif
}

void myRFM69::configure(uint8_t protocol) {

	const unsigned char (*config)[2];

	if(protocol > ProtocolInfoLines || protocol <= 0) {
		DS_P("Protocol <"); DU(protocol,2); DS_P("> unknown."); DNL();
		return;	
	}
	
	resetRFM69();
	//rcCalibration(); //will automatically calibrated after RFM restart
	RF69_Config.Protocol = protocol;

	config = ProtocolInfo[protocol-1].config;
	
  do writeReg(REG_SYNCVALUE1, 0xAA); while (readReg(REG_SYNCVALUE1) != 0xAA);
	do writeReg(REG_SYNCVALUE1, 0x55); while (readReg(REG_SYNCVALUE1) != 0x55);
  
	for (byte i = 0; config[i][0] != 255; i++)
		writeReg(config[i][0], config[i][1]);

// Encryption deactivated because no continues TX Mode (Burst) for Satellites(ListenMode) is possible.
//	if(RF69_Config.Protocol==RFM69_PROTOCOL_MyProtocol) {
//		encrypt(RFM69_ENCRYPTKEY);
//	} else {
		encrypt(0);
//	}
	
#if HAS_RFM69_POWER_ADJUSTABLE
	uint8_t power = RF69_Config.powerLevel;
	setHighPower(RF69_Config.isRFM69HW); //called regardless if it's a RFM69W or RFM69HW
	RF69_Config.powerLevel = power;
	setPowerLevel(RF69_Config.powerLevel);
#else
	setHighPower(RF69_Config.isRFM69HW); //called regardless if it's a RFM69W or RFM69HW
	setMinPowerLevel();
#endif
	
	RF69_Config.PacketFormatVariableLength = (readReg(REG_PACKETCONFIG1) & RF_PACKET1_FORMAT_VARIABLE) ? true : false;
  
 #if HAS_RFM69_LISTENMODE || HAS_RFM69_TXonly
 	setMode(RF69_MODE_SLEEP);
 #else
  setMode(RF69_MODE_STANDBY);
 #endif
	while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady
}

void myRFM69::toggleRadioMode(uint8_t protocol, uint16_t *config) {
	
	if(!(*config)) { //save
		*config = (RF69_Config.Protocol << 8) ;
#if HAS_RFM69_LISTENMODE
		*config |= (RF69_Config.ListenModeActive ? (1<<7): 0) ;
#endif
#if HAS_RFM69_CMD_TUNNELING
		*config |= (RF69_Config.TunnelingCMDQuery ? (1<<5):0);
#endif
#if HAS_RFM69_CMD_TUNNELING==1 //Host
		*config |= (RF69_Config.TunnelingSendBurst ? (1<<6): 0); 
#endif

		configure(protocol);
	
	} else { //restore
		protocol = ((*config) >> 8);
		configure(protocol);

#if HAS_RFM69_CMD_TUNNELING
		RF69_Config.TunnelingCMDQuery = (((*config) & (1<<5)) ? true : false);
#endif
#if HAS_RFM69_CMD_TUNNELING==2 //Satellite
		if(RF69_Config.TunnelingCMDQuery)
			addToRingBuffer(MODULE_DATAPROCESSING,MODULE_DATAPROCESSING_OUTPUTTUNNEL,(const byte*)"1",1);
#endif
//		DU(protocol,0);DS_P(" Tunnel:");DU(RF69_Config.TunnelingCMDQuery,0);
#if HAS_RFM69_CMD_TUNNELING==1 //Host
		RF69_Config.TunnelingSendBurst = (((*config) & (1<<6)) ? true : false);
//		DS_P(" burst:");DS((RF69_Config.TunnelingSendBurst?"1":"0"));
#endif

		
#if HAS_RFM69_LISTENMODE
		RF69_Config.ListenModeActive = (((*config) & (1<<7)) ? true : false);		
//		DS_P(" listen:");DS((RF69_Config.ListenModeActive?"1":"0"));
#endif
//		DS("]\n");
	}
}

bool myRFM69::poll() {

#ifdef HAS_RFM69_TXonly
	if(RF69_Config.TXonly) return 0;
#endif

  if(receiveDone()) {
  	int rssi=DataStruct.RSSI;
  	bool valid=1;
  	bool debounced=1;

 	// check packet validity
		if(DEBUG) { //speed up
			DS_P("RAW["); DC(mySpi.getIdent(RF69_Config.spi_id));DS_P("] ");
			for(uint8_t i=0; i<DataStruct.PAYLOADLEN; i++) {
				DH2(DataStruct.DATA[i]);
			}
			DS_P(" [");DU(DataStruct.PAYLOADLEN,0);DS_P("]\n");
		}
  	if(	ProtocolInfo[RF69_Config.Protocol-1].transformRxData != NULL &&
  			ProtocolInfo[RF69_Config.Protocol-1].transformRxData(&DataStruct) == false) {//(DataStruct.DATA, &DataStruct.PAYLOADLEN, &DataOptions, &DataStruct.SENDERID) == false) {
			valid=0;
  	}
  			
 	// debounce command
	 	if(valid) debounced = DebounceRecvData(&DataStruct);

	// add RSSI value at the end of the buffer
		if (DataStruct.PAYLOADLEN < RF69_MAX_DATA_LEN) {
			DataStruct.DATA[DataStruct.PAYLOADLEN] = ((-rssi) << 1); //transform int to byte and add to DATA BUFFER
			DataStruct.PAYLOADLEN++;
		}
		
	//Debuging
		if(DEBUG) { //speed up
			DS_P("MSG[");DC(mySpi.getIdent(RF69_Config.spi_id));DS_P("] ");
			for(uint8_t i=0; i<DataStruct.PAYLOADLEN-1; i++) {
				DH2(DataStruct.DATA[i]);
			}
			DS_P(" [RSSI:");DC(mySpi.getIdent(RF69_Config.spi_id));DS_P(":");DI(rssi,0);DS_P("|");
			DU(DataStruct.XDATA_BurstTime,0); DS((DataStruct.XData_Config & XDATA_BURST) ? "ms] " : "] "); 
			if(!valid) { DS_P(" >> skipped"); }
			DNL();
		}

		if(!valid) return 1; //exit if message is not valid
		
		//wait till burst is over
		if(DataStruct.XDATA_BurstTime) {
		#if HAS_RFM69_LISTENMODE
			if(RF69_Config.ListenModeActive) {
				DFL();
				FKT_POWERDOWN(SLEEP_15Ms,(DataStruct.XDATA_BurstTime/15)+1); //+1 safety			
			} else
		#endif
			{	
				TIMING::delay(DataStruct.XDATA_BurstTime);
			}
		}

#if HAS_RFM69_TX_FORCED_DELAY
		lastRadioFrame = TIMING::millis();
#endif
 
#ifndef HAS_RFM69_SNIFF
	// DataOptions (ACK_RECEIVED, ACK_REQUESTED, CMD_RECEIVED, CMD_REQUESTED)
		if(DataStruct.XData_Config & XDATA_CMD_REQUEST) { //kann nicht debounced werden da RückgabeWert erwartet wird
		  addToRingBuffer(MODULE_DATAPROCESSING, 0, (const byte*)DataStruct.DATA, DataStruct.PAYLOADLEN-1 /*-1 ohne RSSI*/);

		} else if(DataStruct.XData_Config & XDATA_ACK_REQUEST) {

			if(DataStruct.PAYLOADLEN > 1) {	// Push msg to ring buffer
  	  	addToRingBuffer(MODULE_DATAPROCESSING, 0, (const byte*)DataStruct.DATA, DataStruct.PAYLOADLEN-1 /*-1 ohne RSSI*/);
  	  }

			sendACK(DataStruct.SENDERID); //send ACK after processing command, due to clearing DataStruct

//			byte buf[2];
//			buf[0] = mySpi.getIdent(RF69_Config.spi_id);
//			buf[1] = DataStruct.SENDERID;
//			addToRingBuffer(MODULE_DATAPROCESSING,MODULE_RFM69_SENDACK,(const byte*)buf,2); //send ACK after processing transmitted data -> reduce reaction timing in case of ListenMode  		
  		
		} else if((DataStruct.XData_Config & XDATA_CMD_RECEIVED) && debounced) {		
				// nicht durch den RingBuffer da sonst die falsche DeviceID voran gestellt wird
				DU(DataStruct.SENDERID,2);
				for(uint8_t i=0; i<(DataStruct.PAYLOADLEN-1); i++) {
						DC((char)DataStruct.DATA[i]);
				}
				//rssi = ((-DATA[PAYLOADLEN-1]) >> 1);
				DS_P(" [RSSI:");DC(mySpi.getIdent(RF69_Config.spi_id));DS_P(":");DI(rssi,0);DS_P("]");
				DNL();
				
		} else if(DataStruct.XData_Config & XDATA_ACK_RECEIVED) {
		
		} else if(!(DataStruct.XData_Config & (XDATA_CMD_RECEIVED | XDATA_CMD_REQUEST | XDATA_ACK_RECEIVED | XDATA_ACK_REQUEST))) {
#endif		
		  //print received command to Uart 
  		addToRingBuffer(MODULE_RFM69, mySpi.getIdent(RF69_Config.spi_id), (const byte*)DataStruct.DATA, DataStruct.PAYLOADLEN);
#ifndef HAS_RFM69_SNIFF
		}
#endif

		//receiveDone(); //speedup to be ready for next messages?? --> Führt dazu das das Gateway keine ACK mehr rechtzeitig empfängt...komisch
		return 1;
	}
	return 0;
}

inline bool myRFM69::DebounceRecvData(myRFM69_DATA *lDataStruct) {

    //word crc = ~0;
    bool debounced=true;
    word now = TIMING::millis();
    
  	//for (byte i = 0; i < lDataStruct->PAYLOADLEN; ++i)
	    //crc = _crc16_update(crc, lDataStruct->DATA[i]);
    // if different crc or too long ago, this cannot be a repeated packet
   	//if((lDataStruct->CheckSum == _tDebCRC) && (((now - _tDebCmd) < _minDebounceTime) || (_bDebMsgCnt == lDataStruct->MsgCnt && lDataStruct->MsgCnt!=0 )) ) {
   	if((lDataStruct->CheckSum == _tDebCRC) && ((TIMING::millis_since(_tDebCmd) < _minDebounceTime)) ) {
			debounced=false;
		}
		if(DEBUG) { DS_P("Debounce ");DU(TIMING::millis_since(_tDebCmd),0);if(!debounced) {DC('!');} DC('>');DU(_minDebounceTime,0);DS("\n"); }
    // save last values and decide whether to report this as a new packet
    _tDebCRC = lDataStruct->CheckSum;
    _tDebCmd = now;
    //_bDebMsgCnt = lDataStruct->MsgCnt;
    
    return debounced;
}

bool myRFM69::validdisplayData(RecvData *DataBuffer) {

	if((char)DataBuffer->DataTypeCode!=mySpi.getIdent(RF69_Config.spi_id) && (char)DataBuffer->DataTypeCode!='0')
		return 0; //not right spi id --> other rfm module
	return 1;
}

void myRFM69::displayData(RecvData *DataBuffer) {
	
	//if((uint8_t)DataBuffer->DataTypeCode!=mySpi.getIdent(spi_id) && (uint8_t)DataBuffer->DataTypeCode!=0)
	//if(!validdisplayData(DataBuffer))
	//	return; //not right spi id --> other rfm module

	DC(mySpi.getIdent(RF69_Config.spi_id)); //add Module-ID to recv packet
	
	switch(DataBuffer->ModuleID) {

		case MODULE_RFM69_MODEQUERY:
			DU(RF69_Config.Protocol,2);
			DS((readReg(REG_FRFMSB)==0x6C ? "4" : "8"));
			break;
		
		case MODULE_RFM69_OPTION_SEND:
			DC('s'); //print command
			DS((char*)DataBuffer->Data);
			break;
			
#if HAS_RFM69_POWER_ADJUSTABLE
		case MODULE_RFM69_OPTION_POWER:
			DC('p'); //print command
			DU(RF69_Config.powerLevel,2);
			break;		
#endif

#if HAS_RFM69_TEMPERATURE_READ
		case MODULE_RFM69_OPTION_TEMP:
			DC('t'); //print command
			DU(DataBuffer->Data[0],2);
			break;		
#endif
			
		default:
				//DC(mySpi.getIdent(spi_id)); //add Module-ID to recv packet
				DU(RF69_Config.Protocol,1);
				for(uint8_t i=0; i<(DataBuffer->DataSize-1); i++) {
					if(RF69_Config.Protocol == RFM69_PROTOCOL_MyProtocol || RF69_Config.Protocol == RFM69_PROTOCOL_HX2262)
						DC(DataBuffer->Data[i]);
					else
						DH2(DataBuffer->Data[i]);
				}
			
				int rssi = ((-DataBuffer->Data[DataBuffer->DataSize-1]) >> 1);
				DS_P(" [RSSI:");DC(mySpi.getIdent(RF69_Config.spi_id));DS_P(":");DI(rssi,0);DS_P("]"); 
			break;	
	}
}

void myRFM69::printHelp() {

	DS_P("\n ## RFM69 - ID:");	DC(mySpi.getIdent(RF69_Config.spi_id));	DS_P(" ##\n");
#if defined(RFM69_NO_OTHER_PROTOCOLS)
	DS_P(" Protocols:<1:myProtocol>\n");
#else
	DS_P(" Protocols:<1:myProtocol,2:HX2262,3:FS20,4:LaCrosse,5:ETH200,6:HomeMatic>\n");
#endif

#if INCLUDE_DEBUG_OUTPUT
	DS_P(" * [Reset]    R<ModNo>o\n");
	DS_P(" * [r/w-conf] R<ModNo>c<Addr:2><Value:2>\n");
#endif

#if !defined(RFM69_NO_OTHER_PROTOCOLS)
	DS_P(" * [RX-conf]  R<ModNo>r<Protocol>\n");
#endif

	DS_P(" * [RX-Proto] f<ModNo>\n");
	DS_P(" * [RX-Deb]   R<ModNo>d<ms>\n");

#if HAS_RFM69_POWER_ADJUSTABLE
	DS_P(" * [TX-Power] R<ModNo>p<0-31 W/CW, 0-51 HW>\n");
#endif

	DS_P(" * [TX-myPro] R<ModNo><TX-Cfg>1<Dev-ID:2><Data>\n");
	DS_P(" *                     TX-Cfg<s:Send,b:BurstSend>\n");

#if !defined(RFM69_NO_OTHER_PROTOCOLS)	
	DS_P(" * [TX-HX]    R<ModNo>s2<House:5><Device:5><<State:2>>\n");
	DS_P(" * [TX-ETH]   R<ModNo>s5<Addr:4><Cmd:2><Value:2>\n");
	DS_P(" * [TX-HM]    R<ModNo>s6<Hex>\n");
#endif

#if HAS_RFM69_CMD_TUNNELING
	DS_P(" * [Tun-CMD]  T<ModNo>c<1:on,0:off><opt.Host-ID>\n");
#endif

#if HAS_RFM69_CMD_TUNNELING==1 //Host
	DS_P(" * [TX-Burst] T<ModNo>b<1:on,0:off>\n");
#endif

#if HAS_RFM69_LISTENMODE
	DS_P(" * [RX-List]  R<ModNo>l<1:on,0:off>\n"); //Ultra Low Power RFM-Listen Mode
#endif

#if HAS_RFM69_TXonly
	DS_P(" * [TX-only]  R<ModNo>x<1:on,0:off>\n"); //Ultra Low Power RFM-Listen Mode
#endif

#if HAS_RFM69_TEMPERATURE_READ
	DS_P(" * [Temp]     R<ModNo>t\n");
#endif
}

extern const typeModuleInfo ModuleTab[]; //notwendig für MODULE_COMMAND_CHAR
void myRFM69::send(char *cmd, uint8_t typecode) {

	uint16_t buf=0;
	//uint16_t FrqShift=0; //needed for bursts to ListenMode clients
	char *p = cmd;
	char spiId = cmd[0];
	myRFM69_DATA lDataStruct;
	lDataStruct.SENDERID 			= DataStruct.SENDERID;
	lDataStruct.XData_Config	=	XDATA_NOTHING;
		
	cmd++; //Jump over ModuleID [1]
	p++;
	if(spiId != mySpi.getIdent(RF69_Config.spi_id) && spiId != '0') //valid SPI Module Number
		return;
	spiId=mySpi.getIdent(RF69_Config.spi_id); //in case of requested spiID=0
	//D_DS_P("valid SPI-ID <");D_DC(spiId);D_DS_P(">\n");
	
	//DS("<<");DS(cmd-1);DS(">>\n");
	//if(typecode==MODULE_RFM69_CONFIG_TEMP) typecode=MODULE_RFM69_OPTIONS; //workaround for RFM12 "O" and "F"

	switch(typecode) {

		case MODULE_RFM69_SENDACK:
			sendACK((byte)cmd[0]);
			break;
			
		case MODULE_RFM69_MODEQUERY:
			addToRingBuffer(MODULE_RFM69_MODEQUERY,spiId,NULL,0);
			break;

		case MODULE_RFM69_OPTIONS:
		
			switch(cmd[0]) {
			
#if INCLUDE_DEBUG_OUTPUT			
				case 'o':
					resetRFM69();
					//rcCalibration(); //will automatically calibrated after RFM restart
					//AFCCalibration();
					break;
				case 'c':
					cmd++;
					if(strlen(cmd)==0) {
						readAllRegs();
					} else if(strlen(cmd)==2) {
						readAllRegs(HexStr2uint8(cmd));//addr);
					} else {
						if(strlen(cmd)!=4) break;
						writeReg(HexStr2uint8(cmd),HexStr2uint8(cmd+2));						

					}
					break;
#endif
				
				case 'd': //00F1d100
					_minDebounceTime = atoi(cmd+1);
					D_DS_P("DebounceTime "); D_DU(_minDebounceTime,0); D_DS_P("\n");
					break;
					
#if !defined(RFM69_NO_OTHER_PROTOCOLS)
				case 'r':
					if(strlen(cmd+1)==0) break;
					configure(atoi(cmd+1));
					break;
#endif

				case 'b':
					lDataStruct.XData_Config = XDATA_BURST;
//					frequenceShift(1);
				case 's': //00F1s4010040CB				
					// ETH: learn: 00F1s401004000 day: 00F1s401004200 night: 00F1s401004300 off: 00F1s4010040CB  //00F1s401004000 -_> Data[20]: 6A A9 9555559555559555555555595555565AA655
					if(RF69_Config.Protocol != (cmd[1]-'0')) { //check if currently an other protocol is activ. If yes toggle the protocol during send phase
						buf=0;
						toggleRadioMode((cmd[1]-'0'),&buf);
					}
					cmd+=2; //jump over command 's' or 'b' and protocol number
					if(ProtocolInfo[RF69_Config.Protocol-1].transformTxData != NULL) {
						//lDataStruct.MsgCnt = (_RFM69TXMsgCounter==0xFF ? 1 : ++_RFM69TXMsgCounter);
						lDataStruct.XData_Config |= XDATA_ACK_REQUEST;
						ProtocolInfo[RF69_Config.Protocol-1].transformTxData(cmd,&lDataStruct);
						
						if(lDataStruct.PAYLOADLEN) //valid data if length is not 0
							send(&lDataStruct);
						
//						if(lDataStruct.XData_Config & XDATA_BURST) frequenceShift(0);
						
						if(buf) { //reset RadioConfig
							toggleRadioMode(0,&buf);
						}
						
						if(lDataStruct.XData_Config & XDATA_CMD_ECHO) { // needed for ETH due to no command confirmation
							cmd--; //add protocol number for right cmd echo
							addToRingBuffer(MODULE_RFM69_OPTION_SEND,spiId,(const byte*)cmd,strlen(cmd));
						}
					}
					break;

#if HAS_RFM69_LISTENMODE			
				case 'l':
					RF69_Config.ListenModeActive=(cmd[1]=='0'?0:1);
//					frequenceShift(RF69_Config.ListenModeActive);
					setMode(RF69_MODE_SLEEP);
					break;
#endif

#if HAS_RFM69_TXonly
				case 'x':
					if(cmd[1]=='0') {
						RF69_Config.TXonly = false;
						setMode(RF69_MODE_STANDBY);
					} else {
						RF69_Config.TXonly = true;
						setMode(RF69_MODE_SLEEP);
					}
				#if defined(STORE_CONFIGURATION) 
					StoreValue((void*)&RF69_Config.TXonly,(void*)&eeTXonly,1);
				#endif					
					break;
#endif

#if HAS_RFM69_POWER_ADJUSTABLE					
				case 'p':
					if(strlen(cmd+1)) {
						setPowerLevel(atoi(cmd+1));
					}
					addToRingBuffer(MODULE_RFM69_OPTION_POWER,spiId,(unsigned char*)((uint8_t*)&buf),1); // -1 = user cal factor, adjust for correct ambient
					break;
#endif

#if HAS_RFM69_TEMPERATURE_READ					
				case 't':
					buf = (uint16_t)readTemperature(-1);
					addToRingBuffer(MODULE_RFM69_OPTION_TEMP,spiId,(unsigned char*)((uint8_t*)&buf),1); // -1 = user cal factor, adjust for correct ambient
					break;
#endif
			}
			break;
			
#if HAS_RFM69_CMD_TUNNELING
		case MODULE_RFM69_TUNNELING: //Request CMD on Satellites
			if(RF69_Config.Protocol != RFM69_PROTOCOL_MyProtocol) {
				D_DS_P("no tunneling support.\n");
				break;
			}
			
			switch(cmd[0]) {
			
				case 'c': //activate Tunneling function
					RF69_Config.TunnelingCMDQuery=((cmd[1]=='0')?false:true);
				#if defined(STORE_CONFIGURATION) 
					StoreValue((void*)&RF69_Config.TunnelingCMDQuery,(void*)&eeTunnelingCMDQuery,1);
				#endif
					
			#if HAS_RFM69_CMD_TUNNELING==2 //Satellite						
					if(strlen(cmd)>2) { //safe Host ID
						safeHostID4Tunneling = atoi(cmd+2);
					#if defined(STORE_CONFIGURATION) 
					  StoreValue((void*)&safeHostID4Tunneling,(void*)&eesafeHostID4Tunneling,1);
				  #endif
					} else {
						DS_P("Tun-Host-ID: ");DU(safeHostID4Tunneling,0);DNL();
					}
					addToRingBuffer(MODULE_DATAPROCESSING,MODULE_DATAPROCESSING_OUTPUTTUNNEL,(const byte*)(RF69_Config.TunnelingCMDQuery ? "1" : "0"),1);						
			#endif
					
					break;
			
			#if HAS_RFM69_CMD_TUNNELING==1 //Host				
				case 'b': //activate TX Bursts
					RF69_Config.TunnelingSendBurst=((cmd[1]=='0')?false:true); //send burst on
					break;
			#endif

				default: //Command to tunnel (Host<->Satellite)
					if(!RF69_Config.TunnelingCMDQuery) { D_DS_P("Satellite CMD request not active.\n"); return; } // off
					
			#if HAS_RFM69_CMD_TUNNELING==1 // HOST
					if(RF69_Config.TunnelingSendBurst) { 
						lDataStruct.XData_Config = XDATA_BURST;
//						frequenceShift(1);
					}
			#endif
			
					lDataStruct.XData_Config |= cmd[0];

					if(!(lDataStruct.XData_Config & XDATA_CMD_REQUEST || lDataStruct.XData_Config & XDATA_ACK_REQUEST)) {

					#if HAS_RFM69_CMD_TUNNELING==2 //Satellite
						if(safeHostID4Tunneling!=DEVICE_ID_BROADCAST) {
							cmd[1] = '0' + (safeHostID4Tunneling/10);
							cmd[2] = '0' + (safeHostID4Tunneling%10);
						} else {
					#endif

						cmd[1] = '0' + (lDataStruct.SENDERID/10);
						cmd[2] = '0' + (lDataStruct.SENDERID%10);						

					#if HAS_RFM69_CMD_TUNNELING==2 //Satellite
						}
					#endif
					}
					//no ACK requested 
					ProtocolInfo[RFM69_PROTOCOL_MyProtocol-1].transformTxData(cmd+1,&lDataStruct);
					send(&lDataStruct);
//			#if HAS_RFM69_CMD_TUNNELING==1 // HOST
//					if(RF69_Config.TunnelingSendBurst) { 
//						frequenceShift(0);
//					}
//			#endif					
			}
			break;
#endif			
	}
}

//#if HAS_RFM69_CMD_TUNNELING
////Frequence Shift for low noise on ListenMode satellites
//void myRFM69::frequenceShift(bool activ) {

////	uint16_t FrqShift = ((readReg(REG_FRFMID) << 8 ) | readReg(REG_FRFLSB)) + (RF69_RX_LISTEN_FRQ_SHIFT_OFFSET * (activ ? 1 : -1));
//	//Frequence Shift for low noise on ListenMode satellites
////	writeReg(REG_FRFMID,(FrqShift>>8));
////	writeReg(REG_FRFLSB,FrqShift);	
//}
//#endif

/******************************************************************************************************/
/****************************************** Driver Functions ******************************************/
/******************************************************************************************************/

/*
#define RF69_FSTEP 61.03515625 // == FXOSC/2^19 = 32mhz/2^19 (p13 in DS)

//return the frequency (in Hz)
uint32_t RFM69::getFrequency()
{
return RF69_FSTEP * (((uint32_t)readReg(REG_FRFMSB)<<16) + ((uint16_t)readReg(REG_FRFMID)<<8) + readReg(REG_FRFLSB));
}

//set the frequency (in Hz)
void RFM69::setFrequency(uint32_t freqHz)
{
//TODO: p38 hopping sequence may need to be followed in some cases
freqHz /= RF69_FSTEP; //divide down by FSTEP to get FRF
writeReg(REG_FRFMSB, freqHz >> 16);
writeReg(REG_FRFMID, freqHz >> 8);
writeReg(REG_FRFLSB, freqHz);
}

void myRFM69::setBitRate(uint32_t uBR) {

	uBR = ((XTALFREQ + (uBR/2)) / uBR);
	writeReg(REG_BITRATEMSB, (uBR >> 8) & 0xFF); //LaCrosse 17.24 kbps
  writeReg(REG_BITRATELSB, (uBR) & 0xFF);
}

void myRFM69::setBandwith(uint32_t uBW) {

	writeReg(REG_RXBW, uBW);
}

void myRFM69::sleep() {
  setMode(RF69_MODE_SLEEP);
}

void myRFM69::setAddress(byte addr)
{
  _address = addr;
	writeReg(REG_NODEADRS, _address);
}

void RFM69::setNetwork(byte networkID)
{
  writeReg(REG_SYNCVALUE2, networkID);
}
*/
// set output power: 0=min, 31=max
// this results in a "weaker" transmitted signal, and directly results in a lower RSSI at the receiver
/*void myRFM69::setPowerLevel(byte powerLevel)
{
  _powerLevel = (powerLevel > 31 ? 31 : powerLevel);
  writeReg(REG_PALEVEL, (readReg(REG_PALEVEL) & 0xE0) | _powerLevel);
}
*/

#if HAS_RFM69_POWER_ADJUSTABLE
// set output power: 0=min, 31=max (for RFM69W or RFM69CW), 0-31 or 32->51 for RFM69HW (see below)
// this results in a "weaker" transmitted signal, and directly results in a lower RSSI at the receiver
void myRFM69::setPowerLevel(byte powerLevel)
{
	// TWS Update: allow power level selections above 31.  Select appropriate PA based on the value
		
  RF69_Config.powerBoost = (powerLevel >= 50);
  
	if (!RF69_Config.isRFM69HW || powerLevel < 32) {			// use original code without change
    RF69_Config.powerLevel = (powerLevel > 31 ? 31 : powerLevel);
    writeReg(REG_PALEVEL, (readReg(REG_PALEVEL) & 0xE0) | RF69_Config.powerLevel);
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
	  writeReg(REG_OCP, (RF69_Config.PA_Reg==0x60) ? RF_OCP_OFF : RF_OCP_ON);
    writeReg(REG_PALEVEL, powerLevel | RF69_Config.PA_Reg);
  }
}
#endif

void myRFM69::setMinPowerLevel() {

	if (!RF69_Config.isRFM69HW) {
		writeReg(REG_PALEVEL, (readReg(REG_PALEVEL) & 0xE0));
	} else {
	  writeReg(REG_OCP, RF_OCP_ON);
    writeReg(REG_PALEVEL, (0x20 & 0x0f) | 0x20);	
	}
}

/*
// ON  = disable filtering to capture all frames on network
// OFF = enable node+broadcast filtering to capture only frames sent to this/broadcast address
void myRFM69::promiscuous(bool onOff) {
  _promiscuousMode=onOff;
  //writeReg(REG_PACKETCONFIG1, (readReg(REG_PACKETCONFIG1) & 0xF9) | (onOff ? RF_PACKET1_ADRSFILTERING_OFF : RF_PACKET1_ADRSFILTERING_NODEBROADCAST));
}
*/
#if HAS_RFM69_POWER_ADJUSTABLE
void myRFM69::setHighPower(bool onOff, byte PA_ctl) {
  RF69_Config.isRFM69HW = onOff;
  	
  writeReg(REG_OCP, (RF69_Config.isRFM69HW && PA_ctl==0x60) ? RF_OCP_OFF : RF_OCP_ON);
  if (RF69_Config.isRFM69HW) { //turning ON based on module type 
  	RF69_Config.powerLevel = readReg(REG_PALEVEL) & 0x1F; // make sure internal value matches reg
  	RF69_Config.powerBoost = (PA_ctl == 0x60);
  	RF69_Config.PA_Reg = PA_ctl;
    writeReg(REG_PALEVEL, RF69_Config.powerLevel | PA_ctl ); //TWS: enable selected P1 & P2 amplifier stages
  } else {
    RF69_Config.PA_Reg = RF_PALEVEL_PA0_ON;				// TWS: save to reflect register value
    writeReg(REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF69_Config.powerLevel); //enable P0 only
  }
}
#else
// for RFM69HW only: you must call setHighPower(true) after initialize() or else transmission won't work
void myRFM69::setHighPower(bool onOff, byte PA_ctl) {
  RF69_Config.isRFM69HW = onOff;
  writeReg(REG_OCP, RF69_Config.isRFM69HW ? RF_OCP_OFF : RF_OCP_ON);
  if (RF69_Config.isRFM69HW) // turning ON
    writeReg(REG_PALEVEL, (readReg(REG_PALEVEL) & 0x1F) | RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON); // enable P1 & P2 amplifier stages
  else
    writeReg(REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | 31); // enable P0 only with max TX powerLevel
}
#endif

void myRFM69::setHighPowerRegs(bool onOff) {
#if HAS_RFM69_POWER_ADJUSTABLE
	if ((0x60 != (readReg(REG_PALEVEL) & 0xe0)) || !RF69_Config.powerBoost)		// TWS, only set to high power if we are using both PAs... and boost range is requested.
		onOff = false;
#endif		
  writeReg(REG_TESTPA1, onOff ? 0x5D : 0x55);
  writeReg(REG_TESTPA2, onOff ? 0x7C : 0x70);
}

#if HAS_RFM69_TEMPERATURE_READ
byte myRFM69::readTemperature(byte calFactor)  //returns centigrade
{
  setMode(RF69_MODE_STANDBY);
  writeReg(REG_TEMP1, RF_TEMP1_MEAS_START);
  while ((readReg(REG_TEMP1) & RF_TEMP1_MEAS_RUNNING));
  return ~readReg(REG_TEMP2) + COURSE_TEMP_COEF + calFactor; //'complement'corrects the slope, rising temp = rising val
}												   	  // COURSE_TEMP_COEF puts reading in the ballpark, user can add additional correction
#endif

/* not used
void myRFM69::rcCalibration()
{
  writeReg(REG_OSC1, RF_OSC1_RCCAL_START);
  while ((readReg(REG_OSC1) & RF_OSC1_RCCAL_DONE) == 0x00);
}

void myRFM69::AFCCalibration() {
  //writeReg(REG_AFCFEI, RF_AFCFEI_AFC_CLEAR | RF_AFCFEI_AFC_START | RF_AFCFEI_AFCAUTO_OFF | RF_AFCFEI_AFCAUTOCLEAR_OFF);
  //while ((readReg(REG_AFCFEI) & RF_AFCFEI_AFC_DONE) == 0x00);
	
	writeReg(REG_AFCFEI, RF_AFCFEI_FEI_START);
	while ((readReg(REG_AFCFEI) & RF_AFCFEI_FEI_DONE) == 0x00);
  
  //RF_AFCFEI_AFCAUTO_ON -> 1 → AFC is performed each time Rx mode is entered
  //writeReg(REG_AFCFEI, RF_AFCFEI_AFCAUTO_ON | RF_AFCFEI_AFCAUTOCLEAR_OFF);
}
*/

// To enable encryption: radio.encrypt("ABCDEFGHIJKLMNOP");
// To disable encryption: radio.encrypt(null) or radio.encrypt(0)
// KEY HAS TO BE 16 bytes !!!
void myRFM69::encrypt(const char* key) {
  setMode(RF69_MODE_STANDBY); //must be in Standby
  if (key!=0)
  {
  	 #if BASIC_SPI==1
    	select();
    	SPI.transfer(REG_AESKEY1 | 0x80);
    	for (byte i = 0; i<16; i++)
      	SPI.transfer(key[i]);
    	unselect();
    #else
    	mySpi.xferBegin(RF69_Config.spi_id);
    	mySpi.xfer_byte(REG_AESKEY1 | 0x80);
    	for (byte i = 0; i<16; i++)
      	mySpi.xfer_byte(key[i]); //SPI.transfer(key[i]);
      mySpi.xferEnd(RF69_Config.spi_id);
    #endif
  }
  writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFE) | (key ? 1 : 0));
}

//for debugging
#if INCLUDE_DEBUG_OUTPUT
void myRFM69::readAllRegs(byte addr)
{
  byte regVal;
	
	if(addr!=0) {

  	mySpi.xferBegin(RF69_Config.spi_id);
  	mySpi.xfer_byte(addr & 0x7f);	// send address + r/w bit
  	regVal = mySpi.xfer_byte(0);
  	mySpi.xferEnd(RF69_Config.spi_id);
	
	  DH(addr, 2);
	  DS(" - ");
	  DH(regVal,2);
	  DS(" - ");
	  DB(regVal);	
	 	DNL();

	} else {
		for (byte regAddr = 1; regAddr <= 0x4F; regAddr++)
		{
			#if BASIC_SPI==1
		  	select();
		  	SPI.transfer(regAddr & 0x7f);	// send address + r/w bit
		    regVal = SPI.transfer(0);
		  	unselect();
		  #else
		  	mySpi.xferBegin(RF69_Config.spi_id);
		  	mySpi.xfer_byte(regAddr & 0x7f);	// send address + r/w bit
		  	regVal = mySpi.xfer_byte(0);
		  	mySpi.xferEnd(RF69_Config.spi_id);
			#endif

		  DH(regAddr, 2);
	  	DS(" - ");
		  DH(regVal,2);
		 	DS(" - ");
		  DB(regVal);
		  DNL();
		}
	}
}
#endif

/********************* Transmitter Functions *********************/
//canSend Erkennung ist besser bei hohen REG_RSSITHRESH
bool myRFM69::canSend() {

	#if HAS_RFM69_TX_FORCED_DELAY 
		if(TIMING::millis_since(lastRadioFrame) < HAS_RFM69_TX_FORCED_DELAY) {
			//Auskommentiert um ggf. keinen laufenden Empfang abzubrechen.
			//#if HAS_RFM69_LISTENMODE //Satellite --> delay between TX needed
		 	//setMode(RF69_MODE_SLEEP);
		 	//TIMING::delay(HAS_RFM69_TX_FORCED_DELAY-(TIMING::millis() - lastRadioFrame));
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
			  
	 #if HAS_RFM69_LISTENMODE || HAS_RFM69_TXonly
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
/*
void myRFM69::send(const void* buffer, byte bufferSize, uint16_t repeats, bool singleFrame) {

	byte *b = (byte*)buffer;
	D_DS_P("Msg Transformed: ");
	for(uint8_t i=0; i<bufferSize; i++) {
		D_DH2(b[i]);
	}
	D_DS_P(" <");D_DU(repeats,0);D_DS_P(">\n");

	bool _ListenMode=0;
#if HAS_RFM69_LISTENMODE
	bool _ListenMode = RF69_Config.ListenModeActive;
	RF69_Config.ListenModeActive=0;
	setMode(RF69_MODE_RX); //Switching to continues RX..needed for RSSI reading in canSend()
#endif

  writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks
	unsigned long now = TIMING::millis();
  while (!canSend(_ListenMode) && (TIMING::millis()-now) < RF69_CSMA_LIMIT_MS) receiveDone(); //receiveBegin(); //??Begin führt zu einem kleineren Delay als receiveDone(); besonders ohne ListenMode//
 	if((TIMING::millis()-now) >= RF69_CSMA_LIMIT_MS_DEBUG) { D_DS_P("CSMA ");D_DU(TIMING::millis()-now,0);D_DS("\n"); }
	
	if(repeats>0 && !singleFrame) {
		//DS("Large Frame\n");
	  sendLargeFrame(buffer, bufferSize, repeats);
	} else {
		//DS("Small Frame\n");
		sendFrame(buffer, bufferSize, repeats);
	}

#if HAS_RFM69_LISTENMODE	
	RF69_Config.ListenModeActive=_ListenMode;
#endif	
	//setMode(RF69_MODE_RX,true,true);
}
*/

// should be called immediately after reception in case sender wants ACK
void myRFM69::sendACK(byte senderID, bool CMD, bool ACK) {
	
	char target[3];
	target[0] = '0' + senderID/10;
	target[1] = '0' + senderID%10;
	target[2] = 0x0;
	myRFM69_DATA lDataStruct;// = {0};
	lDataStruct.XData_Config	= (CMD ? XDATA_CMD_RECEIVED : XDATA_NOTHING) | (ACK ? XDATA_ACK_RECEIVED : XDATA_NOTHING);
	//lDataStruct.MsgCnt = (_RFM69TXMsgCounter==0xFF ? 1 : ++_RFM69TXMsgCounter);
	
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

void myRFM69::send(myRFM69_DATA *lDataStruct) {

	byte *b = (byte*)lDataStruct->DATA;
	if(DEBUG) {
		DS_P("TX: ");
		for(uint8_t i=0; i<lDataStruct->PAYLOADLEN; i++) {
			DH2(b[i]);
		}
		DS_P(" <");DU(((lDataStruct->XDATA_Repeats==0)?lDataStruct->XDATA_BurstTime:lDataStruct->XDATA_Repeats),0);DC(lDataStruct->XData_Config & XDATA_BURST ? 'b' : 's');DS_P(">\n");
	}

#if HAS_RFM69_LISTENMODE
	bool _ListenMode=0;
	_ListenMode = RF69_Config.ListenModeActive;
	RF69_Config.ListenModeActive=0;
	setMode(RF69_MODE_RX); //Switching to continues RX..needed for RSSI reading in canSend()
#endif

	//prepare to send and wait till RSSI is just noise, no other transmissions active
  writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks

#if HAS_RFM69_LISTENMODE
  //increase RSSIThreshold for better noise detection
  byte rssiThreshold = readReg(REG_RSSITHRESH);
  writeReg(REG_RSSITHRESH,RF69_RX_MAX_RSSITHRESH);
#endif

	unsigned long startTime = TIMING::millis();
  while (!canSend() && TIMING::millis_since(startTime) < RF69_CSMA_LIMIT_MS) receiveDone(); //receiveBegin(); //??Begin führt zu einem kleineren Delay als receiveDone(); besonders ohne ListenMode//
	if(DEBUG & (TIMING::millis_since(startTime) >= RF69_CSMA_LIMIT_MS_DEBUG)) { DS_P("CSMA ");DU(TIMING::millis_since(startTime),0);DS("ms\n"); }
	 
#if HAS_RFM69_LISTENMODE
	writeReg(REG_RSSITHRESH,rssiThreshold); //reset Threshold value
#endif
	
	bool gotACK=false;
  for (uint8_t i = 0; i <= RF69_SEND_RETRIES; i++) {
  
#if HAS_RFM69_ADC_NOISE_REDUCTION
	  FKT_ADC_OFF;
#endif
		sendFrame(lDataStruct); //sent a BURST or SINGLE Message
#if HAS_RFM69_ADC_NOISE_REDUCTION
		FKT_ADC_ON;
#endif
		
		if(lDataStruct->XData_Config & (XDATA_ACK_REQUEST | XDATA_CMD_REQUEST)) {	//wait for ACK_RECEIVED or CMD_RECEIVED
			//if(!i) startTime = TIMING::millis();
		  //unsigned long sentTime = TIMING::millis();
		  startTime = TIMING::millis();
			while (TIMING::millis_since(startTime) <= RF69_SEND_RETRY_WAITTIME) {
			 	if(ACKReceived(lDataStruct->TARGETID,lDataStruct->XData_Config & XDATA_ACK_REQUEST, lDataStruct->XData_Config & XDATA_CMD_REQUEST)) {
			   	//DU(lDataStruct->TARGETID,2);(lDataStruct->XData_Config & XDATA_ACK_REQUEST) ? DS_P("ACK") : DS_P("CMD");
			   	//DU(lDataStruct->CheckSum,0);DC(' ');DU(TIMING::millis()-startTime,0);DS("ms-");DU(i,0);DNL();
			   	DU(lDataStruct->TARGETID,2);(lDataStruct->XData_Config & XDATA_ACK_REQUEST) ? DS_P(" ACK ") : DS_P(" CMD ");DU(TIMING::millis_since(startTime),0);DS("ms-");DU(i,0);DNL();
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

#if HAS_RFM69_LISTENMODE
	RF69_Config.ListenModeActive=_ListenMode;
#endif
}

/*
 XDATA_BURST																											--> LongFrame mit XDATA_Repeats
 XDATA_SYNC XDATA_PREAMBLE 																				--> simples senden mit XDATA_Repeats
*/
void myRFM69::sendFrame(myRFM69_DATA *lDataStruct) {

//	byte BurstMode	= (lDataStruct->XData_Config & XDATA_BURST);
//	byte BurstTime = (lDataStruct->XDATA_BurstTime>0);
	byte bufferSize	=	lDataStruct->PAYLOADLEN;
	
	if((lDataStruct->XData_Config & XDATA_BURST) && (lDataStruct->XData_Config & XDATA_BURST_INFO_APPENDIX)) bufferSize-=2; //add two BurstDelay bytes to payloadlenght byte 
	uint32_t txStart=0;
	uint32_t txBusy=0;
	
  //turn off receiver to prevent reception while filling fifo
 	setMode(RF69_MODE_STANDBY);  // no sleep mode possible due to not working BURST TX in SleepMode

	while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady
	writeReg(REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN); //clear FIFO
 	writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00); // DIO0 is "Packet Sent"
  if (bufferSize > RF69_MAX_DATA_LEN) bufferSize = RF69_MAX_DATA_LEN;


  //Turn OFF Sync Words if requested
  if(!(lDataStruct->XData_Config & XDATA_SYNC)) writeReg(REG_SYNCCONFIG,readReg(REG_SYNCCONFIG) & (~RF_SYNC_ON));
  //Turn OFF Preamble Words if requested
	uint16_t nPreamble	=	0; //number of preamble bytes
  if(!(lDataStruct->XData_Config & XDATA_PREAMBLE)) {
  	nPreamble = (readReg(REG_PREAMBLEMSB) << 8) | readReg(REG_PREAMBLELSB); //number of Preamble bytes
  	writeReg(REG_PREAMBLEMSB,0x00);
  	writeReg(REG_PREAMBLELSB,0x00);
  }
  
  //send Frame
  uint16_t cycles=0;
  txStart = TIMING::millis();
  uint16_t diff = (uint16_t) lDataStruct->XDATA_BurstTime - TIMING::millis_since(txStart);

	while(cycles <= ( (lDataStruct->XDATA_BurstTime>0) ? lDataStruct->XDATA_BurstTime : lDataStruct->XDATA_Repeats)) { //also valid for burst mode because one cycle always takes more than 1ms

		mySpi.xferBegin(RF69_Config.spi_id);
		mySpi.xfer_byte(REG_FIFO | 0x80); 				//set FIFO to TX Modus
																							//preamble and sync pattern in burst mode will be added by the radio module
		for (byte i = 0; i < bufferSize; i++)  { 	//Data to FIFO
		  mySpi.xfer_byte(lDataStruct->DATA[i]);
		}
		
		//BurstMode add delay info at the end of the payload for ListenMode clients
		if((lDataStruct->XData_Config & XDATA_BURST) && (lDataStruct->XData_Config & XDATA_BURST_INFO_APPENDIX)) { 
			mySpi.xfer_byte((byte)(diff & 0xff));
			mySpi.xfer_byte((byte)(diff >> 8));
		}
		mySpi.xferEnd(RF69_Config.spi_id);

		if(!cycles || !(lDataStruct->XData_Config & XDATA_BURST)) {
			setMode(RF69_MODE_TX); //push first data frame into FIFO while rfm69 is in standby
		}
		
		txBusy=TIMING::millis();
		if((lDataStruct->XData_Config & XDATA_BURST) && (lDataStruct->XData_Config & XDATA_BURST_INFINITY)) { 
			//needed for ETH, infinity PackageMode. Fillup FIFO before it is empty!
			while ((readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_FIFOLEVEL) != 0x00 && TIMING::millis_since(txBusy) < RF69_TX_LIMIT_MS);
		} else {
			while(digitalRead(mySpi.getIntPin(RF69_Config.spi_id)) == 0 && TIMING::millis_since(txBusy) < RF69_TX_LIMIT_MS); //wait till TX is done
		}
		if(DEBUG & (TIMING::millis_since(txBusy) >= RF69_CSMA_LIMIT_MS_DEBUG)) { DS_P("TX ");DU(TIMING::millis_since(txBusy),0);DS("ms\n"); }
		
		//done?		
		if((lDataStruct->XData_Config & XDATA_BURST) && (lDataStruct->XDATA_BurstTime>0)) {
			if( (diff = TIMING::millis_since(txStart)) > lDataStruct->XDATA_BurstTime )  //check time during burst
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

 #if HAS_RFM69_TX_FORCED_DELAY
	lastRadioFrame = TIMING::millis();
 #endif
		
 #if HAS_RFM69_LISTENMODE || HAS_RFM69_TXonly
 	setMode(RF69_MODE_SLEEP);
 #else
  setMode(RF69_MODE_STANDBY);
 #endif
	
 //Turn back on Sync Words if requested
  if(!(lDataStruct->XData_Config & XDATA_SYNC)) writeReg(REG_SYNCCONFIG,readReg(REG_SYNCCONFIG) | RF_SYNC_ON);
 //Turn back on Preamble Words if requested
  if(!(lDataStruct->XData_Config & XDATA_PREAMBLE) && nPreamble) {
		writeReg(REG_PREAMBLEMSB,(byte)(nPreamble>>8));
  	writeReg(REG_PREAMBLELSB,(byte)nPreamble);  
  }
}

// AA AA AA 6A A9 95 55 55 95 55 55 95 55 55 55 55 59 A6 5A A6 65 69 6A 1 
//					6A A9 95 55 55 95 55 55 95 55 55 55 55 59 A6 5A A6 65 69 6A 1 
//					6A A9 95 55 55 95 55 55 95 55 55 55 55 59 A6 5A A6 65 69 6A 1 AA AA
/*
void myRFM69::sendLargeFrame(const void* buffer, byte bufferSize, uint16_t repeats) {

  setMode(RF69_MODE_STANDBY); //turn off receiver to prevent reception while filling fifo
	while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady
	writeReg(REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN); //clear FIFO
  writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00); // DIO0 is "Packet Sent"

  if (bufferSize > RF69_MAX_DATA_LEN) bufferSize = RF69_MAX_DATA_LEN;

	// no need to wait for transmit mode to be ready since its handled by the radio 
	//setMode(RF69_MODE_TX); // vorab TX setzen geht nur wenn RF_FIFOTHRESH_TXSTART_FIFOTHRESH konfiguriert wurde
	for(uint16_t e=0; e<=repeats; e++) {
		if(e==1) setMode(RF69_MODE_TX); //first push data while rfm69 is in standby
		while ((readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_FIFOLEVEL) != 0x00); // Wait till Threshold is not exceeded / Does not work in Sleep_Mode!
			
		mySpi.xferBegin(RF69_Config.spi_id);
		mySpi.xfer_byte(REG_FIFO | 0x80); //set FIFO to TX Modus

		for (byte i = 0; i < bufferSize; i++)  { //if(!i && !e && DEBUG) printRam();
		  mySpi.xfer_byte(((byte*)buffer)[i]);
		}
		mySpi.xferEnd(RF69_Config.spi_id);  
	}
	
	// no need to wait for transmit mode to be ready since its handled by the radio 
	//setMode(RF69_MODE_TX);
	unsigned long txStart = TIMING::millis();
	while (digitalRead(mySpi.getIntPin(RF69_Config.spi_id)) == 0 && (TIMING::millis()-txStart) < RF69_TX_LIMIT_MS); //wait for DIO0 to turn HIGH signalling transmission finish
  //while (readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PACKETSENT == 0x00); // Wait for ModeReady
 #if !HAS_RFM69_LISTENMODE
 	setMode(RF69_MODE_STANDBY);  
 #else
  setMode(RF69_MODE_SLEEP);
 #endif
//printRam();DC('e');DNL();
}

void myRFM69::sendFrame(const void* buffer, byte bufferSize, uint16_t repeats) {

  //turn off receiver to prevent reception while filling fifo
 #if !HAS_RFM69_LISTENMODE
 	setMode(RF69_MODE_STANDBY);  
 #else
  setMode(RF69_MODE_SLEEP);
 #endif

	while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady
	writeReg(REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN); //clear FIFO
  writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00); // DIO0 is "Packet Sent"
  if (bufferSize > RF69_MAX_DATA_LEN) bufferSize = RF69_MAX_DATA_LEN;

	for(uint8_t i=0; i<=repeats; i++) {
		#if BASIC_SPI==1
		  select();
		  SPI.transfer(REG_FIFO & 0x80);
		#else	
			//write to FIFO
			mySpi.xferBegin(RF69_Config.spi_id);
			mySpi.xfer_byte(REG_FIFO | 0x80); //set FIFO to TX Modus	
		#endif

		for (byte i = 0; i < bufferSize; i++) {
	 	#if BASIC_SPI==1
	 		SPI.transfer(((byte*)buffer)[i]);
	 	#else	
		  mySpi.xfer_byte(((byte*)buffer)[i]);
		#endif
		}
	
		#if BASIC_SPI==1
			unselect();
		#else
			mySpi.xferEnd(RF69_Config.spi_id);
		#endif
		
		// no need to wait for transmit mode to be ready since its handled by the radio 
		setMode(RF69_MODE_TX);

		unsigned long txStart = TIMING::millis();
		while (digitalRead(mySpi.getIntPin(RF69_Config.spi_id)) == 0 && TIMING::millis()-txStart < RF69_TX_LIMIT_MS); //wait for DIO0 to turn HIGH signalling transmission finish
	 #if !HAS_RFM69_LISTENMODE
	 	setMode(RF69_MODE_STANDBY);  
	 #else
		setMode(RF69_MODE_SLEEP);
	 #endif
//printRam(); 
	}
}
*/

//Durch Define ersetzt im Speicher zu sparen 25460
//uint8_t myRFM69::mRepeatsInListening(byte packetLen) {

//	packetLen += (readReg(REG_PREAMBLEMSB) <<8) | readReg(REG_PREAMBLELSB); //add Preamble
//	
// 	uint16_t bitrate = (uint16_t)(XTALFREQ / (uint32_t)(((readReg(REG_BITRATEMSB) & 0x3F) << 8) | readReg(REG_BITRATELSB))); //b/ms=kb/s
//	int8_t timePerPacket = (uint8_t)((packetLen*8)/(bitrate/1000) + 0.5); //+0,5 immer aufrunden
//	byte conf = readReg(REG_LISTEN1);
//	uint16_t TimePeriode = /*idle*/ ( (pow(64,(conf >> 6) & 0x03) * readReg(REG_LISTEN2)) +  /* Rx */ (pow(64,(conf >> 4) & 0x03) * readReg(REG_LISTEN3)) ) / 1000;
//	uint8_t retries = (uint8_t)(TimePeriode/timePerPacket + 1);
//	
//	return retries;
//}

/*
bool myRFM69::sendWithRetry(myRFM69_DATA *sData, byte retries, uint16_t retryWaitTime) {

	unsigned long sentRetryTime = 0;
	myRFM69_DATA lDataStruct;
	memcpy((void*)&lDataStruct,(const void*)sData,sizeof(lDataStruct)); //save data to be sent

	//DS("<< Repeats ");DU(lDataStruct.XDATA_Repeats,0);DS(" of ");DU(lDataStruct.PAYLOADLEN,0);DNL();
	
	//unsigned long TempTime = 0;
  for (byte i = 0; i <= retries; i++) {
  	
  	//TempTime=TIMING::millis();
		//for(uint16_t r=0; r<=lDataStruct.XDATA_Repeats; r++) {
	  	send((const void*)lDataStruct.DATA, lDataStruct.PAYLOADLEN, lDataStruct.XDATA_Repeats,true);
	  //}
		//DS("send ");DU(lDataStruct.XDATA_Repeats,0);DS(" ");DU(TIMING::millis()-TempTime,0);DNL();
		
		if(!i) sentRetryTime = TIMING::millis();
	  unsigned long sentTime = TIMING::millis();

	  while (TIMING::millis()-sentTime <= retryWaitTime) {
			if (lDataStruct.XData_Config & (XDATA_ACK_REQUEST | XDATA_CMD_REQUEST)) {
	    	if(ACKReceived(lDataStruct.TARGETID,lDataStruct.XData_Config & XDATA_ACK_REQUEST, lDataStruct.XData_Config & XDATA_CMD_REQUEST)) {
	      	DU(lDataStruct.TARGETID,2);DS(" ACK ");DU(TIMING::millis()-sentRetryTime,0);DS("ms\n");
	      	return true;
	    	}
	  	}
 		}
 		//Serial.print(" RETRY#");Serial.println(i+1);
  }
  if (lDataStruct.XData_Config & (XDATA_ACK_REQUEST | XDATA_CMD_REQUEST)) { 
  	DU(lDataStruct.TARGETID,2);
  	DS_P(" ACK missed"); D_DS(" [");
  	for(uint8_t i=0; i<lDataStruct.PAYLOADLEN; i++)
  		D_DH2(lDataStruct.DATA[i]);
		D_DS("]");
		DNL();
  }
  return false;
}
*/
// should be polled immediately after sending a packet with ACK request
//#if !HAS_RFM69_TXonly
bool myRFM69::ACKReceived(byte senderID, bool ACKcheck, bool CMDcheck) {
	poll(); //process messages
	if( ( (ACKcheck && (DataStruct.XData_Config & XDATA_ACK_RECEIVED)) || 
			  (CMDcheck && (DataStruct.XData_Config & XDATA_CMD_RECEIVED))) 
			&& DataStruct.SENDERID == senderID) {
		poll(); //quickly reset radio to RX	in case of OutputTunneling
		return true;
	}
  return false;
}
//#endif

/*
void myRFM69::send(byte toAddress, const void* buffer, byte bufferSize, bool requestACK)
{
  writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks
  long now = millis();
  while (!canSend() && millis()-now < RF69_CSMA_LIMIT_MS) receiveDone();
  sendFrame(toAddress, buffer, bufferSize, requestACK, false);
}

void myRFM69::sendFrame(byte toAddress, const void* buffer, byte bufferSize, bool requestACK, bool sendACK)
{
  setMode(RF69_MODE_STANDBY); //turn off receiver to prevent reception while filling fifo
	while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady
  writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00); // DIO0 is "Packet Sent"
  if (bufferSize > RF69_MAX_DATA_LEN) bufferSize = RF69_MAX_DATA_LEN;

	//write to FIFO
	select();
	SPI.transfer(REG_FIFO | 0x80);
	SPI.transfer(bufferSize + 3);
	SPI.transfer(toAddress);
  SPI.transfer(_address);

  //control byte
  if (sendACK)
    SPI.transfer(0x80);
  else if (requestACK)
    SPI.transfer(0x40);
  else SPI.transfer(0x00);
  
	for (byte i = 0; i < bufferSize; i++)
    SPI.transfer(((byte*)buffer)[i]);
	unselect();
  	
	// no need to wait for transmit mode to be ready since its handled by the radio 
	setMode(RF69_MODE_TX);
	while (digitalRead(_interruptPin) == 0); //wait for DIO0 to turn HIGH signalling transmission finish
  //while (readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PACKETSENT == 0x00); // Wait for ModeReady
  setMode(RF69_MODE_STANDBY);
}

// to increase the chance of getting a packet across, call this function instead of send
// and it handles all the ACK requesting/retrying for you :)
// The only twist is that you have to manually listen to ACK requests on the other side and send back the ACKs
// The reason for the semi-automaton is that the lib is interrupt driven and
// requires user action to read the received data and decide what to do with it
// replies usually take only 5-8ms at 50kbps@915Mhz
bool RFM69::sendWithRetry(byte toAddress, const void* buffer, byte bufferSize, byte retries, byte retryWaitTime) {
  unsigned long sentTime;
  for (byte i = 0; i <= retries; i++)
  {
    send(toAddress, buffer, bufferSize, true);
    sentTime = millis();
    while (millis()-sentTime<retryWaitTime)
    {
      if (ACKReceived(toAddress))
      {
        //Serial.print(" ~ms:");Serial.print(millis()-sentTime);
        return true;
      }
    }
    //Serial.print(" RETRY#");Serial.println(i+1);
  }
  return false;
}

// should be polled immediately after sending a packet with ACK request
bool RFM69::ACKReceived(byte fromNodeID) {
  if (receiveDone())
    return (SENDERID == fromNodeID || fromNodeID == RF69_BROADCAST_ADDR) && ACK_RECEIVED;
  return false;
}

// check whether an ACK was requested in the last received packet (non-broadcasted packet)
bool RFM69::ACKRequested() {
  return ACK_REQUESTED && (TARGETID != RF69_BROADCAST_ADDR);
}

// should be called immediately after reception in case sender wants ACK
void RFM69::sendACK(const void* buffer, byte bufferSize) {
  byte sender = SENDERID;
  int _RSSI = RSSI; // save payload received RSSI value
  writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks
  unsigned long now = millis();
  while (!canSend() && millis()-now < RF69_CSMA_LIMIT_MS) receiveDone();
  sendFrame(sender, buffer, bufferSize, false, true);
  RSSI = _RSSI; // restore payload RSSI
}

*/

/********************* Receiver Functions *********************/

//void myRFM69::setListenRegs(byte onOff) {
//  RFM69_ListenModeActive=onOff;

  
//  if (onOff) {
//  	DS("LISTEN-ON\n");
//	  /* 0x0D */ writeReg(REG_LISTEN1, RF_LISTEN1_RESOL_IDLE_4100 | RF_LISTEN1_RESOL_RX_64 | RF_LISTEN1_CRITERIA_RSSI | RF_LISTEN1_END_10 ); //0x94	
//	  /* 0x0E */ writeReg(REG_LISTEN2, 0x40); //0x40: 64*4.1ms~262ms idle
//		/* 0x0F */ writeReg(REG_LISTEN3, 0x10); //0x20: 32*64us~2ms RX
//		writeReg(REG_RXTIMEOUT1, 0x10);
//		writeReg(REG_RXTIMEOUT2, 0x10); //needed otherwise will always be in RX mode
//		/* 0x29 */ writeReg(REG_RSSITHRESH, 0xA0); //{ REG_RSSITHRESH, 0xE4 /*MAX*/ } //geändert im Vergleich zum HopeHF Source
//  } else {
//		writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0x83) | RF_OPMODE_LISTEN_OFF | RF_OPMODE_LISTENABORT | RF_OPMODE_STANDBY);
//		writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0x83) | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY);
//    _mode = RF69_MODE_STANDBY;
//    DS("LISTEN-OFF\n");
//  }
//}

//#if !HAS_RFM69_TXonly
void myRFM69::receiveBegin() {
	memset(&DataStruct,0,sizeof(myRFM69_DATA));
	RF69_Config.oldRSSI=0;
	
	if (readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY)
	  writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks
	writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01); //set DIO0 to "PAYLOADREADY" in receive mode

  setMode(RF69_MODE_RX);
}


bool myRFM69::receiveDone() {
// ATOMIC_BLOCK(ATOMIC_FORCEON)
// {
  noInterrupts(); //re-enabled in unselect() via setMode() or via receiveBegin()
//  cfgInterrupt(this, mySpi.getIntPin(RF69_Config.spi_id), Interrupt_Block); //im SPI Module enthalten
  
  if (RF69_Config.mode == RF69_MODE_RX && DataStruct.PAYLOADLEN>0 ) {
   	//enables interrupts
	 #if HAS_RFM69_LISTENMODE || HAS_RFM69_TXonly
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
//#endif

int myRFM69::readRSSI(bool forceTrigger) {
  int rssi = 0;
  if (forceTrigger) {
    //RSSI trigger not needed if DAGC is in continuous mode
    writeReg(REG_RSSICONFIG, RF_RSSI_START);
    while ((readReg(REG_RSSICONFIG) & RF_RSSI_DONE) == 0x00); // Wait for RSSI_Ready
  }
  rssi = -readReg(REG_RSSIVALUE);
  rssi >>= 1; // division: /2
  return rssi;
}

/********************* Basic Driver Functions *********************/

void myRFM69::interrupt() {

	myRFM69_DATA *data = &DataStruct;

	if(RF69_Config.mode != RF69_MODE_RX) return;

//	byte irq2 = readReg(REG_IRQFLAGS2);
//	D_DS_P("Int ");D_DH2(irq2);D_DS_P("\n");

	//INFO PinChange: im PinChg Interrupt gibt es kurz hintereinander ein PayloadReady interrupt und danach ein FifoNotEmpty interrupt. Der zweite löscht den Dateninhalt wieder -> 00000 Daten
#if HAS_RFM69_LISTENMODE
  if (readReg(REG_IRQFLAGS2) /*irq2*/ & (RF_IRQFLAGS2_PAYLOADREADY | RF_IRQFLAGS2_FIFONOTEMPTY) ) {
#else
	if (readReg(REG_IRQFLAGS2) /*irq2*/ & RF_IRQFLAGS2_PAYLOADREADY ) {
#endif

//  if (irq2 & (RF_IRQFLAGS2_PAYLOADREADY | RF_IRQFLAGS2_FIFONOTEMPTY) ) {
		data->RSSI = readRSSI();

		#if HAS_RFM69_LISTENMODE
		if(!RF69_Config.ListenModeActive) 
		#endif	
		{
		 #if HAS_RFM69_LISTENMODE || HAS_RFM69_TXonly
		 	setMode(RF69_MODE_SLEEP);
		 #else
			setMode(RF69_MODE_STANDBY);
		 #endif
		}
		#if HAS_RFM69_LISTENMODE
		 else {
			setMode(RF69_MODE_SLEEP,true,false); //set RFM to sleep without changing current global mode. Necessary for ReceiveDone
		}
		#endif
	
		//DS("Standby\n");TIMING::delay(5000);
 #if BASIC_SPI==1
    select();
    SPI.transfer(REG_FIFO & 0x7f);
	#else	
		mySpi.xferBegin(RF69_Config.spi_id);
    mySpi.xfer_byte(REG_FIFO & 0x7f);
  #endif
		data->PAYLOADLEN = (RF69_Config.PacketFormatVariableLength ? mySpi.xfer_byte(0) : ProtocolInfo[RF69_Config.Protocol-1].PayloadLength ); //ProtocolInfo[Protocol-1].PayloadLength == 0xFF		
		data->PAYLOADLEN = data->PAYLOADLEN > RF69_MAX_DATA_LEN ? RF69_MAX_DATA_LEN : data->PAYLOADLEN; //precaution

    for (byte i=0; i < data->PAYLOADLEN; i++)
    {
     #if BASIC_SPI==1
     	data->DATA[i] = SPI.transfer(0);
     #else
      data->DATA[i] = mySpi.xfer_byte(0); //SPI.transfer(0);
     #endif
    }
    //if((readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_FIFONOTEMPTY) != 0x0) DS("not empty\n");
		//if (data->PAYLOADLEN<RF69_MAX_DATA_LEN) data->DATA[data->PAYLOADLEN]=0; //add null at end of string
		
		#if BASIC_SPI==1
    unselect();
    #else
    mySpi.xferEnd(RF69_Config.spi_id);
    #endif
    
    #if HAS_RFM69_LISTENMODE	
		if(!RF69_Config.ListenModeActive)
		#endif
		{
			setMode(RF69_MODE_RX);
		}

		//data->RSSI = readRSSI();
		//DS("END - ");DS_P("Int ");DU(RF69_Config.mode,0);DS_P(" ");DU(data->PAYLOADLEN,0);DS_P("\n");//TIMING::delay(5000);
  }
}

void myRFM69::setMode(byte newMode, bool forceReset, bool saveMode)
{
	if (newMode == RF69_Config.mode && !forceReset) return; //TODO: can remove this?

#if HAS_RFM69_LISTENMODE	
	//ListenMode deaktivieren wenn ListenModeActive an ist und aber ein anderer Modus als RX gewünscht wird
	//ListeMode deaktivieren wenn ListeModeActive abgeschalten wurde aber ListenMode noch aktiv ist.	
	if( (RF69_Config.ListenModeActive && newMode!=RF69_MODE_RX) || (!RF69_Config.ListenModeActive && ((readReg(REG_OPMODE) & 0x40) != 0x0)) ) {
		//DS("ListenOFF\n");
		writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0x83) | RF_OPMODE_LISTEN_OFF | RF_OPMODE_LISTENABORT | RF_OPMODE_SLEEP);
		writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0x83) | RF_OPMODE_LISTEN_OFF | RF_OPMODE_SLEEP);
	}
#endif
	
//0xE3: Behält unused Bits, Abort, Listen, Sequencer bei
	switch (newMode) {
		case RF69_MODE_TX:
			writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_TRANSMITTER);
      if (RF69_Config.isRFM69HW) setHighPowerRegs(true);
      //DS("TX\n");
			break;

		case RF69_MODE_RX:
		#if HAS_RFM69_LISTENMODE			
			if(!RF69_Config.ListenModeActive)
		#endif
			{
				writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_RECEIVER); //DS("RX\n");
			}
      if (RF69_Config.isRFM69HW) setHighPowerRegs(false);
	#if HAS_RFM69_LISTENMODE	
			if(RF69_Config.ListenModeActive) {
				writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY);
				writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0x83) | RF_OPMODE_SLEEP | RF_OPMODE_LISTEN_ON);
				//DS("ListenOn\n");
			}
	#endif
			break;
		case RF69_MODE_SYNTH:
			writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_SYNTHESIZER);
			//DS("SYNTH\n");
			break;
		case RF69_MODE_STANDBY:
			writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY);
			//DS("STBY\n");
			break;
		case RF69_MODE_SLEEP:
			writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_SLEEP);
			//DS("SLEEP\n");
			break;
		default: return;
	}

	// we are using packet mode, so this check is not really needed
  // but waiting for mode ready is necessary when going from sleep because the FIFO may not be immediately available from previous mode
	while (RF69_Config.mode == RF69_MODE_SLEEP && (readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // Wait for ModeReady

	if(saveMode)
		RF69_Config.mode = newMode;
}

byte myRFM69::readReg(byte addr) {

#if BASIC_SPI==1
  select();
  SPI.transfer(addr & 0x7F);
  byte regval = SPI.transfer(0);
  unselect();
#else
  mySpi.xferBegin(RF69_Config.spi_id);
  mySpi.xfer_byte(addr & 0x7F);
  byte regval = mySpi.xfer_byte(0);
  mySpi.xferEnd(RF69_Config.spi_id);
#endif
  return regval;
}

void myRFM69::writeReg(byte addr, byte value) {
#if BASIC_SPI==1
  select();
  SPI.transfer(addr | 0x80);
  SPI.transfer(value);
  unselect();
#else
  mySpi.xferBegin(RF69_Config.spi_id);
  mySpi.xfer_byte(addr | 0x80);
  mySpi.xfer_byte(value);
  mySpi.xferEnd(RF69_Config.spi_id);
#endif
}


#if BASIC_SPI==1
/// Select the transceiver
void myRFM69::select() {
  noInterrupts();
  //save current SPI settings
  _SPCR = SPCR;
  _SPSR = SPSR;
  //set RFM69 SPI settings
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV4); //decided to slow down from DIV2 after SPI stalling in some instances, especially visible on mega1284p when RFM69 and FLASH chip both present
  digitalWrite(SS, LOW);
}
/// UNselect the transceiver chip
void myRFM69::unselect() {
  digitalWrite(SS, HIGH);
  //restore SPI settings to what they were before talking to RFM69
  SPCR = _SPCR;
  SPSR = _SPSR;
  interrupts();
}
#endif

