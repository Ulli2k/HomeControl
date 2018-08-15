
#ifndef _MY_LIB_I2C_h
#define _MY_LIB_I2C_h

#include <Wire.h> //I2C Driver

/******** DEFINE dependencies ******
  DEFAULT_I2C_ADDRESS: default i2c address which will be used
************************************/

#define DEFAULT_I2C_ADDRESS     0x76

template<uint8_t I2C_Address=DEFAULT_I2C_ADDRESS>
class libI2C {

public:
  static void begin() {
    Wire.begin();
  }

  static void write8(uint8_t reg, uint8_t value) {

  	Wire.beginTransmission(I2C_Address);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
  }

  static uint8_t read8(uint8_t reg) {

  	uint8_t value;
  	Wire.beginTransmission((uint8_t)I2C_Address);
    Wire.write(reg);
    Wire.endTransmission();

    Wire.requestFrom(I2C_Address, (uint8_t)1);
    value = Wire.read();
    Wire.endTransmission();

    return value;
  }

  static uint16_t read16(uint8_t reg) {

  	uint16_t value;
    Wire.beginTransmission(I2C_Address);
    Wire.write(reg);
    Wire.endTransmission();

    Wire.requestFrom(I2C_Address, (uint8_t)2);
    value = (Wire.read() << 8) | Wire.read();
    Wire.endTransmission();

    return value;
  }

  static uint16_t read16_LE(uint8_t reg) {
    uint16_t temp = read16(reg);
    return (temp >> 8) | (temp << 8);
  }

  static int16_t readS16(uint8_t reg) {
    return (int16_t)read16(reg);
  }

  static int16_t readS16_LE(uint8_t reg) {
    return (int16_t)read16_LE(reg);
  }

};

#endif
