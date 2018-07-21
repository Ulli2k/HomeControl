#ifndef _MY_BME280_h
#define _MY_BME280_h

#include <myBaseModule.h>
#include <libI2C.h>
//#define HAS_BME280_PRESSURE

/*=========================================================================
    I2C ADDRESS/BITS
    -----------------------------------------------------------------------*/
	#define BME280_ADDRESS                (0x76)
    //SDO  to  GND  results  in  slave address 1110110 (0x76)
    //SDO to VDDIO results in slave address 1110111 (0x77)

/*=========================================================================*/

/*=========================================================================
    REGISTERS
    -----------------------------------------------------------------------*/
    enum
    {
      BME280_REGISTER_DIG_T1              = 0x88,
      BME280_REGISTER_DIG_T2              = 0x8A,
      BME280_REGISTER_DIG_T3              = 0x8C,

#ifdef HAS_BME280_PRESSURE
      BME280_REGISTER_DIG_P1              = 0x8E,
      BME280_REGISTER_DIG_P2              = 0x90,
      BME280_REGISTER_DIG_P3              = 0x92,
      BME280_REGISTER_DIG_P4              = 0x94,
      BME280_REGISTER_DIG_P5              = 0x96,
      BME280_REGISTER_DIG_P6              = 0x98,
      BME280_REGISTER_DIG_P7              = 0x9A,
      BME280_REGISTER_DIG_P8              = 0x9C,
      BME280_REGISTER_DIG_P9              = 0x9E,
#endif

      BME280_REGISTER_DIG_H1              = 0xA1,
      BME280_REGISTER_DIG_H2              = 0xE1,
      BME280_REGISTER_DIG_H3              = 0xE3,
      BME280_REGISTER_DIG_H4              = 0xE4,
      BME280_REGISTER_DIG_H5              = 0xE6,
      BME280_REGISTER_DIG_H6              = 0xE7,

      BME280_REGISTER_CHIPID             = 0xD0,
      BME280_REGISTER_VERSION            = 0xD1,
      BME280_REGISTER_SOFTRESET          = 0xE0,

      BME280_REGISTER_CAL26              = 0xE1,  // R calibration stored in 0xE1-0xF0

      BME280_REGISTER_CONTROLHUMID       						= 0xF2,
			BME280_REGISTER_CONTROLHUMID_SKIPPED					=	0x0,
			BME280_REGISTER_CONTROLHUMID_OVERSAMPLING_1		=	0x1,
			BME280_REGISTER_CONTROLHUMID_OVERSAMPLING_2		=	0x2,
			BME280_REGISTER_CONTROLHUMID_OVERSAMPLING_4		=	0x3,
			BME280_REGISTER_CONTROLHUMID_OVERSAMPLING_8		=	0x4,
			BME280_REGISTER_CONTROLHUMID_OVERSAMPLING_16	=	0x5,

      BME280_REGISTER_STATUS						 						= 0xF3,
      BME280_REGISTER_STATUS_MEASURING							= 0x8,

      BME280_REGISTER_CONTROL            						= 0xF4,
      BME280_REGISTER_CONTROL_P_SKIPPED							= 0x0,
      BME280_REGISTER_CONTROL_P_OVERSAMPLING_1			= (0x1 << 2),
      BME280_REGISTER_CONTROL_P_OVERSAMPLING_2			= (0x2 << 2),
      BME280_REGISTER_CONTROL_P_OVERSAMPLING_4			= (0x3 << 2),
      BME280_REGISTER_CONTROL_P_OVERSAMPLING_8			= (0x4 << 2),
      BME280_REGISTER_CONTROL_P_OVERSAMPLING_16			= (0x5 << 2),

      BME280_REGISTER_CONTROL_T_SKIPPED							= 0x0,
      BME280_REGISTER_CONTROL_T_OVERSAMPLING_1			= (0x1 << 5),
      BME280_REGISTER_CONTROL_T_OVERSAMPLING_2			= (0x2 << 5),
      BME280_REGISTER_CONTROL_T_OVERSAMPLING_4			= (0x3 << 5),
      BME280_REGISTER_CONTROL_T_OVERSAMPLING_8			= (0x4 << 5),
      BME280_REGISTER_CONTROL_T_OVERSAMPLING_16			= (0x5 << 5),
      BME280_REGISTER_CONTROL_SLEEP_MODE						= 0x0,
      BME280_REGISTER_CONTROL_FORCED_MODE						= 0x1,
      BME280_REGISTER_CONTROL_NORMAL_MODE						= 0x3,

      BME280_REGISTER_CONFIG             						= 0xF5,
      BME280_REGISTER_CONFIG_STANDBY_05							= 0x0,
      BME280_REGISTER_CONFIG_STANDBY_62							= (0x1 << 5),
      BME280_REGISTER_CONFIG_STANDBY_125						= (0x2 << 5),
      BME280_REGISTER_CONFIG_STANDBY_250						= (0x3 << 5),
      BME280_REGISTER_CONFIG_STANDBY_500						= (0x4 << 5),
      BME280_REGISTER_CONFIG_STANDBY_1000						= (0x5 << 5),
      BME280_REGISTER_CONFIG_STANDBY_10							= (0x6 << 5),
      BME280_REGISTER_CONFIG_STANDBY_20							= (0x7 << 5),
			BME280_REGISTER_CONFIG_FILTER_OFF							= 0x0,
			BME280_REGISTER_CONFIG_FILTER_2								= (0x1 << 2),
			BME280_REGISTER_CONFIG_FILTER_4								= (0x2 << 2),
			BME280_REGISTER_CONFIG_FILTER_8								= (0x3 << 2),
			BME280_REGISTER_CONFIG_FILTER_16							= (0x4 << 2),



      BME280_REGISTER_PRESSUREDATA       = 0xF7,
      BME280_REGISTER_TEMPDATA           = 0xFA,
      BME280_REGISTER_HUMIDDATA          = 0xFD,
    };

