
#ifndef _MY_TEST_MODULE_h
#define _MY_TEST_MODULE_h

#include <myBaseModule.h>
#include <libSPI.h>
#include <libRadio.h>


template <class libRadioType, uint8_t intPin>
class myRadio  : public myBaseModule {

private:

    libRadioType radio;

    //Msg Debouncing
    word      _tDebCmd;								// Value-Range: 0-255
    byte      _tDebCRC;								// Value-Range: 0-255
    uint8_t   _bDebMsgCnt;					// Value-Range: 0-255
    uint8_t   _RFM69TXMsgCounter;		// Value-Range: 0-255
    uint16_t  _minDebounceTime;		// Value-Range: 0-65535

    inline bool DebounceRecvData(uint8_t checkSum) {

        //word crc = ~0;
        bool debounced=true;
        word now = millis();

      	//for (byte i = 0; i < lDataStruct->PAYLOADLEN; ++i)
    	    //crc = _crc16_update(crc, lDataStruct->DATA[i]);
        // if different crc or too long ago, this cannot be a repeated packet
       	//if((lDataStruct->CheckSum == _tDebCRC) && (((now - _tDebCmd) < _minDebounceTime) || (_bDebMsgCnt == lDataStruct->MsgCnt && lDataStruct->MsgCnt!=0 )) ) {
       	if((checkSum == _tDebCRC) && ((millis_since(_tDebCmd) < _minDebounceTime)) ) {
    			debounced=false;
    		}
    		if(DEBUG) { DS_P("Debounce ");DU(millis_since(_tDebCmd),0);if(!debounced) {DC('!');} DC('>');DU(_minDebounceTime,0);DS("\n"); }
        // save last values and decide whether to report this as a new packet
        _tDebCRC = checkSum;
        _tDebCmd = now;
        //_bDebMsgCnt = lDataStruct->MsgCnt;

        return debounced;
    }
  public:

    myRadio()  : _tDebCmd(0), _tDebCRC(0), _bDebMsgCnt(0), _RFM69TXMsgCounter(0), _minDebounceTime(RFM69_Default_MSG_DebounceTime) { };

		void initialize() {

        radio.initialize(intPin);
        #if HAS_RFM69_CMD_TUNNELING==2 //Satellite
          addToRingBuffer(MODULE_DATAPROCESSING,MODULE_DATAPROCESSING_OUTPUTTUNNEL,(const byte*)(radio.isTunnelingActive()),1);
        #endif
        cfgInterrupt(this, intPin, Interrupt_Rising);

        #if HAS_RFM69_CMD_TUNNELING==2 && RFM69_CMD_TUNNELING_DEFAULT_VALUE //Satellite send initializing command to Host
      	addToRingBuffer(MODULE_DATAPROCESSING_WAKE_SIGNAL, 0, NULL, 0);
      	#endif
      };

		void displayData(RecvData *DataBuffer) {
      //if((uint8_t)DataBuffer->DataTypeCode!=mySpi.getIdent(spi_id) && (uint8_t)DataBuffer->DataTypeCode!=0)
      //if(!validdisplayData(DataBuffer))
      //	return; //not right spi id --> other rfm module

      //DC(mySpi.getIdent(RF69_Config.spi_id)); //add Module-ID to recv packet
      DC(radio.getRadioIndex());

      switch(DataBuffer->ModuleID) {

        case MODULE_RFM69_MODEQUERY:
          DU(radio.getProtocol(),2);
          DS((radio.is433Module() ? "4" : "8"));
          break;

        case MODULE_RFM69_OPTION_SEND:
          DC('s'); //print command
          DS((char*)DataBuffer->Data);
          break;

    #if HAS_RFM69_POWER_ADJUSTABLE
        case MODULE_RFM69_OPTION_POWER:
          DC('p'); //print command
          DU(RF69_Config.powerLevel(),2);
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
            DU(radio.getProtocol(),1);
            for(uint8_t i=0; i<(DataBuffer->DataSize-1); i++) {
              if(radio.getProtocol() == RFM69_PROTOCOL_MyProtocol || radio.getProtocol() == RFM69_PROTOCOL_HX2262)
                DC(DataBuffer->Data[i]);
              else
                DH2(DataBuffer->Data[i]);
            }

            int rssi = ((-DataBuffer->Data[DataBuffer->DataSize-1]) >> 1);
            DS_P(" [RSSI:");DC(radio.getRadioIndex());/*DC(mySpi.getIdent(RF69_Config.spi_id));*/DS_P(":");DI(rssi,0);DS_P("]");
          break;
      }
    };
		bool validdisplayData(RecvData *DataBuffer) {
      if((char)DataBuffer->DataTypeCode!=radio.getRadioIndex() /*mySpi.getIdent(RF69_Config.spi_id)*/ && (char)DataBuffer->DataTypeCode!='0')
    		return 0; //not right spi id --> other rfm module
    	return 1;
    };

