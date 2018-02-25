
#include <myBME280.h>

myBME280::myBME280(uint8_t I2C_Address) {
	_I2Caddr = I2C_Address;
}

void myBME280::initialize() {

	// i2c
	PowerOpti_TWI_ON;

  Wire.begin();
  _forceModeActive = true;

  //Reset BME280
	I2C_write8(BME280_REGISTER_SOFTRESET, 0xB6);
	TIMING::delay(100);

  if (I2C_read8(BME280_REGISTER_CHIPID) != 0x60) {
  	DS_P("BME280 not available.\n");
    return;
	}

  readCoefficients();

  //sleep mode
	I2C_write8(BME280_REGISTER_CONTROL, BME280_REGISTER_CONTROL_SLEEP_MODE);
}

void myBME280::forceBMEmeasureHTP() {

	PowerOpti_TWI_ON;

	_forceModeActive = true;
	readCoefficients();

  //sleep mode: config will only be writeable in sleep mode, so first insure that.
	I2C_write8(BME280_REGISTER_CONTROL, BME280_REGISTER_CONTROL_SLEEP_MODE);

  //Set ctrl_hum first, then ctrl_meas to activate ctrl_hum (all other bits can be ignored)
  I2C_write8(BME280_REGISTER_CONTROLHUMID, BME280_REGISTER_CONTROLHUMID_OVERSAMPLING_1);

	//set temp,pressure, mode(NormaleMode=3) oversampling
	//OverSample: Skipped|x1|x2|x4|x8|x16
	//Mode: Sleep|Forced|Forced|Normal
  I2C_write8(BME280_REGISTER_CONTROL, BME280_REGISTER_CONTROL_T_OVERSAMPLING_1 |
  #ifdef HAS_BME280_PRESSURE
  																		BME280_REGISTER_CONTROL_P_OVERSAMPLING_1 |
	#else
  																		BME280_REGISTER_CONTROL_P_SKIPPED |
 	#endif
  																		BME280_REGISTER_CONTROL_FORCED_MODE	);

	while(I2C_read8(BME280_REGISTER_STATUS) & BME280_REGISTER_STATUS_MEASURING); //waiting for results

	PowerOpti_TWI_OFF;
}

void myBME280::normalBMEmeasureHTP() {

	PowerOpti_TWI_ON;

	_forceModeActive = false;
  readCoefficients();

  //sleep mode: config will only be writeable in sleep mode, so first insure that.
	I2C_write8(BME280_REGISTER_CONTROL, BME280_REGISTER_CONTROL_SLEEP_MODE);

	//Set the config word
	//tStandby: 0.5|62,5|125|250|500|1000|10|20 [ms]
	//Filter: off|2|4|8|16 [Filter coeff]
  I2C_write8(BME280_REGISTER_CONFIG, BME280_REGISTER_CONFIG_STANDBY_1000 | BME280_REGISTER_CONFIG_FILTER_OFF);

  //Set ctrl_hum first, then ctrl_meas to activate ctrl_hum (all other bits can be ignored)
  I2C_write8(BME280_REGISTER_CONTROLHUMID, BME280_REGISTER_CONTROLHUMID_OVERSAMPLING_1);


	//set temp,pressure, mode(NormaleMode=3) oversampling
	//OverSample: Skipped|x1|x2|x4|x8|x16
	//Mode: Sleep|Forced|Forced|Normal
  I2C_write8(BME280_REGISTER_CONTROL, BME280_REGISTER_CONTROL_T_OVERSAMPLING_1 |
  #ifdef HAS_BME280_PRESSURE
  																		BME280_REGISTER_CONTROL_P_OVERSAMPLING_1 |
	#else
  																		BME280_REGISTER_CONTROL_P_SKIPPED |
 	#endif
  																		BME280_REGISTER_CONTROL_NORMAL_MODE	);
	PowerOpti_TWI_OFF;
}

void myBME280::readBMEreadHTP() {

	float data[3];

	PowerOpti_TWI_ON;

  data[0] = readTemperature();
  data[1] = readHumidity();
#ifdef HAS_BME280_PRESSURE
  data[2] = readPressure()/100; //in bar
#endif

	PowerOpti_TWI_OFF;

	if(DEBUG) {
		DS_P("T:");Serial.print(data[0]);
		DC(' ');
		DS_P("H:");Serial.print(data[1]);
#ifdef HAS_BME280_PRESSURE
		DC(' ');
		DS_P("P:");Serial.print(data[2]);
		DC(' ');
#endif
		//DS_P("A:");Serial.print(readAltitude());
		DNL();
	}

  addToRingBuffer(MODULE_BME280_ENVIRONMENT, 0,(const byte*)data,sizeof(data));
}

