

	DEVICE_ID		= 1


	FORCE_MONITOR_PORT	=	1
	MONITOR_PORT				=	net:beagle:4000
	MONITOR_CMD					=	nc
	MONITOR_PARAMS			=	beagle 4001
	
### Libraries
	ARDUINO_LIBS			+= mySPI myRFM69
	ARDUINO_LIBS			+= TimerOne myIRMP
	#ARDUINO_LIBS			+= Wire myBME280

### CPPFLAGS
### Flags you might want to set for debugging purpose. Comment to stop.
	CPPFLAGS         += -DHAS_INFO_POLL -DINFO_POLL_CYCLE_TIME=10
	CPPFLAGS				 += -DHAS_SPI -DHAS_RFM69=3
	CPPFLAGS				 +=	-DHAS_ADC
	CPPFLAGS				 += -DHAS_BUZZER -DBUZZER_PIN=5 -DBUZZER_FREQUENCE=2.731 -DBUZZER_ON_TIME=91 -DBUZZER_OFF_TIME=275
	CPPFLAGS				 += -DHAS_IR_RX 
											#-DHAS_IR_TX
	