		void printHelp(){
      DS_P("\n ## RFM69 - ID:");	DC(radio.getRadioIndex());/*DC(mySpi.getIdent(RF69_Config.spi_id));*/	DS_P(" ##\n");
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
    };

    bool poll() {
        if(radio.receiveDone()) {
        	//int rssi=radio.getRSSI();
        	bool valid=1;
        	bool debounced=1;

      		valid = radio.transformData(); 	// check packet validity

      	 	if(valid) debounced = DebounceRecvData(radio.getPayloadCheckSum()); // debounce command

          radio.attachRSSI2DATA();

      	//Debuging
      		// if(DEBUG) { //speed up
      		// 	DS_P("MSG[");DU(radio.getRadioIndex(),0);/*DC(mySpi.getIdent(RF69_Config.spi_id));*/DS_P("] ");
      		// 	for(uint8_t i=0; i<DataStruct.PAYLOADLEN-1; i++) {
      		// 		DH2(DataStruct.DATA[i]);
      		// 	}
      		// 	DS_P(" [RSSI:");DC(mySpi.getIdent(RF69_Config.spi_id));DS_P(":");DI(rssi,0);DS_P("|");
      		// 	DU(DataStruct.XDATA_BurstTime,0); DS((DataStruct.XData_Config & XDATA_BURST) ? "ms] " : "] ");
      		// 	if(!valid) { DS_P(" >> skipped"); }
      		// 	DNL();
      		// }

      		if(!valid) return 1; //exit if message is not valid

          radio.waitBurstTime(); //wait till burst is over


      #if HAS_RFM69_TX_FORCED_DELAY
          radio.resetForcedDelayTimer();
      #endif

      #ifndef HAS_RFM69_SNIFF
      	// DataOptions (ACK_RECEIVED, ACK_REQUESTED, CMD_RECEIVED, CMD_REQUESTED)
      		if(radio.isPayloadFlag_CMD_REQ()) { //kann nicht debounced werden da RückgabeWert erwartet wird
      		  addToRingBuffer(MODULE_DATAPROCESSING, 0, radio.getPayloadDATA(), radio.getPayloadLen()-1 /*-1 ohne RSSI*/);

      		} else if(radio.isPayloadFlag_ACK_REQ()) {

      			if(radio.getPayloadLen() > 1) {	// Push msg to ring buffer
        	  	addToRingBuffer(MODULE_DATAPROCESSING, 0, radio.getPayloadDATA(), radio.getPayloadLen()-1 /*-1 ohne RSSI*/);
        	  }

            radio.sendACK(); //send ACK after processing command, due to clearing DataStruct

      //			byte buf[2];
      //			buf[0] = mySpi.getIdent(RF69_Config.spi_id);
      //			buf[1] = DataStruct.SENDERID;
      //			addToRingBuffer(MODULE_DATAPROCESSING,MODULE_RFM69_SENDACK,(const byte*)buf,2); //send ACK after processing transmitted data -> reduce reaction timing in case of ListenMode

            } else if((radio.isPayloadFlag_CMD_RECV()) && debounced) {
      				// nicht durch den RingBuffer da sonst die falsche DeviceID voran gestellt wird
      				DU(radio.getSenderID(),2);
              byte *data = radio.getPayloadPointer();
      				for(uint8_t i=0; i<(radio.getPayloadLen()-1); i++) {
      						DC((char)data[i]);
      				}
      				//rssi = ((-DATA[PAYLOADLEN-1]) >> 1);
      				DS_P(" [RSSI:");DC(radio.getRadioIndex());/*DC(mySpi.getIdent(RF69_Config.spi_id));*/DS_P(":");DI(radio.getRSSI(),0);DS_P("]");
      				DNL();

      		} else if(radio.isPayloadFlag_ACK_RECV()) {

      		} else if(!(radio.isPayloadFlagSet())) {
      #endif
      		  //print received command to Uart
        		addToRingBuffer(MODULE_RFM69, radio.getRadioIndex()/*mySpi.getIdent(RF69_Config.spi_id)*/, radio.getPayloadDATA(), radio.getPayloadLen());
      #ifndef HAS_RFM69_SNIFF
      		}
      #endif

      		//receiveDone(); //speedup to be ready for next messages?? --> Führt dazu das das Gateway keine ACK mehr rechtzeitig empfängt...komisch
      		return 1;
      	}
      	return 0;
    };