/*=========================================================================*/

/*=========================================================================
    CALIBRATION DATA
    -----------------------------------------------------------------------*/
    typedef struct
    {
      uint16_t dig_T1;
      int16_t  dig_T2;
      int16_t  dig_T3;
#ifdef HAS_BME280_PRESSURE
      uint16_t dig_P1;
      int16_t  dig_P2;
      int16_t  dig_P3;
      int16_t  dig_P4;
      int16_t  dig_P5;
      int16_t  dig_P6;
      int16_t  dig_P7;
      int16_t  dig_P8;
      int16_t  dig_P9;
#endif
      uint8_t  dig_H1;
      int16_t  dig_H2;
      uint8_t  dig_H3;
      int16_t  dig_H4;
      int16_t  dig_H5;
      int8_t   dig_H6;
    } bme280_calib_data;

template<class libI2CType>
class myBME280 : public myBaseModule {

private:
	libI2CType i2c;

public:
	const char* getFunctionCharacter() { return "E"; };

	void initialize() {
		// i2c
		PowerOpti_TWI_ON;

	  i2c.begin();
	  _forceModeActive = true;

	  //Reset BME280
		i2c.write8(BME280_REGISTER_SOFTRESET, 0xB6);
		delay(100);

	  if (i2c.read8(BME280_REGISTER_CHIPID) != 0x60) {
	  	DS_P("BME280 not available.\n");
	    return;
		}

	  readCoefficients();

	  //sleep mode
		i2c.write8(BME280_REGISTER_CONTROL, BME280_REGISTER_CONTROL_SLEEP_MODE);
	}
	void displayData(RecvData *DataBuffer) {
		float data[3];
		memcpy(data,DataBuffer->Data,sizeof(float)*3);
		DU(data[0]*100,4);
		DC('t');
		DU(data[1]*100,4);
		DC('h');
	#ifdef HAS_BME280_PRESSURE
		DU(data[2],4);
		DC('p');
	#endif

	}
	void printHelp() {
		DS_P("\n ## BME280 ##\n");
		DS_P(" * [Mode] E<n:normal,f:force>\n");
		DS_P(" * [Read] E<(degC)t(%H)h(bar)p>\n");
	}
	// bool poll();
	void infoPoll(byte prescaler) {
		#ifdef INFO_POLL_PRESCALER_BME
			if(!(prescaler % INFO_POLL_PRESCALER_BME))
		#endif
			{
				char buf[] = "f";
				if(_forceModeActive) {
					buf[0] = 'f';
					send(buf,MODULE_BME280_ENVIRONMENT);
				}
				buf[0] = 'r';
				send(buf,MODULE_BME280_ENVIRONMENT); //just print the current available results
			}
	}
	void send(char *cmd, uint8_t typecode) {

			switch(typecode) {

				case MODULE_BME280_ENVIRONMENT:
					switch(cmd[0]) {
						case 'f':
							forceBMEmeasureHTP();
						break;
						case 'n':
							normalBMEmeasureHTP();
						break;
						default:
							readBMEreadHTP();
						break;
					}
				break;
			}
	}

