

	DEVICE_ID		= 5


### MONITOR_BAUDRATE
### It must be set to Serial baudrate value you are using.

	MONITOR_BAUDRATE  	= 57600
	FORCE_MONITOR_PORT	=	1
	MONITOR_PORT				=	net:192.168.178.1:2222
	MONITOR_CMD					=	nc
	MONITOR_PARAMS			=	192.168.178.1 2222
	
### Libraries
	ARDUINO_LIBS			+= mySPI myRFM69
	#ARDUINO_LIBS			+= TimerOne myIRMP
	#ARDUINO_LIBS			+= Wire myBME280

### CPPFLAGS
### Flags you might want to set for debugging purpose. Comment to stop.
	CPPFLAGS				 += -DHAS_SPI -DHAS_RFM69