void myBME280::infoPoll(byte prescaler) {
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

bool myBME280::poll() {

//	TIMING::delay(2000);
//	readBMEDataHTP();
	return 0;
}

void myBME280::send(char *cmd, uint8_t typecode) {

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

void myBME280::displayData(RecvData *DataBuffer) {

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

void myBME280::printHelp() {

	DS_P("\n ## BME280 ##\n");
	DS_P(" * [Mode] E<n:normal,f:force>\n");
	DS_P(" * [Read] E<(degC)t(%H)h(bar)p>\n");
}

/**************************************************************************/
float myBME280::readTemperature(void) {
  int32_t var1, var2;

  int32_t adc_T = I2C_read16(BME280_REGISTER_TEMPDATA);
  adc_T <<= 8;
  adc_T |= I2C_read8(BME280_REGISTER_TEMPDATA+2);
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

/**************************************************************************/
/*!

*/
/**************************************************************************/
#ifdef HAS_BME280_PRESSURE
float myBME280::readPressure(void) {
  int64_t var1, var2, p;

  int32_t adc_P = I2C_read16(BME280_REGISTER_PRESSUREDATA);
  adc_P <<= 8;
  adc_P |= I2C_read8(BME280_REGISTER_PRESSUREDATA+2);
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
/**************************************************************************/
/*!

*/
/**************************************************************************/

//float myBME280::readAltitude( void ) {
//	float heightOutput = 0;
//	heightOutput = ((float)-45846.2)*(pow(((float)readPressure()/(float)101325), 0.190263) - (float)1);
//	return heightOutput;
//}

/**************************************************************************/
/*!

*/
/**************************************************************************/
float myBME280::readHumidity(void) {

  int32_t adc_H = I2C_read16(BME280_REGISTER_HUMIDDATA);

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

void myBME280::readCoefficients(void)
{
    bme280_calib.dig_T1 = ((uint16_t)I2C_read16_LE(BME280_REGISTER_DIG_T1));
    bme280_calib.dig_T2 = ((int16_t)I2C_readS16_LE(BME280_REGISTER_DIG_T2));
    bme280_calib.dig_T3 = ((int16_t)I2C_readS16_LE(BME280_REGISTER_DIG_T3));
#ifdef HAS_BME280_PRESSURE
    bme280_calib.dig_P1 = ((uint16_t)I2C_read16_LE(BME280_REGISTER_DIG_P1));
    bme280_calib.dig_P2 = ((int16_t)I2C_readS16_LE(BME280_REGISTER_DIG_P2));
    bme280_calib.dig_P3 = ((int16_t)I2C_readS16_LE(BME280_REGISTER_DIG_P3));
    bme280_calib.dig_P4 = ((int16_t)I2C_readS16_LE(BME280_REGISTER_DIG_P4));
    bme280_calib.dig_P5 = ((int16_t)I2C_readS16_LE(BME280_REGISTER_DIG_P5));
    bme280_calib.dig_P6 = ((int16_t)I2C_readS16_LE(BME280_REGISTER_DIG_P6));
    bme280_calib.dig_P7 = ((int16_t)I2C_readS16_LE(BME280_REGISTER_DIG_P7));
    bme280_calib.dig_P8 = ((int16_t)I2C_readS16_LE(BME280_REGISTER_DIG_P8));
    bme280_calib.dig_P9 = ((int16_t)I2C_readS16_LE(BME280_REGISTER_DIG_P9));
#endif
    bme280_calib.dig_H1 = ((uint8_t)I2C_read8(BME280_REGISTER_DIG_H1));
    bme280_calib.dig_H2 = ((int16_t)I2C_readS16_LE(BME280_REGISTER_DIG_H2));
    bme280_calib.dig_H3 = ((uint8_t)I2C_read8(BME280_REGISTER_DIG_H3));
    bme280_calib.dig_H4 = ((int16_t)((I2C_read8(BME280_REGISTER_DIG_H4) << 4) | (I2C_read8(BME280_REGISTER_DIG_H4+1) & 0x0F)));
    bme280_calib.dig_H5 = ((int16_t)((I2C_read8(BME280_REGISTER_DIG_H5) << 4) | ((I2C_read8(BME280_REGISTER_DIG_H5+1) >> 4) & 0x0F)));
    bme280_calib.dig_H6 = ((uint8_t)I2C_read8(BME280_REGISTER_DIG_H6));
}

/************************* I2C-API ***********************/
void myBME280::I2C_write8(uint8_t reg, uint8_t value) {

	Wire.beginTransmission(_I2Caddr);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t myBME280::I2C_read8(uint8_t reg) {

	uint8_t value;
	Wire.beginTransmission((uint8_t)_I2Caddr);
  Wire.write(reg);
  Wire.endTransmission();

  Wire.requestFrom(_I2Caddr, (byte)1);
  value = Wire.read();
  Wire.endTransmission();

  return value;
}

uint16_t myBME280::I2C_read16(byte reg) {

	uint16_t value;
  Wire.beginTransmission(_I2Caddr);
  Wire.write(reg);
  Wire.endTransmission();

  Wire.requestFrom(_I2Caddr, (byte)2);
  value = (Wire.read() << 8) | Wire.read();
  Wire.endTransmission();

  return value;
}

uint16_t myBME280::I2C_read16_LE(byte reg) {
  uint16_t temp = I2C_read16(reg);
  return (temp >> 8) | (temp << 8);
}

int16_t myBME280::I2C_readS16(byte reg) {
  return (int16_t)I2C_read16(reg);
}

int16_t myBME280::I2C_readS16_LE(byte reg) {
  return (int16_t)I2C_read16_LE(reg);
}
