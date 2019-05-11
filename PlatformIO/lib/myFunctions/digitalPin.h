
#ifndef _MY_DIGITALPIN_h
#define _MY_DIGITALPIN_h

//Copy and modified of https://github.com/mathertel/OneButton

#include <myBaseModule.h>

/******** DEFINE dependencies ******
Button:
	DIGITAL_PIN_EVENT: redirect pin change event to other modules (e.g. Rollo)
	DIGITAL_PIN_DEBOUNCE: see below
	DIGITAL_PIN_PULSE_DEBOUNCE: see below
	DIGITAL_PIN_LONG_PULSE_DEBOUNCE: see below
************************************/

class ArduinoPins {
	uint8_t pin;
	bool activeLow;	// true -> the button connects the input pin to GND when pressed.
public:
	ArduinoPins(uint8_t p, bool aLow) 			{ activeLow=aLow; pin=p; }
  inline void setOutput()  								{ setLow(); pinMode(pin,OUTPUT); }
	inline void setInput(bool pullUp)				{ pinMode(pin, (pullUp?INPUT_PULLUP:INPUT) ); }
  inline void setInput()    							{ pinMode(pin,INPUT);  }
  inline void setHigh()     							{ digitalWrite(pin,(activeLow?LOW:HIGH)); }
  inline void setLow()      							{ digitalWrite(pin,(activeLow?HIGH:LOW)); }
	inline void setState (bool on)					{ if(on) { setHigh(); } else { setLow(); } }
  inline bool getState() 							{ return (activeLow?(digitalRead(pin)==LOW):(digitalRead(pin)==HIGH)); }
};

#define DIGITAL_PIN_MAX_ON_TIME					1000 //[ms]
#define DIGITAL_PIN_MAX_ON_TIME_VALUE		0xFFFFFFFF

class libDigitalPin : public Alarm {

private:
	uint8_t ID;
	uint32_t maxOnTime; //ms
	bool tiggerState;
	virtual void switchState(uint8_t state);
	virtual void receivedEvent(uint8_t state);

public:
	const char* getFunctionCharacter() { return "N"; };
  libDigitalPin(uint8_t id, uint32_t onTime=DIGITAL_PIN_MAX_ON_TIME) : Alarm(0) { ID=id; maxOnTime=onTime; tiggerState=false; async(true); }

	bool isRightID(uint8_t id) { return (id==ID); }

	bool isTimerRunning() { return active(); }
	void stopTimer() { sysclock.cancel(*this); }
	void setTimer(uint32_t ms) {
		ms = (ms==DIGITAL_PIN_MAX_ON_TIME_VALUE?maxOnTime:ms);
		stopTimer();
		tick=millis2ticks(ms);
		sysclock.add(*this);
		if(DEBUG) { DS_P("Timer: ");DU(ms,0);DNL(); }
	}

