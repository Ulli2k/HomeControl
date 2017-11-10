
DEVICE_ID		= 8

	FORCE_MONITOR_PORT	=	1
	MONITOR_PORT				=	net:esp:23
	MONITOR_CMD					=	nc
	MONITOR_PARAMS			=	esp 23
	FORCE_FLASH_PORT		=	1
	
### Libraries
	ARDUINO_LIBS			+= mySPI myRFM69
	#ARDUINO_LIBS			+= TimerOne myIRMP
	ARDUINO_LIBS			+= Wire myBME280
	#ARDUINO_LIBS			+= myPowerMonitor

### CPPFLAGS
### Flags you might want to set for debugging purpose. Comment to stop.
#	CPPFLAGS				 	+= -DSTORE_CONFIGURATION
	CPPFLAGS         	+= -DHAS_INFO_POLL -DINFO_POLL_CYCLE_TIME=300 #[s] >=8 (300:5min)
	CPPFLAGS         	+= -DHAS_POWER_OPTIMIZATIONS
	CPPFLAGS				 	+= -DREADVCC_CALIBRATION_CONST=1152457L
	
	CPPFLAGS					+= -DHAS_SPI -DHAS_RFM69
	CPPFLAGS					+= -DHAS_RFM69_CMD_TUNNELING=2
#	CPPFLAGS					+= -DHAS_RFM69_TXonly
#	CPPFLAGS					+= -DHAS_RFM69_SNIFF
#	CPPFLAGS					+= -DRFM69_NO_OTHER_PROTOCOLS -DRFM69_NO_BROADCASTS
#	CPPFLAGS					+= -DHAS_RFM69_LISTENMODE #-DHAS_RFM69_POWER_ADJUSTABLE 
	#CPPFLAGS					+= -DHAS_RFM69_ADC_NOISE_REDUCTION
	
	CPPFLAGS				 += -DHAS_LEDs -DLED_ACTIVITY
	CPPFLAGS				 +=	-DHAS_ADC -DINFO_POLL_PRESCALER_ADC=1
#	CPPFLAGS				 += -DINFO_POLL_PRESCALER_VCC=3
	CPPFLAGS				 +=	-DINFO_POLL_PRESCALER_BME=1
#	CPPFLAGS				 += -DHAS_TRIGGER=1 -DTRIGGER_DEBOUNCE_TIME=2000 -DTRIGGER_PIN=INTOne
	#CPPFLAGS				 += -DHAS_BUZZER -DBUZZER_PIN=5 -DBUZZER_FREQUENCE=2.730 -DBUZZER_ON_TIME=100 -DBUZZER_OFF_TIME=100
	#-DBUZZER_ON_TIME=91 -DBUZZER_OFF_TIME=275
	#CPPFLAGS				 += -DHAS_IR_RX 
	#-DHAS_IR_TX
	#CPPFLAGS				 += -DHAS_SWITCH -DSWITCH1_S_PIN=7 -DSWITCH1_R_PIN=8
	CPPFLAGS				 += -DHAS_BME280

	#CPPFLAGS				 += -DHAS_POWER_MONITOR=2 -DHAS_POWER_MONITOR_VCC_1 -DHAS_POWER_MONITOR_CT2_1 -DHAS_POWER_MONITOR_CT1_1
	
	#CPPFLAGS				 += -DHAS_POWER_MONITOR -DREADVCC_CALIBRATION_CONST=1118361L -DHAS_POWER_MONITOR_CT1_1
	#CPPFLAGS				 += -DHAS_POWER_MONITOR -DHAS_POWER_MONITOR_PULSE