	float readTemperature(void) {
		int32_t var1, var2;

	  int32_t adc_T = i2c.read16(BME280_REGISTER_TEMPDATA);
	  adc_T <<= 8;
	  adc_T |= i2c.read8(BME280_REGISTER_TEMPDATA+2);
	  adc_T >>= 4;

	  var1  = ((((adc_T>>3) - ((int32_t)bme280_calib.dig_T1 <<1))) *
	  				((int32_t)bme280_calib.dig_T2)) >> 11;
	  var2  = (((((adc_T>>4) - ((int32_t)bme280_calib.dig_T1)) *
	  				((adc_T>>4) - ((int32_t)bme280_calib.dig_T1))) >> 12) *
	  				((int32_t)bme280_calib.dig_T3)) >> 14;

	  t_fine = var1 + var2;

	  float T  = (t_fine * 5 + 128) >> 8;
	  return T/100;
	}
#ifdef HAS_BME280_PRESSURE
	float readPressure(void) {
		int64_t var1, var2, p;

	  int32_t adc_P = i2c.read16(BME280_REGISTER_PRESSUREDATA);
	  adc_P <<= 8;
	  adc_P |= i2c.read8(BME280_REGISTER_PRESSUREDATA+2);
	  adc_P >>= 4;

	  var1 = ((int64_t)t_fine) - 128000;
	  var2 = var1 * var1 * (int64_t)bme280_calib.dig_P6;
	  var2 = var2 + ((var1*(int64_t)bme280_calib.dig_P5)<<17);
	  var2 = var2 + (((int64_t)bme280_calib.dig_P4)<<35);
	  var1 = ((var1 * var1 * (int64_t)bme280_calib.dig_P3)>>8) +
	    ((var1 * (int64_t)bme280_calib.dig_P2)<<12);
	  var1 = (((((int64_t)1)<<47)+var1))*((int64_t)bme280_calib.dig_P1)>>33;

	  if (var1 == 0) {
	    return 0;  // avoid exception caused by division by zero
	  }
	  p = 1048576 - adc_P;
	  p = (((p<<31) - var2)*3125) / var1;
	  var1 = (((int64_t)bme280_calib.dig_P9) * (p>>13) * (p>>13)) >> 25;
	  var2 = (((int64_t)bme280_calib.dig_P8) * p) >> 19;

	  p = ((p + var1 + var2) >> 8) + (((int64_t)bme280_calib.dig_P7)<<4);
	  //return (float)p/256;
	  return (float)(p>>8);
	}
#endif
	float readHumidity(void) {

		  int32_t adc_H = i2c.read16(BME280_REGISTER_HUMIDDATA);

		  int32_t v_x1_u32r;

		  v_x1_u32r = (t_fine - ((int32_t)76800));

		  v_x1_u32r = (((((adc_H << 14) - (((int32_t)bme280_calib.dig_H4) << 20) -
				  (((int32_t)bme280_calib.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
			       (((((((v_x1_u32r * ((int32_t)bme280_calib.dig_H6)) >> 10) *
				    (((v_x1_u32r * ((int32_t)bme280_calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
				  ((int32_t)2097152)) * ((int32_t)bme280_calib.dig_H2) + 8192) >> 14));

		  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
					     ((int32_t)bme280_calib.dig_H1)) >> 4));

		  v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
		  v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
		  //float h = (v_x1_u32r>>12);
		  //return  h / 1024.0;
		  return (float)((v_x1_u32r>>12) >> 10);
	}
	void readCoefficients(void) {
		bme280_calib.dig_T1 = ((uint16_t)i2c.read16_LE(BME280_REGISTER_DIG_T1));
    bme280_calib.dig_T2 = ((int16_t)i2c.readS16_LE(BME280_REGISTER_DIG_T2));
    bme280_calib.dig_T3 = ((int16_t)i2c.readS16_LE(BME280_REGISTER_DIG_T3));
#ifdef HAS_BME280_PRESSURE
    bme280_calib.dig_P1 = ((uint16_t)i2c.read16_LE(BME280_REGISTER_DIG_P1));
    bme280_calib.dig_P2 = ((int16_t)i2c.readS16_LE(BME280_REGISTER_DIG_P2));
    bme280_calib.dig_P3 = ((int16_t)i2c.readS16_LE(BME280_REGISTER_DIG_P3));
    bme280_calib.dig_P4 = ((int16_t)i2c.readS16_LE(BME280_REGISTER_DIG_P4));
    bme280_calib.dig_P5 = ((int16_t)i2c.readS16_LE(BME280_REGISTER_DIG_P5));
    bme280_calib.dig_P6 = ((int16_t)i2c.readS16_LE(BME280_REGISTER_DIG_P6));
    bme280_calib.dig_P7 = ((int16_t)i2c.readS16_LE(BME280_REGISTER_DIG_P7));
    bme280_calib.dig_P8 = ((int16_t)i2c.readS16_LE(BME280_REGISTER_DIG_P8));
    bme280_calib.dig_P9 = ((int16_t)i2c.readS16_LE(BME280_REGISTER_DIG_P9));
#endif
    bme280_calib.dig_H1 = ((uint8_t)i2c.read8(BME280_REGISTER_DIG_H1));
    bme280_calib.dig_H2 = ((int16_t)i2c.readS16_LE(BME280_REGISTER_DIG_H2));
    bme280_calib.dig_H3 = ((uint8_t)i2c.read8(BME280_REGISTER_DIG_H3));
    bme280_calib.dig_H4 = ((int16_t)((i2c.read8(BME280_REGISTER_DIG_H4) << 4) | (i2c.read8(BME280_REGISTER_DIG_H4+1) & 0x0F)));
    bme280_calib.dig_H5 = ((int16_t)((i2c.read8(BME280_REGISTER_DIG_H5) << 4) | ((i2c.read8(BME280_REGISTER_DIG_H5+1) >> 4) & 0x0F)));
    bme280_calib.dig_H6 = ((uint8_t)i2c.read8(BME280_REGISTER_DIG_H6));
	}
//	float readAltitude(void);

private:
	bme280_calib_data bme280_calib;
	uint8_t _I2Caddr;
  int32_t _sensorID;
  int32_t t_fine;
  uint8_t _forceModeActive;

	void readBMEreadHTP() {
		float data[3];

		PowerOpti_TWI_ON;

	  data[0] = readTemperature();
	  data[1] = readHumidity();
	#ifdef HAS_BME280_PRESSURE
	  data[2] = readPressure()/100; //in bar
	#endif

		PowerOpti_TWI_OFF;

		if(DEBUG) {
			DS_P("T:");DU(data[0],0);
			DC(' ');
			DS_P("H:");DU(data[1],0);
	#ifdef HAS_BME280_PRESSURE
			DC(' ');
			DS_P("P:");DU(data[2],0);
			DC(' ');
	#endif
			//DS_P("A:");DF(readAltitude(),0);
			DNL();
		}

	  addToRingBuffer(MODULE_BME280_ENVIRONMENT, 0,(const byte*)data,sizeof(data));
	}
	void forceBMEmeasureHTP() {
		PowerOpti_TWI_ON;

		_forceModeActive = true;
		readCoefficients();

	  //sleep mode: config will only be writeable in sleep mode, so first insure that.
		i2c.write8(BME280_REGISTER_CONTROL, BME280_REGISTER_CONTROL_SLEEP_MODE);

	  //Set ctrl_hum first, then ctrl_meas to activate ctrl_hum (all other bits can be ignored)
	  i2c.write8(BME280_REGISTER_CONTROLHUMID, BME280_REGISTER_CONTROLHUMID_OVERSAMPLING_1);

		//set temp,pressure, mode(NormaleMode=3) oversampling
		//OverSample: Skipped|x1|x2|x4|x8|x16
		//Mode: Sleep|Forced|Forced|Normal
	  i2c.write8(BME280_REGISTER_CONTROL, BME280_REGISTER_CONTROL_T_OVERSAMPLING_1 |
	  #ifdef HAS_BME280_PRESSURE
	  																		BME280_REGISTER_CONTROL_P_OVERSAMPLING_1 |
		#else
	  																		BME280_REGISTER_CONTROL_P_SKIPPED |
	 	#endif
	  																		BME280_REGISTER_CONTROL_FORCED_MODE	);

		while(i2c.read8(BME280_REGISTER_STATUS) & BME280_REGISTER_STATUS_MEASURING); //waiting for results

		PowerOpti_TWI_OFF;
	}
	void normalBMEmeasureHTP() {
		PowerOpti_TWI_ON;

		_forceModeActive = false;
	  readCoefficients();

	  //sleep mode: config will only be writeable in sleep mode, so first insure that.
		i2c.write8(BME280_REGISTER_CONTROL, BME280_REGISTER_CONTROL_SLEEP_MODE);

		//Set the config word
		//tStandby: 0.5|62,5|125|250|500|1000|10|20 [ms]
		//Filter: off|2|4|8|16 [Filter coeff]
	  i2c.write8(BME280_REGISTER_CONFIG, BME280_REGISTER_CONFIG_STANDBY_1000 | BME280_REGISTER_CONFIG_FILTER_OFF);

	  //Set ctrl_hum first, then ctrl_meas to activate ctrl_hum (all other bits can be ignored)
	  i2c.write8(BME280_REGISTER_CONTROLHUMID, BME280_REGISTER_CONTROLHUMID_OVERSAMPLING_1);


		//set temp,pressure, mode(NormaleMode=3) oversampling
		//OverSample: Skipped|x1|x2|x4|x8|x16
		//Mode: Sleep|Forced|Forced|Normal
	  i2c.write8(BME280_REGISTER_CONTROL, BME280_REGISTER_CONTROL_T_OVERSAMPLING_1 |
	  #ifdef HAS_BME280_PRESSURE
	  																		BME280_REGISTER_CONTROL_P_OVERSAMPLING_1 |
		#else
	  																		BME280_REGISTER_CONTROL_P_SKIPPED |
	 	#endif
	  																		BME280_REGISTER_CONTROL_NORMAL_MODE	);
		PowerOpti_TWI_OFF;
	}
};

#endif