	virtual void trigger (__attribute__((unused)) AlarmClock& clock) {
		ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
			switchState((uint8_t)tiggerState);
		}
	}

	void sendlib(char *cmd, uint8_t typecode) {
		if(!isRightID(cmd[0]-'0')) return;
		cmd++;
		uint32_t sdelay = 0;
		uint8_t state = 0;

		switch(typecode) {
				case MODULE_DIGITAL_PIN_FKT:

					if(cmd[0] == 't' && strlen(cmd+1)>1) { // set new maxOnTime
						maxOnTime = atoi(cmd+1)*100; //[ms]
						if(DEBUG) { DS_P("OnTime: ");DU(maxOnTime,0);DNL(); }
						return;
					} else if( cmd[0] == 't' && strlen(cmd+1)==0) { // print maxOnTime
						DS_P("OnTime: ");DU(maxOnTime,0);DNL();
						return;
					}

					//on/off with Timer
					if(cmd[0] == 't') { // on/off based on maxOnTime
						sdelay = maxOnTime;
						state = cmd[1]-'0';
					} else if(strlen(cmd) > 1) { // on/off based on defined Time
						sdelay = (uint32_t)(atoi(cmd+1)*100); //[s*10]->[ms]
						state = cmd[0]-'0';
					} else { //simple On/Off
						state = cmd[0]-'0';
					}
					tiggerState=(state!=0?0:1);
					if(sdelay) {
						stopTimer();
						setTimer(sdelay);
					}
					switchState(state);
					break;

				case MODULE_DIGITAL_PIN_EVENT: // ID undependant
					if(DEBUG) {	DS_P("EVENT - ");DC(cmd[0]);DNL(); }
					receivedEvent(cmd[0]-'0');
					break;
		}
	}

  void displaylibData(RecvData *DataBuffer) {
		if(!isRightID(DataBuffer->Data[0]-'0'))
  		return;
  	DC(DataBuffer->Data[0]);
  	DC(DataBuffer->Data[1]);
  }

  void printlibHelp(bool incTimeFkt=true) {
    DS_P("\n ## DIGITAL PIN - ID:");DU(ID,0);DS_P(" ##\n");
		if(!incTimeFkt) return;
		DS_P(" * [setTIME] N<id>t<sec*10>\n");
		DS_P(" * [TIMED]   N<id>t<pinFkt>\n");
  }
};

/******************************************** SimplePin ********************************************/
template <uint8_t Pin1, bool activeLow=false, uint8_t id=Pin1, uint32_t onTime=DIGITAL_PIN_MAX_ON_TIME>
class simplePin : private libDigitalPin, public myBaseModule {

	ArduinoPins	dPin;

public:
	const char* getFunctionCharacter() { return libDigitalPin::getFunctionCharacter(); };
	simplePin() : dPin(Pin1,activeLow), libDigitalPin(id, onTime) {}

	void receivedEvent(uint8_t state) {}

	void initialize() {
		dPin.setOutput();
	}
	void send(char *cmd, uint8_t typecode) {
		sendlib(cmd, typecode);
	}
	void switchState(uint8_t state) {
		dPin.setState(state);
	}

	void printHelp() {
		printlibHelp();
		DS_P(" * [digPin]  N<ID><0:off,1:on><sec*10>\n");
	}
};

/******************************************** TwoWayRelay ********************************************/
template <uint8_t Pin1, uint8_t Pin2, uint8_t id=Pin1>
class twoWayRelay  : private libDigitalPin, public myBaseModule {

	ArduinoPins	dPin1;
	ArduinoPins	dPin2;

public:
	const char* getFunctionCharacter() { return libDigitalPin::getFunctionCharacter(); };
	twoWayRelay() : dPin1(Pin1,false), dPin2(Pin2,false), libDigitalPin(id) {};

	void receivedEvent(uint8_t state) {}

	void initialize() {
		dPin1.setOutput();
		dPin2.setOutput();
	}
	void switchState(uint8_t state) {
		dPin1.setState(state);
		dPin2.setState(!state);
		delay(50);
		dPin1.setLow();
		dPin2.setLow();
	}
	void send(char *cmd, uint8_t typecode) {
		sendlib(cmd, typecode);
	}
	void printHelp() {
		printlibHelp();
		DS_P(" * [RELAY]  N<ID><0:off,1:on><sec*10>\n");
	}
};

/******************************************** BUTTON ********************************************/
// #ifndef DIGITAL_PIN_EVENT
// 	#define DIGITAL_PIN_EVENT									MODULE_DIGITAL_PIN_EVENT
// #endif

#define DIGITAL_PIN_DEBOUNCE								 20		// in HAS_POWER_OPTIMIZATIONS Debounce time need to be smaller than IDLE Time!
																									// number of millisec that have to pass by before a click is assumed as safe.
#define DIGITAL_PIN_PULSE_DEBOUNCE 					600		// number of millisec that have to pass by before a click is detected.
#define DIGITAL_PIN_LONG_PULSE_DEBOUNCE 	 1000 	// number of millisec that have to pass by before a long button press is detected.