    // extern const typeModuleInfo ModuleTab[]; //notwendig für MODULE_COMMAND_CHAR
    void send(char *cmd, uint8_t typecode) {

  	   uint16_t buf=0;
      	//uint16_t FrqShift=0; //needed for bursts to ListenMode clients
      	char *p = cmd;
      	char spiId = cmd[0];
      	myRFM69_DATA lDataStruct;
      	lDataStruct.SENDERID 			= radio.getSenderID();
      	lDataStruct.XData_Config	=	XDATA_NOTHING;

      	cmd++; //Jump over ModuleID [1]
      	p++;
        if(spiId != radio.getRadioIndex()/*mySpi.getIdent(RF69_Config.spi_id)*/ && spiId != '0') //valid SPI Module Number
	         return;
        spiId=radio.getRadioIndex(); //mySpi.getIdent(RF69_Config.spi_id); //in case of requested spiID=0
      	// D_DS_P("valid SPI-ID <");D_DC(spiId);D_DS_P(">\n");

      	//DS("<<");DS(cmd-1);DS(">>\n");
      	//if(typecode==MODULE_RFM69_CONFIG_TEMP) typecode=MODULE_RFM69_OPTIONS; //workaround for RFM12 "O" and "F"

      	switch(typecode) {

      		// case MODULE_RFM69_SENDACK:
      		// 	sendACK((byte)cmd[0]);
      		// 	break;

      		case MODULE_RFM69_MODEQUERY:
      			addToRingBuffer(MODULE_RFM69_MODEQUERY,spiId,NULL,0);
      			break;

      		case MODULE_RFM69_OPTIONS:

      			switch(cmd[0]) {

      #if INCLUDE_DEBUG_OUTPUT
      				case 'o':
      					radio.resetRadio();
      					//rcCalibration(); //will automatically calibrated after RFM restart
      					//AFCCalibration();
      					break;
      				case 'c':
      					cmd++;
      					if(strlen(cmd)==0) {
      						radio.readAllRegs();
      					} else if(strlen(cmd)==2) {
      						radio.readAllRegs(HexStr2uint8(cmd));//addr);
      					} else {
      						if(strlen(cmd)!=4) break;
      						radio.writeRegs(HexStr2uint8(cmd),HexStr2uint8(cmd+2));

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
      					radio.configure(atoi(cmd+1));
      					break;
      #endif

      				case 'b':
      					lDataStruct.XData_Config = XDATA_BURST;
      //					frequenceShift(1);

     				  case 's': //00F1s4010040CB
      					// ETH: learn: 00F1s401004000 day: 00F1s401004200 night: 00F1s401004300 off: 00F1s4010040CB  //00F1s401004000 -_> Data[20]: 6A A9 9555559555559555555555595555565AA655
      					if(radio.getProtocol() != (cmd[1]-'0')) { //check if currently an other protocol is activ. If yes toggle the protocol during send phase
      						buf=0;
      						radio.toggleRadioMode((cmd[1]-'0'),&buf);
      					}
      					cmd+=2; //jump over command 's' or 'b' and protocol number
      					if(ProtocolInfo[radio.getProtocol()-1].transformTxData != NULL) {
      						//lDataStruct.MsgCnt = (_RFM69TXMsgCounter==0xFF ? 1 : ++_RFM69TXMsgCounter);
      						lDataStruct.XData_Config |= XDATA_ACK_REQUEST;
      						ProtocolInfo[radio.getProtocol()-1].transformTxData(cmd,&lDataStruct);

      						if(lDataStruct.PAYLOADLEN) //valid data if length is not 0
      							radio.send(&lDataStruct);

      //						if(lDataStruct.XData_Config & XDATA_BURST) frequenceShift(0);

      						if(buf) { //reset RadioConfig
      							radio.toggleRadioMode(0,&buf);
                    #if HAS_RFM69_CMD_TUNNELING==2 //Satellite
                    		if(radio.isTunnelingActive())
                    			addToRingBuffer(MODULE_DATAPROCESSING,MODULE_DATAPROCESSING_OUTPUTTUNNEL,(const byte*)"1",1);
                    #endif
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
      			if(radio.getProtocol() != RFM69_PROTOCOL_MyProtocol) {
      				D_DS_P("no tunneling support.\n");
      				break;
      			}

      			switch(cmd[0]) {

      				case 'c': //activate Tunneling function
      					radio.setTunneling((cmd[1]=='0')?false:true);
      				#if defined(STORE_CONFIGURATION)
      					StoreValue((void*)&RF69_Config.TunnelingCMDQuery,(void*)&eeTunnelingCMDQuery,1);
      				#endif

      			#if HAS_RFM69_CMD_TUNNELING==2 //Satellite
      					if(strlen(cmd)>2) { //safe Host ID
      						radio.setTunnelingHostID(atoi(cmd+2));
      					#if defined(STORE_CONFIGURATION)
      					  StoreValue((void*)&safeHostID4Tunneling,(void*)&eesafeHostID4Tunneling,1);
      				  #endif
      					} else {
      						DS_P("Tun-Host-ID: ");DU(radio.getTunnelingHostID(),0);DNL();
      					}
      					addToRingBuffer(MODULE_DATAPROCESSING,MODULE_DATAPROCESSING_OUTPUTTUNNEL,(const byte*)(radio.isTunnelingActive() ? "1" : "0"),1);
      			#endif

      					break;

      			#if HAS_RFM69_CMD_TUNNELING==1 //Host
      				case 'b': //activate TX Bursts
      					RF69_Config.TunnelingSendBurst=((cmd[1]=='0')?false:true); //send burst on
      					break;
      			#endif

      				default: //Command to tunnel (Host<->Satellite)
      					if(!radio.isTunnelingActive()) { D_DS_P("Satellite CMD request not active.\n"); return; } // off

      			#if HAS_RFM69_CMD_TUNNELING==1 // HOST
      					if(RF69_Config.TunnelingSendBurst) {
      						lDataStruct.XData_Config = XDATA_BURST;
      //						frequenceShift(1);
      					}
      			#endif

      					lDataStruct.XData_Config |= cmd[0];

      					if(!(lDataStruct.XData_Config & XDATA_CMD_REQUEST || lDataStruct.XData_Config & XDATA_ACK_REQUEST)) {

      					#if HAS_RFM69_CMD_TUNNELING==2 //Satellite
      						if(radio.getTunnelingHostID()!=DEVICE_ID_BROADCAST) {
      							cmd[1] = '0' + (radio.getTunnelingHostID()/10);
      							cmd[2] = '0' + (radio.getTunnelingHostID()%10);
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
      					radio.send(&lDataStruct);
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

      void interrupt() {
        radio.fetchData(); };
    }; //CLASS


#endif
