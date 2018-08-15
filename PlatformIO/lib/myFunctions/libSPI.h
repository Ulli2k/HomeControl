
//TODO: PowerOpti_SPI_ON ergänzen

#ifndef _MY_LIB_SPI_h
#define _MY_LIB_SPI_h

#include <SPI.h>

/******** DEFINE dependencies ******

************************************/

#ifdef ARDUINO_ARCH_AVR
  typedef uint8_t BitOrder;
#endif

template <uint8_t CS, uint32_t CLOCK=2000000, BitOrder BITORDER=MSBFIRST, uint8_t MODE=SPI_MODE0>
class LibSPI {

public:
  LibSPI () {}
  void initialize () {
    pinMode(CS,OUTPUT);
    SPI.begin();
  }

  void select () {
    noInterrupts();
    digitalWrite(CS,LOW);
  }

  void deselect () {
    digitalWrite(CS,HIGH);
    interrupts();
  }

  void ping () {
    SPI.beginTransaction(SPISettings(CLOCK,BITORDER,MODE));
    select();                                     // wake up the communication module
    SPI.transfer(0); // ????
    deselect();
    SPI.endTransaction();
  }

  void waitMiso () {
    delayMicroseconds(10);
  }

  uint8_t send (uint8_t data) {
    SPI.beginTransaction(SPISettings(CLOCK,BITORDER,MODE));
    uint8_t ret = SPI.transfer(data);
    SPI.endTransaction();
    return ret;
  }

  uint8_t strobe(uint8_t cmd) {
    SPI.beginTransaction(SPISettings(CLOCK,BITORDER,MODE));
    select();                                     // select
    uint8_t ret = SPI.transfer(cmd);
    deselect();                                   // deselect
    SPI.endTransaction();
    return ret;
  }

  void readBurst(uint8_t regAddr, uint8_t * buf, uint8_t len, bool doOpen=true, bool doClose=true, bool doWriteAddr=true, uint8_t regType = 0x7F) {
  //void readBurst(uint8_t * buf, uint8_t regAddr, uint8_t len, bool doOpen=true, bool doWriteAddr=true, uint8_t regType = 0x7F) {
    if(doOpen) {
      SPI.beginTransaction(SPISettings(CLOCK,BITORDER,MODE));
      select();                        // select
    }
    if(doWriteAddr) SPI.transfer(regAddr & regType);                         // send register address
    for(uint8_t i=0 ; i<len ; i++) {
      buf[i] = SPI.transfer(0x00);                            // read result byte by byte
      //dbg << i << ":" << buf[i] << '\n';
    }
    if(doClose) {
      deselect();                                   // deselect
      SPI.endTransaction();
    }
  }

  void writeBurst(uint8_t regAddr, uint8_t* buf, uint8_t len) {
    SPI.beginTransaction(SPISettings(CLOCK,BITORDER,MODE));
    select();                                     // select
    SPI.transfer(regAddr | 0x80);                          // send register address
    for(uint8_t i=0 ; i<len ; i++)
      SPI.transfer(buf[i]);                  // send value
    deselect();                                   // deselect
    SPI.endTransaction();
  }

  void writeBurstKeepOpen(uint8_t regAddr, uint8_t* buf, uint8_t len) {
    SPI.beginTransaction(SPISettings(CLOCK,BITORDER,MODE));
    select();                                     // select
    SPI.transfer(regAddr | 0x80);                          // send register address
    for(uint8_t i=0 ; i<len ; i++)
      SPI.transfer(buf[i]);                  // send value
  }

  void writeBurstWithoutOpen(uint8_t* buf, uint8_t len) {
    for(uint8_t i=0 ; i<len ; i++)
      SPI.transfer(buf[i]);                  // send value
    deselect();                                   // deselect
    SPI.endTransaction();
  }

  uint8_t readReg(uint8_t regAddr, bool doOpen=true, bool doClose=true, bool doWriteAddr=true, bool doReadBack=true, uint8_t regType = 0x7F) {
    uint8_t val=0;
    if(doOpen) {
      SPI.beginTransaction(SPISettings(CLOCK,BITORDER,MODE));
      select();                                     // select
    }
    if(doWriteAddr) SPI.transfer(regAddr & regType);                            // send register address
    if(doReadBack) val = SPI.transfer(0x00);                           // read result
    if(doClose) {
      deselect();                                   // deselect
      SPI.endTransaction();
    }
    return val;
  }

  void writeReg(uint8_t regAddr, uint8_t val) {
    SPI.beginTransaction(SPISettings(CLOCK,BITORDER,MODE));
    select();                                     // select
    SPI.transfer(regAddr | 0x80);                                // send register address
    SPI.transfer(val);                                  // send value
    deselect();                                   // deselect
    SPI.endTransaction();
  }

  // uint8_t readRegKeepOpen(uint8_t regAddr, uint8_t regType = 0x7F, bool noReturn=false) {
  //   SPI.beginTransaction(SPISettings(CLOCK,BITORDER,MODE));
  //   select();                                     // select
  //   SPI.transfer(regAddr & regType);                            // send register address
  //   uint8_t val=0;
  //   if(!noReturn) SPI.transfer(0x00);
  //   return val;
  // }


  void readBurstWithoutOpen(uint8_t * buf, uint8_t len) {
    for(uint8_t i=0 ; i<len ; i++) {
      buf[i] = SPI.transfer(0x00);                            // read result byte by byte
      //dbg << i << ":" << buf[i] << '\n';
    }
    deselect();                                   // deselect
    SPI.endTransaction();
  }


};

#endif