#define PinMode_PULSE							(1<<1)
#define PinMode_PULSE_LOW					(1<<2)
#define PinMode_CLICK							(1<<3)
#define PinMode_DOUBLE_CLICK			(1<<4)
#define PinMode_LONG_CLICK_HOLD		(1<<5)
#define PinMode_LONG_CLICK				(1<<6)

#define PinMode_ALL_EVENTS				(PinMode_PULSE | PinMode_PULSE_LOW | PinMode_CLICK | PinMode_DOUBLE_CLICK | PinMode_LONG_CLICK_HOLD | PinMode_LONG_CLICK)

template <uint8_t Mode, uint8_t Pin1, bool activeLow=false, uint8_t id=Pin1, uint8_t EventID=id, uint8_t PinEvent=MODULE_DIGITAL_PIN_EVENT>
class Button : private libDigitalPin, public myBaseModule {

	ArduinoPins	dPin;
	// uint8_t _DigitalIOEvent:8;

	// Pulse functions
	unsigned int _debounceTicks; // number of ticks for debounce times.
	unsigned int _clickTicks; // number of ticks that have to pass by before a click is detected
	unsigned int _pressTicks; // number of ticks that have to pass by before a long button press is detected

	// bool _pulseReleased:1;
	// bool _pulseActive:1;
	bool _pulseLocked:1;

	// These variables that hold information across the upcoming tick calls.
	// They are initialized once on program start and are updated every time the tick function is called.
	uint8_t _state:3; //0..6
	unsigned long _startTime; // will be set in state 1
	unsigned long _stopTime; // will be set in state 2

public:
	const char* getFunctionCharacter() { return libDigitalPin::getFunctionCharacter(); };
	Button() : dPin(Pin1, activeLow), libDigitalPin(id) { }

	void switchState(uint8_t state) {}
	void receivedEvent(uint8_t state) {}
	bool poll() { return tickPulseDetection(); }
	void interrupt() { tickPulseDetection(); }

	void initialize() {

		  _debounceTicks = DIGITAL_PIN_DEBOUNCE;      		// number of millisec that have to pass by before a click is assumed as safe.
		  _clickTicks = DIGITAL_PIN_PULSE_DEBOUNCE;       // number of millisec that have to pass by before a click is detected.
		  _pressTicks = DIGITAL_PIN_LONG_PULSE_DEBOUNCE;  // number of millisec that have to pass by before a long button press is detected.

		  _state = 0; // starting with state 0: waiting for button to be pressed
		  // _isLongPressed = false;  // Keep track of long press state

		  if (activeLow) {
		    // the button connects the input pin to GND when pressed.
		    // _pulseReleased = HIGH; // notPressed
		    // _pulseActive = LOW;

		    // use the given pin as input and activate internal PULLUP resistor.
				dPin.setInput(true);
		    // pinMode(Pin1, INPUT_PULLUP );
				_pulseLocked = HIGH;
		  } else {
		    // the button connects the input pin to VCC when pressed.
		    // _pulseReleased = LOW;
		    // _pulseActive = HIGH;

		    // use the given pin as input
				dPin.setInput(false);
				// pinMode(Pin1, INPUT);
				_pulseLocked = LOW;
		  } // if
			// _pulseLocked = _pulseReleased;

			cfgInterrupt(this, Pin1, Interrupt_Change);
	}

