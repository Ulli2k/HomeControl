
#include <myDataProcessing.h>

// #if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING
// 	#include <myRFM69protocols.h> //needed for Tunneling Function
// #endif

// extern const char helpText[] PROGMEM;
extern void printPROGMEM (const char * s);

#if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING==2 //Satellite
byte myDataProcessing::OutputTunnel	=	0;
#endif

// #define STR_HELPER(x) #x
// #define STR(x) STR_HELPER(x)

// const char myDataProcessing::welcomeText[] PROGMEM =
// 	  "[RN"
// 	#if (DEVICE_ID<=9)
// 		"0"
// 	#endif
// 	  STR(DEVICE_ID)
// 	#if defined(STORE_CONFIGURATION)
// 		"s"
// 	#endif
// 	#ifdef HAS_ADC
// 		",C"
// 	#endif
// 	#ifdef HAS_TRIGGER
// 		",N"
// 	#endif
// 	#ifdef HAS_LEDs
// 		",L"
// 	#endif
// 	#ifdef HAS_SWITCH //|| HAS_ROLLO_SWITCH
// 		",S"
// 	#endif
// 	#ifdef HAS_ROLLO
// 		",J"
// 	#endif
// 	#ifdef HAS_BUZZER
// 		",B"
// 	#endif
// 	#ifdef HAS_RFM69
// 		",R"
// 		#if HAS_RFM69_LISTENMODE && !HAS_RFM69_TXonly
// 		"l"
// 		#elif !HAS_RFM69_LISTENMODE && HAS_RFM69_TXonly
// 		"t"
// 		#endif
// 		#if HAS_RFM69_CMD_TUNNELING==1
// 		"h"
// 		#elif(HAS_RFM69_CMD_TUNNELING==2)
// 		"s"
// 		#endif
// 	#endif
// 	#if (defined(HAS_IR_TX) || defined(HAS_IR_RX))
// 		",I"
// 		#if HAS_IR_TX
// 			"s"
// 		#endif
// 		#if HAS_IR_RX
// 			"r"
// 		#endif
// 	#endif
// 	#ifdef HAS_POWER_MONITOR_CT //|| HAS_POWER_MONITOR_PULSE
// 		",P"
// 	#endif
// 	#ifdef HAS_BME280
// 		",E"
// 	#endif
// 	"]";


myDataProcessing::myDataProcessing() {
	WelcomeMSG=0;
#if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING==2 //Satellite
	OutputTunnel=0;
#endif
}

void myDataProcessing::send(char *cmd, uint8_t typecode) {

	char *p=NULL;

	switch(typecode) {

		case MODULE_DATAPROCESSING_DEVICEID: //process command with DeviceID in Front (used only by UART)
	    if(!(cmd[0]>='0' && cmd[0]<='9' && cmd[1]>='0' && cmd[1]<='9')) { //no DeviceID noted
	    	p=cmd;
	    } else if(isRightDeviceID(cmd)) {
				p=cmd+2; //jump over DeviceID
			} else {
				D_DS_P("Wrong DeviceID.");
			#if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING==1 //Host
				uint8_t len=strlen(cmd);
				if(len+3 <= MAX_RING_DATA_SIZE) {
					//transform send msg in forward msg (<devID>... -> T<devID>....)
					shiftCharRight((byte*)cmd,len+1,3);
					cmd[0] = MODULE_COMMAND_CHAR(MODULE_RFM69, MODULE_RFM69_TUNNELING);
					cmd[1] = '0'; //dummy SPI-ID, will be replaced by RFM Module if Tunneling function is active
					cmd[2] = XDATA_ACK_REQUEST;
					if(DEBUG) { DS_P("..forward with <");DS(cmd);DS_P(">\n"); }
					p=cmd;
				} else {
					DS_P("Data too long for tunneling.\n");
				}
			#else
				DNL();
			#endif
			}
			if(p) callSendFkt(p);
			break;

#if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING==2 //Satellite
		case MODULE_DATAPROCESSING_OUTPUTTUNNEL:
			DFL();
			if(cmd[0]=='0') {
				if(!isUartTxActive) {
					PowerOpti_USART0_ON;
				}
				OutputTunnel=0;
			} else {
				OutputTunnel=1;
				if(!DEBUG) {
					PowerOpti_USART0_OFF;
				} //turn off UART (power saving)
			}
			break;
#endif

		case MODULE_DATAPROCESSING_QUIET:
			DEBUG_ONOFF((cmd[0]=='1') ? 1:0);
#if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING==2 //Satellite
			if(cmd[0]=='0' && !isUartTxActive) {
				PowerOpti_USART0_ON;
			} else if(cmd[0]=='0' && OutputTunnel) {
				PowerOpti_USART0_OFF;
			}
#endif
			break;

#if INCLUDE_HELP
		case MODULE_DATAPROCESSING_HELP:
			printModuleHelp();
			break;
#endif
		case MODULE_DATAPROCESSING_FIRMWARE:
			addToRingBuffer(MODULE_DATAPROCESSING_FIRMWARE, 0, NULL, 0);
			break;
	}
}

void myDataProcessing::displayData(RecvData *DataBuffer) {

	switch(DataBuffer->ModuleID) {
		case MODULE_DATAPROCESSING_FIRMWARE:
			printPROGMEM(welcomeText);
			break;
		case MODULE_DATAPROCESSING_WAKE_SIGNAL:
			break;
	}
}

#if INCLUDE_HELP
void myDataProcessing::printModuleHelp() {

	const typeModuleInfo* pmt = ModuleTab;
	DS_P("Available commands:\n");
	DS_P(" [00-99:DeviceID]<cmd>\n");
	while(pmt->typecode >= 0) {
	  pmt->module->printHelp();
	  pmt++;
	}
}
#endif

