
#ifndef MY_SPI_h
#define MY_SPI_h

#include <stdint.h>
#include <myBaseModule.h>

#define MAX_SPI_SLAVES			4	//for saving states of chip select pins //MAX: 15 durch  myRFM69::toggleRadioMode begrenzt.
#define SPI_CONFIG_ID_INVALID		0x00
#define SPI_CONFIG_ID_1    			0x01
#define SPI_CONFIG_ID_2    			0x02
#define SPI_CONFIG_ID_3    			0x03
#define SPI_CONFIG_ID_4    			0x04

#if F_CPU<8000000L
	#error SPI speed!
#else
	#define OPTIMIZE_SPI 1  // uncomment this to write to the RFM12B @ 8 Mhz
#endif

#define SPI_MOSI    11    // PB3, pin 17
#define SPI_MISO    12    // PB4, pin 18
#define SPI_SCK     13    // PB5, pin 19

#define SPI_CLOCK_DIV4 		0x00
#define SPI_CLOCK_DIV16 	0x01
#define SPI_CLOCK_DIV64 	0x02
#define SPI_CLOCK_DIV128 	0x03
#define SPI_CLOCK_DIV2 		0x04
#define SPI_CLOCK_DIV8 		0x05
#define SPI_CLOCK_DIV32 	0x06
//#define SPI_CLOCK_DIV64 0x07

#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C

#define SPI_MODE_MASK 		0x0C  // CPOL = bit 3, CPHA = bit 2 on SPCR
#define SPI_CLOCK_MASK 		0x03  // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 	0x01  // SPI2X = bit 0 on SPSR

typedef struct {
    uint8_t id:3;						// Value-Range: 0-4 = spi_id value range!
#if MAX_SPI_SLAVES>0x04
	#error id memory size too small
#endif
    uint8_t cs:5;					//Value-Range: 0-21 (max. A7 in Atmega328p) 
    uint8_t irq:5;				//Value-Range: 0-21 (max. A7 in Atmega328p) (SPI & myAVR_interrupt)
    byte InterruptState:3; // Value-Range: 0-7
} typeSPItable;
extern const typeSPItable SPITab[];

class mySPI : public myBaseModule {

private:
	uint8_t cs_buffer[MAX_SPI_SLAVES];
	
	void store_deactivate_Slaves();
	void restore_Slaves();

	void setBitOrder(uint8_t bitOrder);
	void setDataMode(uint8_t mode);
	void setClockDivider(uint8_t rate);
	
public:
	void xferBegin(uint8_t spi_index);
	void xferEnd(uint8_t spi_index);
	inline uint8_t xfer_byte(uint8_t out);	 //low level - needs to use xferBegin and xferEnd
	inline uint8_t xfer(uint8_t spi_index, uint8_t cmd);
	inline uint8_t xferSlow(uint8_t spi_id, uint8_t cmd);	
	
	inline uint16_t xfer16(uint8_t spi_index, uint16_t cmd);
	inline uint16_t xferSlow16(uint8_t spi_index, uint16_t cmd);
	
	void initialize();
	void cleanUp(uint8_t spi_index);
	
	uint8_t getIntPin(uint8_t spi_pin);
	byte getIntState(uint8_t spi_index) ;
	char getIdent(uint8_t spi_index);
	uint8_t getClassID(uint8_t spi_id);

};

/* 16 Bit SPI transfer */
uint16_t mySPI::xferSlow16(uint8_t spi_index, uint16_t cmd) {
	
	xferBegin(spi_index);
	// slow down to under 2.5 MHz
  bitSet(SPCR, SPR0);

  //uint16_t reply = xfer16(cmd);
  uint16_t reply = xfer_byte(cmd >> 8) << 8;
  reply |= xfer_byte(cmd);    
    
  xferEnd(spi_index);
  bitClear(SPCR, SPR0); //speed back up spi
  
  return reply;
}

uint16_t mySPI::xfer16(uint8_t spi_index, uint16_t cmd) {

#if (not defined(OPTIMIZE_SPI)) || OPTIMIZE_SPI==0
	return xferSlow16(spi_index, cmd);
#endif

	xferBegin(spi_index);	
  uint16_t reply = xfer_byte(cmd >> 8) << 8;
  reply |= xfer_byte(cmd);    
  xferEnd(spi_index);
  
  return reply;
}

/* 8 Bit SPI transfer */
uint8_t mySPI::xferSlow(uint8_t spi_index, uint8_t cmd) {
	
	xferBegin(spi_index);
	// slow down to under 2.5 MHz
  bitSet(SPCR, SPR0);

  uint8_t reply = xfer_byte(cmd);
    
  xferEnd(spi_index);
  bitClear(SPCR, SPR0); //speed back up spi
  
  return reply;
}

uint8_t mySPI::xfer(uint8_t spi_index, uint8_t cmd) {
#if (not defined(OPTIMIZE_SPI)) || OPTIMIZE_SPI==0
	return xferSlow(spi_index, cmd);
#endif

	xferBegin(spi_index);	
  uint8_t reply = xfer_byte(cmd);
  xferEnd(spi_index);
  
  return reply;
}

uint8_t mySPI::xfer_byte(uint8_t out) {

    SPDR = out;
    // this loop spins 4 usec with a 2 MHz SPI clock
    while (!(SPSR & _BV(SPIF)));
    return SPDR;
}

#endif

