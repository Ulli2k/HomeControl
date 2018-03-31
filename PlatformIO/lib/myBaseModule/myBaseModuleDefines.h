

#define MODULE_DATAPROCESSING    									0x00
	#define MODULE_DATAPROCESSING_QUIET	 								0x10
	#define MODULE_DATAPROCESSING_HELP									0x20
	#define MODULE_DATAPROCESSING_FIRMWARE							0x30
	#define MODULE_DATAPROCESSING_WAKE_SIGNAL						0x40
	#define MODULE_DATAPROCESSING_DEVICEID							0xE0	//internal use, no official command
	#define MODULE_DATAPROCESSING_OUTPUTTUNNEL					0xF0	//internal use, no official command
#define MODULE_AVR							 									0x01
	#define MODULE_AVR_RAMAVAILABLE			 								0x11
	#define MODULE_AVR_POWERDOWN												0x21
	#define MODULE_AVR_REBOOT														0x31
	#define MODULE_AVR_LOWPOWER													0x41
	#define MODULE_AVR_ADC															0x51
	#define MODULE_AVR_ATMEGA_VCC												0x61
	#define MODULE_AVR_LED															0x71
	#define MODULE_AVR_ACTIVITYLED											0x81
	#define MODULE_AVR_BUZZER														0x91
	#define MODULE_AVR_SWITCH														0xA1
	#define MODULE_AVR_TRIGGER													0xB1
#define MODULE_SERIAL           	 								0x02
#define MODULE_IRMP             									0x03
#define MODULE_RFM12            	 								0x04
	#define MODULE_RFM12_OOK        								 		0x14
	#define MODULE_RFM12_FSK         								 		0x24
	#define MODULE_RFM12_TUNE        								 		0x34
	#define MODULE_RFM12_RAW         								 		0x44
	#define MODULE_RFM12_MODEQUERY   								 		0x54
#define MODULE_SPI																0x05
#define MODULE_RFM69															0x06
	#define MODULE_RFM69_OPTIONS												0x16
	#define MODULE_RFM69_MODEQUERY											0x26
	#define MODULE_RFM69_TUNNELING											0x36
	#define MODULE_RFM69_SENDACK												0xF6 //internal use, no official command
	#define MODULE_RFM69_OPTION_SEND										0xE6 //internal use, no official command
	#define MODULE_RFM69_OPTION_POWER										0xD6 //internal use, no official command
	#define MODULE_RFM69_OPTION_TEMP										0xC6 //internal use, no official command
#define MODULE_BME280															0x07
	#define MODULE_BME280_ENVIRONMENT										0x17
#define MODULE_POWERMONITOR												0x08
	#define MODULE_POWERMONITOR_POWER										0x18
#define MODULE_ROLLO															0x09
	#define MODULE_ROLLO_CMD														0x19
	#define MODULE_ROLLO_EVENT													0x29

#define MODULE_TYP(m) 						(uint8_t)(m & 0x0F)
#define MODULE_PROTOCOL(m)				(m > 0x0F ? ( (m >> 4) -1) : 0)
#define MODULE_COMMAND_CHAR(m,c)  ( { char b='-'; const typeModuleInfo* pmt = ModuleTab; while(pmt->typecode >= 0) { if(pmt->typecode==(m)) { b=(((uint8_t)strlen(pmt->name) >= (uint8_t)((c)>>4)) ? (pmt->name[((c)>>4)-1]) : '-'); break; } pmt++; } b; } )
