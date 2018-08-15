
#ifndef _MY_LED_h
#define _MY_LED_h

#include <myBaseModule.h>

/******** DEFINE dependencies ******
  DELAY_ACTIVITY_LED: delay between activity led goes off/on
	LED_PIN_INVERSE: switch between active low or high
************************************/

#ifndef DELAY_ACTIVITY_LED
	#define DELAY_ACTIVITY_LED	200
#endif
#ifdef LED_PIN_INVERSE
	#define bLED_PIN_INVERSE	true
#else
	#define bLED_PIN_INVERSE	false
#endif

template <uint8_t LEDpin, bool LEDinverse=bLED_PIN_INVERSE, bool Activity=LED_ACTIVITY, uint8_t ActivityDelay=DELAY_ACTIVITY_LED>
class LED : public myBaseModule {

  private:
  	static byte blockActivity;
    static uint8_t timeActivity;

  public:
    LED() { }

		const char* getFunctionCharacter() { return "la"; };

    void initialize() {
      	timeActivity = 0;
      	activityLed(1);
    }

    bool poll() {
      	activityLed(0);
        return 0;
    }

    void send(char *cmd, uint8_t typecode) {
    	switch(typecode) {
		    case MODULE_LED_LED:
      			if(cmd[0]=='0') {
      				blockActivity=0;
      				LedOnOff(0);
      			} else {
      				blockActivity=1;
      				LedOnOff(1);
      			}
      			break;

      		case MODULE_LED_ACTIVITY:
            LedOnOff(0);
            timeActivity=0;
            blockActivity = ( (cmd[0]=='0' || cmd[0]==0x0) ? 1 : 0);
      			break;
      }
    }

    void printHelp() {
      	DS_P("\n ## LED ##\n");
      	DS_P(" * [LED] l<0:off,1:on>\n");
      	if(Activity)
      	 DS_P(" * [Act] a<0:off,1:on>\n");
    }

    void LedOnOff (uint8_t on) {
    		  pinMode(LEDpin, OUTPUT);
          if(LEDinverse) on = !on;
  		  	digitalWrite(LEDpin, on);
    }

    void activityLed(uint8_t on) {

      if( (!timeActivity && !on) || !Activity || blockActivity) return;

     	// unsigned long msec = millis();
      if(!on) {
        // if( ActivityDelay <= ( (timeActivity > msec) ? (0xFFFFFFFF - timeActivity + msec) : (msec - timeActivity) ) ) {
				if( ActivityDelay <= millis_since(timeActivity)) {
          timeActivity=0;
    			LedOnOff(on);
        }
      } else {
        timeActivity=millis();
    		if(!timeActivity) timeActivity++; //for initialization
       	LedOnOff(on);
      }
    }
};

template <uint8_t LEDpin, bool LEDinverse, bool Activity, uint8_t ActivityDelay>
byte LED<LEDpin,LEDinverse,Activity,ActivityDelay>::blockActivity		= 0;
template <uint8_t LEDpin, bool LEDinverse, bool Activity, uint8_t ActivityDelay>
uint8_t LED<LEDpin,LEDinverse,Activity,ActivityDelay>::timeActivity	= 0;

#endif