	bool tickPulseDetection(void) {
		//Zustandsmaschine http://www.mathertel.de/Arduino/OneButtonLibrary.aspx
		bool ret = 0;

		ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {

		  // Detect the input information
		  bool pulseLevel = dPin.getState(); // current button signal.
			// int pulseLevel = digitalRead(Pin1); // current button signal.
		  unsigned long now = millis(); // current (relative) time in msecs.

		  // Implementation of the state machine
		  if (_state == 0) { // waiting for menu pin being pressed.
		    if (pulseLevel) {
		      _state = 1; // step to state 1
		      _startTime = now; // remember starting time
					_pulseLocked = LOW;
		    } // if

		  } else if (_state == 1) { // waiting for menu pin being released.

				if ((!pulseLevel) && (millis_since(_startTime) < _debounceTicks)) {
		      // button was released to quickly so I assume some debouncing.
			  	// go back to state 0 without calling a function.
					_state = 0;

				} else if ((pulseLevel) && (!_pulseLocked) && (millis_since(_startTime) > _debounceTicks)) {
					_pulseLocked = HIGH;
					PulseEvent(PinMode_PULSE);
					ret=1;

		    } else if (!pulseLevel) { //debounced
		      _state = 2; // step to state 2
		      _stopTime = now; // remember stopping time
					_pulseLocked = LOW;
					if(_pulseLocked) PulseEvent(PinMode_PULSE);
					ret=1;

		    } else if ((pulseLevel) && (millis_since(_startTime) > _pressTicks)) {
		      _state = 6; // step to state 6
					PulseEvent(PinMode_LONG_CLICK_HOLD);
					ret=1;

		    } else {
		      // wait. Stay in this state.
		    } // if

		  } else if (_state == 2) { // waiting for menu pin being pressed the second time or timeout.
				if(!_pulseLocked) {
					_pulseLocked = HIGH; //workaorund...not really Pressed
					PulseEvent(PinMode_PULSE_LOW);
					ret=1;

				} else if (/*_doubleClickFunc == NULL || */millis_since(_startTime) > _clickTicks) {
		      // this was only a single short click
		      _state = 0; // restart.
					PulseEvent(PinMode_CLICK);
					ret=1;

		    } else if ((pulseLevel) && (millis_since(_stopTime) > _debounceTicks)) {
		      _state = 3; // step to state 3
		      _startTime = now; // remember starting time
					ret=1;
		    } // if

		  } else if (_state == 3) { // waiting for menu pin being released finally.
		    // Stay here for at least _debounceTicks because else we might end up in state 1 if the
		    // button bounces for too long.
		    if (!pulseLevel && (millis_since(_startTime) > _debounceTicks)) {
		      // this was a 2 click sequence.
		      _state = 0; // restart.
					PulseEvent(PinMode_PULSE_LOW);
					PulseEvent(PinMode_DOUBLE_CLICK);
					ret=1;
		    } // if

		  } else if (_state == 6) { // waiting for menu pin being release after long press.
		    if (!pulseLevel) {
			    _state = 0; // restart.
					PulseEvent(PinMode_PULSE_LOW);
					PulseEvent(PinMode_LONG_CLICK);
					ret=1;
		    } else {
			  	// button is being long pressed
		    } // if
		  } // if
		}
		return ret;
	} // OneButton.tick()

	void PulseEvent(uint8_t event) {
		uint8_t buf[2];
		buf[0] = EventID+'0';
		buf[1] = 0xFF;

		event &= Mode; //check if active
		if(!event) return;

		if(event & PinMode_PULSE_LOW) {
			D_DS_P("Low\n");
			buf[1] = '0';
		} else if (event & PinMode_PULSE) {
			D_DS_P("High\n");
			buf[1] = '1';
		} else if (event & PinMode_CLICK) {
			D_DS_P("Pulse\n");
			buf[1] = '2';
		} else if (event & PinMode_LONG_CLICK_HOLD) {
			D_DS_P("LongPulseHold\n");
			buf[1] = '3';
		} else if (event & PinMode_LONG_CLICK) {
			D_DS_P("LongPulse\n");
			buf[1] = '4';
		} else if (event & PinMode_DOUBLE_CLICK) {
			D_DS_P("DoublePulse\n");
			buf[1] = '5';
		}

		if(buf[1] != 0xFF) {
			addToRingBuffer(( (PinEvent!=MODULE_DIGITAL_PIN_EVENT || EventID!=id)?MODULE_DATAPROCESSING:MODULE_DIGITAL_PIN_EVENT), PinEvent, buf, sizeof(byte)*2);
		}
	}

