
## Monitor beenden mit Strg+A danach Strg+X

DEVICE_ID		= 99

#FORCE_MONITOR_PORT	=	1
#MONITOR_PORT				=	net:esp:23
#MONITOR_CMD					=	nc
#MONITOR_PARAMS			=	esp 23
	
MONITOR_PORT				= /dev/ttyUSB0

#MONITOR_PORT				= net:192.168.188.40:23
### Libraries
	ARDUINO_LIBS			+= mySPI myRFM69
	#ARDUINO_LIBS			+= TimerOne myIRMP
#	ARDUINO_LIBS			+= Wire myBME280
	#ARDUINO_LIBS			+= myPowerMonitor
#	ARDUINO_LIBS			+= myRollo

## AVR ##
	CPPFLAGS				 	+= -DINCLUDE_DEBUG_OUTPUT
	CPPFLAGS					+= -DQUIETMODE_DEFAULT_VALUE=false
#	CPPFLAGS				 	+= -DSTORE_CONFIGURATION	#angelöst durch DEFAULT Werte im Makefile
	CPPFLAGS         	+= -DHAS_INFO_POLL -DINFO_POLL_CYCLE_TIME=8 #[s]  #[s] >=8 <=2040 (8s - 34min)
#	CPPFLAGS         	+= -DHAS_POWER_OPTIMIZATIONS
#	CPPFLAGS					+= -DAVR_LOWPOWER_DEFAULT_VALUE=false	#set AVR to sleep after startup
#	CPPFLAGS				 	+= -DINFO_POLL_PRESCALER_VCC=1
	CPPFLAGS				 	+= -DREADVCC_CALIBRATION_CONST=1085982L
	
	CPPFLAGS				 	+= -DHAS_LEDs -DLED_ACTIVITY	

## RADIO ##
	CPPFLAGS					+= -DHAS_SPI -DHAS_RFM69
	CPPFLAGS					+= -DHAS_RFM69_CMD_TUNNELING=2	#1: Host | 2: Satellite
	CPPFLAGS					+= -DRFM69_CMD_TUNNELING_DEFAULT_VALUE=true -DRFM69_CMD_TUNNELING_HOSTID_DEFAULT_VALUE=1
#	CPPFLAGS					+= -DHAS_RFM69_POWER_ADJUSTABLE -DRFM69_POWER_DEFAULT_VALUE=32	#set output power: 0=min, 31=max (for RFM69W or RFM69CW), 0-31 or 32->51 for RFM69HW
	
#	CPPFLAGS					+= -DHAS_RFM69_TXonly #Tunneling needed
#	CPPFLAGS					+= -DRFM69_TXonly_DEFAULT_VALUE=false
	
#	CPPFLAGS					+= -DHAS_RFM69_SNIFF
	CPPFLAGS					+= -DRFM69_NO_OTHER_PROTOCOLS -DRFM69_NO_BROADCASTS
#	CPPFLAGS					+= -DHAS_RFM69_LISTENMODE #-DHAS_RFM69_POWER_ADJUSTABLE -DRFM69_LISTENMODE_DEFAULT_VALUE=true
	#CPPFLAGS					+= -DHAS_RFM69_ADC_NOISE_REDUCTION	

## ADC ##
#	CPPFLAGS				 +=	-DHAS_ADC=6 -DINFO_POLL_PRESCALER_ADC=1 	#please define READVCC_CALIBRATION_CONST

## ROLLO ##
#	CPPFLAGS				 += -DHAS_ROLLO -DROLLO_UP_DOWN_PIN=8 -DROLLO_POWER_PIN=7

## TRIGGER (Interrupts) ##
#	CPPFLAGS				 += -DHAS_TRIGGER=3 #trigger 3:button(click,doubleclick,longclick) 2:pulse 1:High 0:Low
#	CPPFLAGS				 += -DTRIGGER_EVENT=MODULE_ROLLO_EVENT #Module Message like default: MODULE_AVR_TRIGGER or e.g. MODULE_ROLLO_EVENT
#	CPPFLAGS				 += -DTRIGGER_PIN=6 #INTZero or INTOne or digitalPin
	#CPPFLAGS				 += -DTRIGGER_DEBOUNCE_TIME=2000 #[ms] !keeps AVR awake during debouncing time! (Just for pulse,high,low)

#	CPPFLAGS				 += -DHAS_TRIGGER_1=2 #trigger 3:pulse&hold 2:pulse 1:High 0:Low
#	#CPPFLAGS				 += -DTRIGGER_1_EVENT=MODULE_ROLLO_EVENT #Module Message like MODULE_ROLLO_EVENT
#	CPPFLAGS				 += -DTRIGGER_1_PIN=3 #INTZero or INTOne or digitalPin
#	CPPFLAGS				 += -DTRIGGER_1_DEBOUNCE_TIME=100 #[ms] !keeps AVR awake during debouncing time!

## BUZZER ##
	#CPPFLAGS				 += -DHAS_BUZZER -DBUZZER_PIN=5 -DBUZZER_FREQUENCE=2.730 -DBUZZER_ON_TIME=100 -DBUZZER_OFF_TIME=100
	#-DBUZZER_ON_TIME=91 -DBUZZER_OFF_TIME=275
	
## IR - Infrared ##	
	#CPPFLAGS				 += -DHAS_IR_RX 
	#-DHAS_IR_TX
	
## SWITCHES ##
	#CPPFLAGS				 += -DHAS_SWITCH -DSWITCH1_S_PIN=7 -DSWITCH1_R_PIN=8

## SENSORS ##
#	CPPFLAGS				 += -DHAS_BME280
#	CPPFLAGS				 +=	-DINFO_POLL_PRESCALER_BME=1

## POWER MONITOR ##
	#Burden Resistor 150 Ohm
	#CPPFLAGS				 += -DHAS_POWER_MONITOR_CT=2 -DPOWER_MONITOR_CT_ADJUST=0.92 #do not forget to set READVCC_CALIBRATION_CONST 
	#CPPFLAGS				 += -DHAS_POWER_MONITOR_VCC_2 -DHAS_POWER_MONITOR_CT2_1 -DHAS_POWER_MONITOR_CT1_1 #HAS_POWER_MONITOR_CT3_1

	
	
