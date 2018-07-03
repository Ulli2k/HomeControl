
#include "myRollo.h"

volatile byte myROLLO::waiting4stop = 0;

void myROLLO_Interrupt_Function() {
	myROLLO::waiting4stop=2;
	TIMING::detachTimeInterrupt(&myROLLO_Interrupt_Function);
}

void myROLLO::initialize() {

	waiting4stop = 0;
	maxMovingTime = ROLLO_MAX_MOVING_TIME;

	digitalWrite(ROLLO_POWER_PIN, 0); //Wichtig: Erst ausschalten dann als Output konfigurieren
	pinMode(ROLLO_POWER_PIN, OUTPUT);
	TIMING::delay(ROLLO_POWER_OFF_DELAY);

	digitalWrite(ROLLO_UP_DOWN_PIN, 0); //Wichtig: Erst ausschalten dann als Output konfigurieren
	pinMode(ROLLO_UP_DOWN_PIN, OUTPUT);

}

bool myROLLO::poll() {

	if(waiting4stop) {
		if(waiting4stop == 2) { // End of moving
			//byte directionUp = (digitalRead(ROLLO_UP_DOWN_PIN)? 2 : 1);
			SwitchUpDown("0");
			//addToRingBuffer(MODULE_ROLLO_CMD, 0, &directionUp, 1);
		}
		return 1;
	}
	return 0;
}

//void myROLLO::infoPoll(byte prescaler) {
//
//}

void myROLLO::send(char *cmd, uint8_t typecode) {

	switch(typecode) {

		case MODULE_ROLLO_CMD:
			if(cmd[0] != 't') {
				SwitchUpDown(cmd);

			} else { //set max moving time
				if(cmd[1] != 0x0) {
					maxMovingTime = atoi(cmd+1);
				} else {
					addToRingBuffer(MODULE_ROLLO_CMD, 't', 0, 0);
				}
			}
			break;

		case MODULE_ROLLO_EVENT:
			if(DEBUG) {	DS_P("EVENT - ");DU(cmd[0],0);DNL(); }
			byte directionUp;
			switch(cmd[0]) {
				case 1: //press
					if(!waiting4stop) { //not moving --> down
						SwitchUpDown("1"); //down
						directionUp = 1;
						addToRingBuffer(MODULE_ROLLO_CMD, 0, &directionUp, 1);
					} else {
						SwitchUpDown("0"); //stop
						directionUp = 0;
						addToRingBuffer(MODULE_ROLLO_CMD, 0, &directionUp, 1);
					}
					break;
				case 3:
					SwitchUpDown("2"); //Up
					directionUp = 2;
					addToRingBuffer(MODULE_ROLLO_CMD, 0, &directionUp, 1);
					break;
			}
			break;
	}
}

void myROLLO::SwitchUpDown(const char* cmd) {

	uint16_t sdelay=maxMovingTime; //[s*10]

	if(strlen(cmd)>1) {
		sdelay = (uint16_t)atoi(cmd+1);
	}

	//Shutoff SSR / Power
	if(DEBUG) {	DS_P("Power OFF\n"); }
  digitalWrite(ROLLO_POWER_PIN, 0);
  TIMING::delay(ROLLO_POWER_OFF_DELAY);//ms 50Hz supply frequence --> 0.02s

	//Up/Down/off
  switch(cmd[0]) {
  	case '1': //Down
  		if(DEBUG) {	DS_P("Power DOWN 0\n"); }
	    digitalWrite(ROLLO_UP_DOWN_PIN, 0);
    break;
		case '2':	//Up
			if(DEBUG) {	DS_P("Power UP 1\n"); }
	  	digitalWrite(ROLLO_UP_DOWN_PIN, 1);
	  break;
	  default: //Stop
	  	if(DEBUG) {	DS("Stop\n"); }
	  	digitalWrite(ROLLO_UP_DOWN_PIN, 0);
	  	waiting4stop=0;
	  	TIMING::detachTimeInterrupt(&myROLLO_Interrupt_Function);
	  	return;
	}

	if(DEBUG) {	DS("Power ON\n"); }
	digitalWrite(ROLLO_POWER_PIN, 1);

	if(!sdelay) { waiting4stop=0; return; }
	if(DEBUG) { DU(sdelay*100-ROLLO_POWER_OFF_DELAY,1);DNL(); }

	waiting4stop=1;
	TIMING::detachTimeInterrupt(&myROLLO_Interrupt_Function);
	TIMING::attachTimeInterrupt(&myROLLO_Interrupt_Function, (sdelay*100-ROLLO_POWER_OFF_DELAY) );
}

void myROLLO::displayData(RecvData *DataBuffer) {

	//unsigned long buf[2];

	switch(DataBuffer->ModuleID) {
		case  MODULE_ROLLO_CMD:
			if(DataBuffer->DataTypeCode == 't') {
				DC('t');
				DU(maxMovingTime,0);
			} else {
				DU(DataBuffer->Data[0], 0);
			}
		break;
	}
}

void myROLLO::printHelp() {

	DS_P("\n ## ROLLO-SWITCH ##\n");
	DS_P(" * [ROLLO]  	J<0:off,1:down,2:up><sec*10>\n");
	DS_P(" * [MOV-TIME]	Jt<sec*10>\n");
}