	void displayData(RecvData *DataBuffer) {
		displaylibData(DataBuffer);
	}

	void printHelp() {
		printlibHelp(false);
		DS_P(" * [PULSE] N<ID><");
		if(Mode & PinMode_PULSE_LOW) DS_P("0:low,");
		if(Mode & PinMode_PULSE) DS_P("1:high,");
		if(Mode & PinMode_CLICK) DS_P("2:pulse,");
		if(Mode & PinMode_LONG_CLICK_HOLD) DS_P("3:longPulseHold,");
		if(Mode & PinMode_LONG_CLICK) DS_P("4:longPulse,");
		if(Mode & PinMode_DOUBLE_CLICK) DS_P("5:doublePulse");
		DS_P(">\n");
	}
};


/******************************************** Rollo ********************************************/

#define ROLLO_MAX_MOVING_TIME		21000		//[ms]
#define ROLLO_POWER_OFF_DELAY		200 		//[ms]

template <uint8_t PowerPin, uint8_t UpDownPin, uint8_t id=PowerPin>
class Rollo: private libDigitalPin, public myBaseModule {

	ArduinoPins	dPinPower;
	ArduinoPins	dPinDirection;

public:
	const char* getFunctionCharacter() { return libDigitalPin::getFunctionCharacter(); };
	Rollo() : dPinPower(PowerPin,false), dPinDirection(UpDownPin,false), libDigitalPin(id,ROLLO_MAX_MOVING_TIME) {} //maxMovingTime = ROLLO_MAX_MOVING_TIME;

	void initialize() {
		dPinPower.setOutput();
		delay(ROLLO_POWER_OFF_DELAY);
		dPinDirection.setOutput();
	}

	void send(char *cmd, uint8_t typecode) {
		sendlib(cmd, typecode);
	}

	void receivedEvent(uint8_t state) {
			uint8_t buf[2];
			buf[0] = id+'0';
			switch(state) {
				case 1: //press
					if(!isTimerRunning()) { //not moving --> down
//TODO: switchState muss über den RingBuffer um die Alarm/Time funktion zu nutzen!
						switchState(1); //down
						buf[1] = '1';
						addToRingBuffer(MODULE_DIGITAL_PIN_FKT, 0, buf, sizeof(uint8_t)*2);
					} else {
						switchState(0); //stop
						buf[1] = '0';
						addToRingBuffer(MODULE_DIGITAL_PIN_FKT, 0, buf, sizeof(uint8_t)*2);
					}
					break;
				case 3:
					switchState(2); //Up
					buf[1] = '2';
					addToRingBuffer(MODULE_DIGITAL_PIN_FKT, 0, buf, sizeof(uint8_t)*2);
					break;
			}
	}

	void switchState(uint8_t state) {

		//Shutoff SSR / Power
		D_DS_P("Power OFF\n");
		dPinPower.setLow();
		if(state!=0) { //workaround: delay is not possible during interrupt --> trigger switchState(0);
	  	delay(ROLLO_POWER_OFF_DELAY);//ms 50Hz supply frequence --> 0.02s
		}

		//Up/Down/off
	  switch(state) {
	  	case 1: //Down
	  		D_DS_P("Power DOWN 0\n")
				dPinDirection.setLow();
	    break;
			case 2:	//Up
				D_DS_P("Power UP 1\n");
				dPinDirection.setHigh();
		  break;
		  default: //Stop
		  	D_DS_P("Stop\n");
		  	dPinDirection.setLow();
				stopTimer();
		  	return;
		}

		D_DS_P("Power ON\n");
		dPinPower.setHigh();
	}

	void displayData(RecvData *DataBuffer) {
		displaylibData(DataBuffer);
	}

	void printHelp() {
		printlibHelp();
		DS_P(" * [ROLLO]   N<id><0:off,1:down,2:up><sec*10>\n");
	}
};

#endif