void myDataProcessing::printHelp() {

	DS_P("\n ## General ##\n");
	DS_P(" * [FW]      w\n");
	DS_P(" * [Quiet]   q<State>\n");
	DS_P(" * [Version] v\n");
	DS_P(" * [Help]    h\n");
}

bool myDataProcessing::poll() {

  RecvData RecvDataBuffer;
  RecvData *PtrRecvData;
  //bool valid=0;

  if(!WelcomeMSG) { printPROGMEM(welcomeText); DNL(); WelcomeMSG++; }

  /* Check if data is available at RingBuffer and process */
  if( FIFO_ready(DataRing) && FIFO_available(DataRing) ) {

    // copy RingBuffer data
    FIFO_block(DataRing);
    PtrRecvData = DataFIFO_read(DataRing); // return a pointer to the current available data
    if(!PtrRecvData) { FIFO_release(DataRing); return 0; }
    memcpy(&RecvDataBuffer, PtrRecvData, sizeof(RecvData));
    FIFO_release(DataRing);

    //DS("<<Dataprocessing: <");DS((char*)RecvDataBuffer.Data);DS(">\n");

		//Find right module in ModuleTable
		const typeModuleInfo* pmt = ModuleTab;
		while(pmt->typecode >= 0) {
			//maybe module found if valid module char is not ' ' otherwise go further
		  if((uint8_t)pmt->typecode == MODULE_TYP(RecvDataBuffer.ModuleID)) {
				//char *p=NULL;

				//Process Data copied from RingBuffer
				switch(RecvDataBuffer.ModuleID) {

				  case MODULE_DATAPROCESSING: //execute Module send function

					//Internal Command Processing, no DeviceID necessary
				 		if(RecvDataBuffer.DataTypeCode != 0) {
				 			const typeModuleInfo* mT = ModuleTab;
							while(mT->typecode >= 0) {
									if(mT->typecode == MODULE_TYP(RecvDataBuffer.DataTypeCode)) {
										mT->module->send((char*)RecvDataBuffer.Data,RecvDataBuffer.DataTypeCode);
									}
								mT++;
							}
							//valid=true;
							return 1; //command just valid for one Module
						}

					//echo command in non QuietMode
				   	D_DS((char*)RecvDataBuffer.Data);D_DS_P("\n");
				   	callSendFkt((char*)RecvDataBuffer.Data);
						//valid=true;
						//break;
						return 1;

					default:
						char c = pmt->name[MODULE_PROTOCOL(RecvDataBuffer.ModuleID)];
						if(!( (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) c=pmt->name[0];
						if(c==' ') break;

						if(!(pmt->module->validdisplayData(&RecvDataBuffer)))
							break;

						#if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING==2 //Satellite
						if(OutputTunnel) {
							setDisplayCopy(1);
							//00F1s500
							setDisplayCopy(1,'0'/*dummy Module SPI ID*/);
							//setDisplayCopy(1,RF69_getModeDataOptionChar(RF69_DataOption_sendCMD));
							setDisplayCopy(1,(char)XDATA_CMD_RECEIVED); //(char) nachträglich ergänzt wegen warning!!
						}
						#endif

						DU(DEVICE_ID,2); //Print DeviceID
						DC(c); //Print Module Name Character
						//Print Module Data
						pmt->module->displayData(&RecvDataBuffer);
						#if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING==2 //Satellite
						if(OutputTunnel) {
							setDisplayCopy(0);
							DS_P(" >> tunneled"); if(DEBUG) { DS_P(" with <");DS(getDisplayCopy());DS_P(">"); }
						}
						#endif
						DNL();

					#if HAS_RFM69 && HAS_RFM69_CMD_TUNNELING==2 //Satellite
						if(OutputTunnel) {
							uint8_t len=strlen(getDisplayCopy());
							if((getDisplayCopy())[len-1] == '\n') len--;
							addToRingBuffer(MODULE_DATAPROCESSING, MODULE_RFM69_TUNNELING, (const byte*)getDisplayCopy(), len);
							poll(); //directly push Message through Ringbuffer
						}
					#endif
						//valid=true;
						//break;
						return 1;
				}

		  }
		  pmt++;
		}

		if(pmt->typecode<0 ) { //&& !valid) {
			D_DS_P("Unknown received command\n");
		}
  	return 1;
  }
  return 0;
}

bool myDataProcessing::isRightDeviceID(char *cmd) {

	if(cmd[0]>='0' && cmd[0]<='9' && cmd[1]>='0' && cmd[1]<='9') {
		if((10* (cmd[0]-'0') + cmd[1]-'0') == DEVICE_ID || (10* (cmd[0]-'0') + cmd[1]-'0') == DEVICE_ID_BROADCAST)
			return 1;
	}
	return 0;
}

void myDataProcessing::callSendFkt(char *cmd) {

		const typeModuleInfo* pmt = ModuleTab;
		while(pmt->typecode >= 0) {
			for(uint8_t i=0; i<=strlen(pmt->name); i++) {
			  if(pmt->name[i] == cmd[0]) {
			  	pmt->module->send(cmd+1, (uint8_t) (strlen(pmt->name)>=1 ? ((i+1)<<4) | pmt->typecode : pmt->typecode));
			  }
			}
		  pmt++;
		}
}
//PGM_P
void myDataProcessing::printPROGMEM (const char * s) {
	for (;;) {
		char c = pgm_read_byte(s++);
		if (c == 0)
			break;
		if (c == '\n')
			DNL();
		DC(c);
	}
}
