
#ifndef _MY_DIGITALPIN_h
#define _MY_DIGITALPIN_h

//Copy and modified of https://github.com/mathertel/OneButton

#include <myBaseModule.h>

/******** DEFINE dependencies ******
	DIGITAL_PIN_EVENT: redirect pin change event to other modules (e.g. Rollo)
	DIGITAL_PIN_DEBOUNCE: see below
	DIGITAL_PIN_PULSE_DEBOUNCE: see below
	DIGITAL_PIN_LONG_PULSE_DEBOUNCE: see below
************************************/

#ifndef DIGITAL_PIN_EVENT
	#define DIGITAL_PIN_EVENT									MODULE_DIGITAL_PIN_EVENT
#endif

#define DIGITAL_PIN_DEBOUNCE								 20		// in HAS_POWER_OPTIMIZATIONS Debounce time need to be smaller than IDLE Time!
																									// number of millisec that have to pass by before a click is assumed as safe.
#define DIGITAL_PIN_PULSE_DEBOUNCE 					600		// number of millisec that have to pass by before a click is detected.
#define DIGITAL_PIN_LONG_PULSE_DEBOUNCE 	 1000 	// number of millisec that have to pass by before a long button press is detected.


#define PinMode_SIMPLE						(0)
#define PinMode_SR_RELAY					(1<<1)
#define PinMode_PULSE							(1<<2)
#define PinMode_PULSE_LOW					(1<<3)
#define PinMode_CLICK							(1<<4)
#define PinMode_DOUBLE_CLICK			(1<<5)
#define PinMode_LONG_CLICK_HOLD		(1<<6)
#define PinMode_LONG_CLICK				(1<<7)

#define PinMode_ALL_EVENTS				(PinMode_PULSE | PinMode_PULSE_LOW | PinMode_CLICK | PinMode_DOUBLE_CLICK | PinMode_LONG_CLICK_HOLD | PinMode_LONG_CLICK)

template <uint8_t Mode=PinMode_SIMPLE, uint8_t Pin1=0xFF, bool activeLow=false, uint8_t id=Pin1, uint8_t Pin2=0xFF>
class digitalPin : public myBaseModule {

private:
	uint8_t _DigitalIOEvent:8;

	// Pulse functions
	unsigned int _debounceTicks; // number of ticks for debounce times.
	unsigned int _clickTicks; // number of ticks that have to pass by before a click is detected
	unsigned int _pressTicks; // number of ticks that have to pass by before a long button press is detected

	bool _pulseReleased:1;
	bool _pulseActive:1;
	bool _pulseLocked:1;

	// These variables that hold information across the upcoming tick calls.
	// They are initialized once on program start and are updated every time the tick function is called.
	uint8_t _state:3; //0..6
	unsigned long _startTime; // will be set in state 1
	unsigned long _stopTime; // will be set in state 2

	void initPulseDetection()	{

	  _debounceTicks = DIGITAL_PIN_DEBOUNCE;      		// number of millisec that have to pass by before a click is assumed as safe.
	  _clickTicks = DIGITAL_PIN_PULSE_DEBOUNCE;       // number of millisec that have to pass by before a click is detected.
	  _pressTicks = DIGITAL_PIN_LONG_PULSE_DEBOUNCE;  // number of millisec that have to pass by before a long button press is detected.

	  _state = 0; // starting with state 0: waiting for button to be pressed
	  // _isLongPressed = false;  // Keep track of long press state

	  if (activeLow) {
	    // the button connects the input pin to GND when pressed.
	    _pulseReleased = HIGH; // notPressed
	    _pulseActive = LOW;

	    // use the given pin as input and activate internal PULLUP resistor.
	    pinMode(Pin1, INPUT_PULLUP );

	  } else {
	    // the button connects the input pin to VCC when pressed.
	    _pulseReleased = LOW;
	    _pulseActive = HIGH;

	    // use the given pin as input
	    pinMode(Pin1, INPUT);
	  } // if
		_pulseLocked = _pulseReleased;
	} // OneButton

