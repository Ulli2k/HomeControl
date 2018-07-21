
#ifndef MYBME280_h
#define MYBME280_h

#include "myBaseModule.h"
#include <Wire.h> //I2C Driver

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

class myBME280 : public myBaseModule {

public:
  myBME280(uint8_t I2C_Address=BME280_ADDRESS);
	//~myBME280() {};
	const char* getFunctionCharacter() { return "E"; };

	void initialize();
	void displayData(RecvData *DataBuffer);
	void printHelp();
	bool poll();
	void infoPoll(byte prescaler);
	void send(char *cmd, uint8_t typecode);

	float readTemperature(void);
#ifdef HAS_BME280_PRESSURE
	float readPressure(void);
#endif
	float readHumidity(void);
	void readCoefficients(void);
//	float readAltitude(void);

private:
	bme280_calib_data bme280_calib;
	uint8_t _I2Caddr;
  int32_t _sensorID;
  int32_t t_fine;
  uint8_t _forceModeActive;

	void I2C_write8(uint8_t reg, uint8_t value);
	uint8_t I2C_read8(uint8_t reg);
	uint16_t I2C_read16(byte reg);
	uint16_t I2C_read16_LE(byte reg);
	int16_t I2C_readS16(byte reg);
	int16_t I2C_readS16_LE(byte reg);

	void readBMEreadHTP();
	void forceBMEmeasureHTP();
	void normalBMEmeasureHTP();
};

#endif
