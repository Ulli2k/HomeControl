
#include <myIRMP.h>

void myIRMP::initialize() {//myIRMP_Callback func) { //DataFIFO_t *ring) {

#if HAS_IR_RX
  irmp_init();
  PowerOpti_ADC2_ON;
#endif
#if HAS_IR_TX  
	//PowerOptimization(BOD_,ADC_,AIN_,TIMER2_,TIMER1_,TIMER0_ON,SPI_,USART0_,TWI_,WDT_);
	PowerOpti_TIMER0_ON;
  irsnd_init();
#endif

	//PowerOptimization(BOD_,ADC_,AIN_,TIMER2_,TIMER1_ON,TIMER0_,SPI_,USART0_,TWI_,WDT_);
	PowerOpti_TIMER1_ON;
  Timer1.initialize(US);
  Timer1.attachInterrupt(timerinterrupt);
}

bool myIRMP::poll() {

#if HAS_IR_RX
	IRMP_DATA irmp_data;
	
	if (irmp_get_data(&irmp_data)) {
		addToRingBuffer(MODULE_IRMP, irmp_data.protocol, (const byte*)&irmp_data, sizeof(IRMP_DATA));
		return 1;
	}
#endif
	return 0;
}	

void myIRMP::send(char *cmd, uint8_t typecode) {
#if HAS_IR_RX || HAS_IR_TX
	uint8_t flag, protocol;
  uint16_t command, address;
#endif

#if HAS_IR_RX  
  if(cmd[0] == 'r') {
  	if(cmd[1] == '0') {
  		FKT_TIMER1_OFF;
	  	//TurnOffFunktions(BOD_,ADC_,AIN_,TIMER2_,TIMER1_OFF,TIMER0_,SPI_,USART0_,TWI_,WDT_);
	  } else {
			FKT_TIMER1_ON;
	  	//TurnOffFunktions(BOD_,ADC_,AIN_,TIMER2_,TIMER1_ON,TIMER0_,SPI_,USART0_,TWI_,WDT_);
	  }
	  return;
  }
#endif
#if HAS_IR_TX
	PowerOpti_TIMER0_ON;
	if(strlen(cmd)==12)	{
		flag = HexStr2uint8(&cmd[10]);
		cmd[10] = 0x0;
		command = HexStr2uint16(&cmd[6]);
		cmd[6] = 0x0;
		address = HexStr2uint16(&cmd[2]);
		cmd[2] = 0x0;
		protocol = HexStr2uint8(&cmd[0]);
		
		sendIR(protocol, address, command, flag);
	}
	PowerOpti_TIMER0_OFF;
#endif
}

void myIRMP::sendIR(unsigned int protocol, unsigned long address, unsigned long command, unsigned int flags) {

#if HAS_IR_TX
	IRMP_DATA irsnd_data;

	irsnd_data.protocol = protocol;
  irsnd_data.address  = address;
  irsnd_data.command  = command;
  irsnd_data.flags    = flags;

	irsnd_send_data (&irsnd_data, true);

	//#if HAS_POWER_OPTIMIZATIONS
	irsnd_data.protocol = 0; //undefined protocol 
	irsnd_send_data (&irsnd_data, true); //wait till IR command was sent
	//#endif

#endif
}

uint16_t myIRMP::getReceivedAddress(RecvData *rData) {
	IRMP_DATA *irmp_data = (IRMP_DATA*)rData->Data;
	return irmp_data->address;
}
uint16_t myIRMP::getReceivedCommand(RecvData *rData) {
	IRMP_DATA *irmp_data = (IRMP_DATA*)rData->Data;
	return irmp_data->command;
}
uint8_t myIRMP::getReceivedFlag(RecvData *rData) {
	IRMP_DATA *irmp_data = (IRMP_DATA*)rData->Data;
	return irmp_data->flags;
}

void myIRMP::displayData(RecvData *DataBuffer) {

	DH2(DataBuffer->DataTypeCode);
	DH4(getReceivedAddress(DataBuffer));
	DH4(getReceivedCommand(DataBuffer));
	DH2(getReceivedFlag(DataBuffer));
}

void myIRMP::printHelp() {

	DS_P("\n ## IR ##\n");
#if HAS_IR_TX
	DS_P(" * [TX] I<Protocol[2]><Address[4]><Command[4]><Flag-Repeats[2]>\n");
#endif
#if HAS_IR_RX	
	DS_P(" * [RX] Ir<1:on,0:off>\n");
#endif
}

/* helper function: attachInterrupt wants void(), but irmp_ISR is uint8_t() */
void myIRMP::timerinterrupt() {

#if HAS_IR_TX
  if (! irsnd_ISR() )         // call irsnd ISR
#endif
	{
#if HAS_IR_RX	
    irmp_ISR();             // call irmp ISR
#endif
  }
}
