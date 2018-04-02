
//#include <myInterrupt.h>
#include <myBaseModule.h> // class myBaseModule für cfgInterrupt

// class myBaseModule;
myBaseModule *INT0Module = NULL;
myBaseModule *INT1Module = NULL;
myBaseModule *PinChgModule = NULL;
myBaseModule *PinChgModule1 = NULL;

void myInterrupt::cfgInterrupt(myBaseModule *module, uint8_t irqPin, byte state) {

#if defined(__AVR_ATmega328P__)
	switch(irqPin) {
		case INTZero:
			if(state==Interrupt_Off) {
				detachInterrupt(0);
				INT0Module= NULL;

			} else if(state==Interrupt_Block) {
				bitClear(EIMSK, INT0);
				if(module!=NULL) INT0Module= NULL;

			} else if(state==Interrupt_Release) {
				if(module!=NULL) INT0Module = module;
				if(INT0Module!=NULL)
					bitSet(EIMSK, INT0); //sicher gehen das Interrupt überhaut aktiviert wurde

			} else {
				INT0Module = module;
				detachInterrupt(0); //be sure that nobody else already attached the interrupt
				attachInterrupt(0, INT0_interrupt, (state==Interrupt_Low ? LOW : state==Interrupt_High ? HIGH : state==Interrupt_Falling ? FALLING : state==Interrupt_Rising ? RISING : CHANGE));
			}
		break;

		case INTOne:
			if(state==Interrupt_Off) {
				detachInterrupt(1);
				INT1Module = NULL;

			} else if(state==Interrupt_Block) {
				bitClear(EIMSK, INT1);
				if(module!=NULL) INT1Module= NULL;

			} else if(state==Interrupt_Release) {
				if(module!=NULL) INT1Module = module;
				if(INT1Module!=NULL)
					bitSet(EIMSK, INT1); //sicher gehen das Interrupt überhaut aktiviert wurde

			} else {
				INT1Module = module;
				detachInterrupt(1); //be sure that nobody else already attached the interrupt
				attachInterrupt(1, INT1_interrupt, (state==Interrupt_Low ? LOW : state==Interrupt_High ? HIGH : state==Interrupt_Falling ? FALLING : state==Interrupt_Rising ? RISING : CHANGE));
			}
		break;

		case 6:
			if (state==Interrupt_Off) {
				detachPinChangeInterrupt(irqPin);
				PinChgModule1 = NULL;

			} else if (state==Interrupt_Block) {
				if (irqPin < 8) {
						bitClear(PCICR, PCIE2);
				} else if (irqPin < 14) {
						bitClear(PCICR, PCIE0);
				} else {
						bitClear(PCICR, PCIE1);
				}
				if(module!=NULL) PinChgModule1 = NULL;

			} else if (state==Interrupt_Release) {
				if(module!=NULL) PinChgModule1 = module;
				if(PinChgModule1!=NULL) {//sicher gehen das Interrupt überhaut aktiviert wurde
					if (irqPin < 8) {
							bitSet(PCICR, PCIE2);
					} else if (irqPin < 14) {
							bitSet(PCICR, PCIE0);
					} else {
							bitSet(PCICR, PCIE1);
					}
				}

			} else {
				PinChgModule1 = module;
				detachPinChangeInterrupt(irqPin);
				attachPinChangeInterrupt(irqPin,PinChg_interrupt1,(state==Interrupt_Falling ? FALLING : state==Interrupt_Rising ? RISING : CHANGE));
			}

		break;

		default://case PINCHG:
			if (state==Interrupt_Off) {
				detachPinChangeInterrupt(irqPin);
				PinChgModule = NULL;

			} else if (state==Interrupt_Block) {
				if (irqPin < 8) {
						bitClear(PCICR, PCIE2);
				} else if (irqPin < 14) {
						bitClear(PCICR, PCIE0);
				} else {
						bitClear(PCICR, PCIE1);
				}
				if(module!=NULL) PinChgModule = NULL;

			} else if (state==Interrupt_Release) {
				if(module!=NULL) PinChgModule = module;
				if(PinChgModule!=NULL) {//sicher gehen das Interrupt überhaut aktiviert wurde
					if (irqPin < 8) {
							bitSet(PCICR, PCIE2);
					} else if (irqPin < 14) {
							bitSet(PCICR, PCIE0);
					} else {
							bitSet(PCICR, PCIE1);
					}
				}

			} else {
				PinChgModule = module;
				detachPinChangeInterrupt(irqPin);
				attachPinChangeInterrupt(irqPin,PinChg_interrupt,(state==Interrupt_Falling ? FALLING : state==Interrupt_Rising ? RISING : CHANGE));
			}

		break;
	}

	// if(state==Interrupt_Off || state==Interrupt_Block) {
	// 	module->_irqPin = 0xFF;
	// 	module->_irqState = 0xFF;
	// } else {
	// 	module->_irqPin = irqPin;
	// 	module->_irqState = state;
	// }
#endif
}
//
// void myInterrupt::_interrupt() {
// 	const typeModuleInfo* pmt = ModuleTab;
// 	while(pmt->typecode >= 0) {
// 		if(pmt->module->_irqPin != 0xFF &&
// 				digitalRead(pmt->module->_irqPin) == (pmt->module->_irqState == Interrupt_High || pmt->module->_irqState == Interrupt_Rising)
// 			) {
// 			if(DEBUG) { DS_P("IRQ:");DU(pmt->module->_irqPin,0); }
// 			pmt->module->interrupt();
// 		}
// 		pmt++;
// 	}
// }

void myInterrupt::INT1_interrupt() {
	if(INT1Module!=NULL)
			INT1Module->interrupt();
}

void myInterrupt::INT0_interrupt() {
	if(INT0Module!=NULL)
			INT0Module->interrupt();
}

void myInterrupt::PinChg_interrupt() {
	if(PinChgModule!=NULL)
			PinChgModule->interrupt();
}

void myInterrupt::PinChg_interrupt1() {
	if(PinChgModule1!=NULL)
			PinChgModule1->interrupt();
}