
#include "myAVR_interrupt.h"

#ifdef INCLUDE_BUTTON
	byte myAVRInterrupt::myAVR_ButtonState = TRIGGER_BUTTON_NONE;
#endif

myAVRInterrupt::myAVRInterrupt(uint8_t irq, uint16_t debounce, uint8_t level /* 3:button fkt 2:pulse 1:High 0:Low */, uint8_t event)
#ifdef INCLUDE_BUTTON
		: cButton(irq, false)
#endif
{
	sTrigger.irqPin = (irq==INTOne?3:(irq==INTZero?2:irq));
	sTrigger.event = event;
	sTrigger.debounce = debounce;
	sTrigger.button = (level==3?1:0);
	sTrigger.pulse = (level==2 /*|| sTrigger.hold*/ ?1:0);
	sTrigger.level = (sTrigger.pulse?1:level);
}

void myAVRInterrupt::initialize() {

#ifdef INCLUDE_BUTTON
	if(sTrigger.button) {
		cButton.attachClick(myAVR_Button_Click);
		cButton.attachDoubleClick(myAVR_Button_DoubleClick);
		cButton.attachLongPressStart(myAVR_Button_LongClick);
	} else {
#endif
		sTrigger.triggerDetected=!sTrigger.level;
		sTrigger.triggerLevel=sTrigger.triggerDetected;
		sTrigger.triggerDetectedTime=0;
		cfgInterrupt(this, sTrigger.irqPin, Interrupt_Change);
#ifdef INCLUDE_BUTTON
 	}
#endif
}

void myAVRInterrupt::interrupt() {

	byte buf[2];
	buf[1] = sTrigger.irqPin;

	sTrigger.triggerLevel = digitalRead(sTrigger.irqPin);
	if(!sTrigger.triggerDetectedTime && sTrigger.triggerLevel==sTrigger.level) {
		sTrigger.triggerDetectedTime = millis();
		sTrigger.triggerDetected = sTrigger.triggerLevel;
		buf[0] = sTrigger.triggerDetected;
		addToRingBuffer((sTrigger.event!=MODULE_AVR_TRIGGER ? MODULE_DATAPROCESSING : MODULE_AVR_TRIGGER), sTrigger.event, buf, sizeof(byte)*2);
	}

	//DS_P("TRIGGER_INT: "); DU((triggerDetected?1:0),0); DC('>'); DU((triggerLevel?1:0),0); DC(' '); DU(TIMING::millis() - triggerDetectedTime,0); DNL();
}

bool myAVRInterrupt::poll() {

	byte buf[2];
	buf[1] = sTrigger.irqPin;

#ifdef INCLUDE_BUTTON
	if(sTrigger.button) {
		cButton.tick();
		if(myAVR_ButtonState != TRIGGER_BUTTON_NONE) {
			buf[0] = myAVR_ButtonState;
			myAVR_ButtonState=TRIGGER_BUTTON_NONE;
			addToRingBuffer((sTrigger.event!=MODULE_AVR_TRIGGER ? MODULE_DATAPROCESSING : MODULE_AVR_TRIGGER), sTrigger.event, buf, sizeof(byte)*2);
			return 1;
		}

	} else {
#endif

		if(sTrigger.triggerDetectedTime) {
			if(millis_since(sTrigger.triggerDetectedTime) > sTrigger.debounce) { //time is over

				if(DEBUG) { DS_P("TRIGGER: "); DU((sTrigger.triggerDetected?1:0),0); DC('>'); DU((sTrigger.triggerLevel?1:0),0); DC(' '); DU(millis_since(sTrigger.triggerDetectedTime),0); DNL(); }

				if(sTrigger.triggerDetected != sTrigger.triggerLevel) { //TriggerLevel Change
					sTrigger.triggerDetected = sTrigger.triggerLevel;
					if(!sTrigger.pulse || (sTrigger.pulse && sTrigger.triggerDetected)) {
						buf[0] = sTrigger.triggerDetected;
						addToRingBuffer((sTrigger.event!=MODULE_AVR_TRIGGER ? MODULE_DATAPROCESSING : MODULE_AVR_TRIGGER), sTrigger.event, buf, sizeof(byte)*2);
					}
				}
				sTrigger.triggerDetected = sTrigger.triggerLevel;
				sTrigger.triggerDetectedTime=0;
				return 1;
			}
		} else if(sTrigger.triggerDetected != sTrigger.triggerLevel) {
			sTrigger.triggerDetected = sTrigger.triggerLevel;
			if(!sTrigger.pulse || (sTrigger.pulse && sTrigger.triggerDetected)) {
				buf[0] = sTrigger.triggerDetected;
				addToRingBuffer((sTrigger.event!=MODULE_AVR_TRIGGER?MODULE_DATAPROCESSING:MODULE_AVR_TRIGGER), sTrigger.event, buf, sizeof(byte)*2);
			}
			return 1;
		}
#ifdef INCLUDE_BUTTON
	}
#endif

	return 0;
}

//void myAVRInterrupt::infoPoll(byte prescaler) {

//	char buf[2];
//	buf[1]=0x0;

//	DS_P("TRIGGER_info: "); DU((triggerDetected?1:0),0); DC('>'); DU((triggerLevel?1:0),0); DC(' '); DU(TIMING::millis(),0); DS("-"); DU(triggerDetectedTime,0); DNL();
//	if(triggerDetected != triggerLevel) {
//		triggerDetected = triggerLevel;
//		addToRingBuffer(MODULE_AVR_TRIGGER, 0, NULL, 0);
//	}
//}
/*
void myAVRInterrupt::send(char *cmd, uint8_t typecode) {

	unsigned long buf[2];

	switch(typecode) {

		case MODULE_AVR_TRIGGER:
			addToRingBuffer(MODULE_AVR_TRIGGER, sTrigger.irqPin, NULL, 0);
			break;

	}
}
*/
void myAVRInterrupt::displayData(RecvData *DataBuffer) {

	if((uint8_t)DataBuffer->Data[1] != sTrigger.irqPin)
		return;
	DU(DataBuffer->Data[1],0);
	DU(DataBuffer->Data[0],0);
}

//void printHelp() {
//	DS_P(" * [TRIGGER] N<Pin><0:low,1:high>\n");
//}
