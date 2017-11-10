

	DEVICE_ID		= 3


### MONITOR_BAUDRATE
### It must be set to Serial baudrate value you are using.

	MONITOR_BAUDRATE  	=	57600
	MONITOR_PORT				=	/dev/ttyUSB1
	MONITOR_CMD					=	minicom
	
### Libraries
	ARDUINO_LIBS			+= mySPI myRFM69
	ARDUINO_LIBS			+= TimerOne myIRMP
	
### CPPFLAGS
### Flags you might want to set for debugging purpose. Comment to stop.
	CPPFLAGS				 += -DSTORE_CONFIGURATION
	
	CPPFLAGS				 += -DHAS_SPI -DHAS_RFM69
	CPPFLAGS				 += -DHAS_RFM69_CMD_TUNNELING=2	
	CPPFLAGS				 +=	-DRFM69_NO_OTHER_PROTOCOLS -DRFM69_NO_BROADCASTS
	CPPFLAGS				 += -DHAS_IR_TX
	CPPFLAGS				 += -DHAS_SWITCH -DSWITCH1_S_PIN=7 -DSWITCH1_R_PIN=5