	bool tickPulseDetection(void) {
		//Zustandsmaschine http://www.mathertel.de/Arduino/OneButtonLibrary.aspx
		bool ret = 0;

		ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {

		  // Detect the input information
		  int pulseLevel = digitalRead(Pin1); // current button signal.
		  unsigned long now = millis(); // current (relative) time in msecs.

		  // Implementation of the state machine
		  if (_state == 0) { // waiting for menu pin being pressed.
		    if (pulseLevel == _pulseActive) {
		      _state = 1; // step to state 1
		      _startTime = now; // remember starting time
					_pulseLocked = _pulseReleased;
		    } // if

		  } else if (_state == 1) { // waiting for menu pin being released.

				if ((pulseLevel == _pulseReleased) && (millis_since(_startTime) < _debounceTicks)) {
		      // button was released to quickly so I assume some debouncing.
			  	// go back to state 0 without calling a function.
					_state = 0;

				} else if ((pulseLevel == _pulseActive) && (_pulseLocked != _pulseActive) && (millis_since(_startTime) > _debounceTicks)) {
					_pulseLocked = _pulseActive;
					PulseEvent(PinMode_PULSE);
					ret=1;

		    } else if (pulseLevel == _pulseReleased) { //debounced
		      _state = 2; // step to state 2
		      _stopTime = now; // remember stopping time
					_pulseLocked = _pulseReleased;
					if(_pulseLocked == _pulseActive) PulseEvent(PinMode_PULSE);
					ret=1;

		    } else if ((pulseLevel == _pulseActive) && (millis_since(_startTime) > _pressTicks)) {
		      _state = 6; // step to state 6
					PulseEvent(PinMode_LONG_CLICK_HOLD);
					ret=1;

		    } else {
		      // wait. Stay in this state.
		    } // if

		  } else if (_state == 2) { // waiting for menu pin being pressed the second time or timeout.
				if(_pulseLocked == _pulseReleased) {
					_pulseLocked = _pulseActive; //workaorund...not really Pressed
					PulseEvent(PinMode_PULSE_LOW);
					ret=1;

				} else if (/*_doubleClickFunc == NULL || */millis_since(_startTime) > _clickTicks) {
		      // this was only a single short click
		      _state = 0; // restart.
					PulseEvent(PinMode_CLICK);
					ret=1;

		    } else if ((pulseLevel == _pulseActive) && (millis_since(_stopTime) > _debounceTicks)) {
		      _state = 3; // step to state 3
		      _startTime = now; // remember starting time
					ret=1;
		    } // if

		  } else if (_state == 3) { // waiting for menu pin being released finally.
		    // Stay here for at least _debounceTicks because else we might end up in state 1 if the
		    // button bounces for too long.
		    if (pulseLevel == _pulseReleased && (millis_since(_startTime) > _debounceTicks)) {
		      // this was a 2 click sequence.
		      _state = 0; // restart.
					PulseEvent(PinMode_PULSE_LOW);
					PulseEvent(PinMode_DOUBLE_CLICK);
					ret=1;
		    } // if

		  } else if (_state == 6) { // waiting for menu pin being release after long press.
		    if (pulseLevel == _pulseReleased) {
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
		buf[0] = 0xFF;
  	buf[1] = id;

		event &= Mode; //check if active
		if(!event) return;

		if(event & PinMode_PULSE_LOW) {
			D_DS_P("Low\n");
			buf[0] = 0;
		} else if (event & PinMode_PULSE) {
			D_DS_P("High\n");
			buf[0] = 1;
		} else if (event & PinMode_CLICK) {
			D_DS_P("Pulse\n");
			buf[0] = 2;
		} else if (event & PinMode_LONG_CLICK_HOLD) {
			D_DS_P("LongPulseHold\n");
			buf[0] = 3;
		} else if (event & PinMode_LONG_CLICK) {
			D_DS_P("LongPulse\n");
			buf[0] = 4;
		} else if (event & PinMode_DOUBLE_CLICK) {
			D_DS_P("DoublePulse\n");
			buf[0] = 5;
		}

		if(buf[0] != 0xFF) {
			addToRingBuffer((_DigitalIOEvent!=MODULE_DIGITAL_PIN_EVENT?MODULE_DATAPROCESSING:MODULE_DIGITAL_PIN_EVENT), _DigitalIOEvent, buf, sizeof(byte)*2);
		}
	}

  void Relay_OnOff(bool on) {
    if(Mode & PinMode_SR_RELAY) {
      digitalWrite(Pin1, on);
      digitalWrite(Pin2, !on);
    	delay(50);
      digitalWrite(Pin1, 0);
      digitalWrite(Pin2, 0);
    } else if (Mode & PinMode_SIMPLE) {
			digitalWrite(Pin1, on);
    }
  }

public:
  digitalPin() {
		_DigitalIOEvent = DIGITAL_PIN_EVENT;
  }

	const char* getFunctionCharacter() { return "SSN"; };

  void initialize() {
    if(Mode & PinMode_SR_RELAY) {
      pinMode(Pin1, OUTPUT);
  	  pinMode(Pin2, OUTPUT);

		} else if (Mode & PinMode_SIMPLE){
			pinMode(Pin1, OUTPUT);

		} else if(Mode & PinMode_ALL_EVENTS) {
			initPulseDetection();
			cfgInterrupt(this, Pin1, Interrupt_Change);
		}
  }

	void interrupt() {
		poll();
	}

  bool poll() {

		if(Mode & PinMode_ALL_EVENTS) {
			return tickPulseDetection();
    }
  	return 0;
  }

  void send(char *cmd, uint8_t typecode) {
    if((cmd[0]-'0')!=id) return;
  	switch(typecode) {

      case MODULE_DIGITAL_PIN_RELAY_SR:
        Relay_OnOff((cmd[1]=='0' ? 0 : 1));
      break;

			case MODULE_DIGITAL_PIN_SIMPLE:
				Relay_OnOff((cmd[1]=='0' ? 0 : 1));
			break;
    }
  }

  void displayData(RecvData *DataBuffer) {

  	if((uint8_t)DataBuffer->Data[1] != id)
  		return;
  	DU(DataBuffer->Data[1],0);
  	DU(DataBuffer->Data[0],0);
  }

  void printHelp() {

    DS_P("\n ## DIGITAL PIN - ID:");DU(id,0);DS(" ##\n");

    if(Mode == PinMode_SR_RELAY) {
    	DS_P(" * [RELAY S&R]  S<ID><0:off,1:on>\n");
		} else if(Mode == PinMode_SIMPLE) {
			DS_P(" * [DIGITAL]  	S<ID><0:off,1:on>\n");
    }
		if(Mode & PinMode_ALL_EVENTS) {
  		DS_P(" * [PULSE] N<ID><");
			if(Mode & PinMode_PULSE_LOW) DS_P("0:low,");
			if(Mode & PinMode_PULSE) DS_P("1:high,");
			if(Mode & PinMode_CLICK) DS_P("2:pulse,");
			if(Mode & PinMode_LONG_CLICK_HOLD) DS_P("3:longPulseHold,");
			if(Mode & PinMode_LONG_CLICK) DS_P("4:longPulse,");
			if(Mode & PinMode_DOUBLE_CLICK) DS_P("5:doublePulse");
			DS_P(">\n");
    }
  }
};

#endif
