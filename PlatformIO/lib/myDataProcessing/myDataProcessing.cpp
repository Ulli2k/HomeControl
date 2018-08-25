
//TODO:  sysclock.runready()wirklich hier?

#include <myDataProcessing.h>

// extern const char helpText[] PROGMEM;
extern void printPROGMEM (const char * s);

#ifdef HAS_DISPLAY_TUNNELING
byte myDataProcessing::OutputTunnel	=	0;
#endif

myDataProcessing::myDataProcessing() {
	WelcomeMSG=0;
#ifdef HAS_DISPLAY_TUNNELING
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
			#ifdef HAS_FORWARD_MESSAGES
				uint8_t len=strlen(cmd);
				if(len+3 <= MAX_RING_DATA_SIZE) {
					//transform send msg in forward msg (<devID>... -> T<devID>....)
					shiftCharRight((byte*)cmd,len+1,1);
					cmd[0] = MODULE_COMMAND_CHAR(MODULE_DATAPROCESSING_TUNNELING_MODULE);
					if(DEBUG) { DS_P("..forwarded with <");DS(cmd);DS_P(">\n"); }
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

#ifdef HAS_DISPLAY_TUNNELING
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
#ifdef HAS_DISPLAY_TUNNELING
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
	DS_P(" * [Help]    h\n");
}

bool myDataProcessing::poll() {

  RecvData RecvDataBuffer;
  RecvData *PtrRecvData;
  uint8_t ret = 0;

  if(!WelcomeMSG) { printPROGMEM(welcomeText); DNL(); WelcomeMSG++; }

	//poll SysClock for not async functionality
	ret = sysclock.runready();

  /* Check if data is available at RingBuffer and process */
  if( FIFO_ready(DataRing) && FIFO_available(DataRing) ) {

    // copy RingBuffer data
    FIFO_block(DataRing);
    PtrRecvData = DataFIFO_read(DataRing); // return a pointer to the current available data
    if(!PtrRecvData) { FIFO_release(DataRing); return ret; }
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
						// char c = pmt->name[MODULE_PROTOCOL(RecvDataBuffer.ModuleID)];
						char c = getFunctionChar(pmt->module->getFunctionCharacter(),MODULE_PROTOCOL(RecvDataBuffer.ModuleID));
						// if(!( (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) c=pmt->name[0];
						if(c==' ') break;

						if(!(pmt->module->validdisplayData(&RecvDataBuffer)))
							break;

						#ifdef HAS_DISPLAY_TUNNELING
						if(OutputTunnel) setDisplayCopy(1); //reset & clear DisplayCopy Buffer
						#endif

						DU(DEVICE_ID,2); //Print DeviceID
						DC(c); //Print Module Name Character
						//Print Module Data
						pmt->module->displayData(&RecvDataBuffer);

						#ifdef HAS_DISPLAY_TUNNELING
						if(OutputTunnel) {
							setDisplayCopy(0);
							DS_P(" >> tunneled"); //if(DEBUG) { DS_P(" with <");DS(getDisplayCopy());DS_P(">"); }
						}
						#endif
						DNL();

					#ifdef HAS_DISPLAY_TUNNELING
						if(OutputTunnel) {
							uint8_t len=strlen(getDisplayCopy());
							if((getDisplayCopy())[len-1] == '\n') len--;
							addToRingBuffer(MODULE_DATAPROCESSING, MODULE_DATAPROCESSING_TUNNELING_MODULE, (const byte*)getDisplayCopy(), len);
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
  return ret;
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
			const char *funcChar = pmt->module->getFunctionCharacter();
			for(uint8_t i=0; i<=strlen(funcChar); i++) {
			  if(funcChar[i] == cmd[0]) {
			  	pmt->module->send(cmd+1, (uint8_t) (strlen(funcChar)>=1 ? ((i+1)<<4) | pmt->typecode : pmt->typecode));
			  }
			// for(uint8_t i=0; i<=strlen(pmt->name); i++) {
			//   if(pmt->name[i] == cmd[0]) {
			//   	pmt->module->send(cmd+1, (uint8_t) (strlen(pmt->name)>=1 ? ((i+1)<<4) | pmt->typecode : pmt->typecode));
			//   }
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
