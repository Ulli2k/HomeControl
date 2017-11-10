
#include <mySPI.h>

void mySPI::initialize() {

	//initialize SPI and all CS/SS/IRQ ports
	const typeSPItable* tab = SPITab;
  while(tab->id != SPI_CONFIG_ID_INVALID) {

	// Chip unselect
    digitalWrite(tab->cs, HIGH);
    pinMode(tab->cs, OUTPUT);

	//IRQ  --> wird von attachinterrupt and detachinterrupt behandelt
    //pinMode(tab->irq_pin, INPUT);
    //digitalWrite(tab->irq_pin, HIGH); // pull-up <<---führt zu problemen im der RFM69
		
    tab++;
  }

	//SPI
	pinMode(SPI_MOSI, OUTPUT);
  pinMode(SPI_MISO, INPUT);
  pinMode(SPI_SCK, OUTPUT);
   
  SPCR = _BV(SPE) | _BV(MSTR);
  // use clk/2 (2x 1/4th) for sending (and clk/8 for recv, see rf12_xferSlow)
  SPSR |= _BV(SPI2X);

	//RFM69 specific
/*
  setDataMode(SPI_MODE0);
  setBitOrder(MSBFIRST);
  setClockDivider(SPI_CLOCK_DIV4); //decided to slow down from DIV2 after SPI stalling in some instances, especially visible on mega1284p when RFM69 and FLASH chip both present
*/
}

void mySPI::setBitOrder(uint8_t bitOrder) {
  if(bitOrder == LSBFIRST) {
    SPCR |= _BV(DORD);
  } else {
    SPCR &= ~(_BV(DORD));
  }
}

void mySPI::setDataMode(uint8_t mode) { 
  SPCR = (SPCR & ~SPI_MODE_MASK) | mode;
}

void mySPI::setClockDivider(uint8_t rate) {
  SPCR = (SPCR & ~SPI_CLOCK_MASK) | (rate & SPI_CLOCK_MASK);
  SPSR = (SPSR & ~SPI_2XCLOCK_MASK) | ((rate >> 2) & SPI_2XCLOCK_MASK);
}

void mySPI::cleanUp(uint8_t spi_index) {

	xferBegin(spi_index);	
	while (digitalRead(SPITab[spi_index].irq) == 0)
		xfer_byte(0x00);
	xferEnd(spi_index);	
}


void mySPI::xferBegin(uint8_t spi_index) {
 	
 	noInterrupts(); 
 	
 	//PowerOptimization(BOD_,ADC_,AIN_,TIMER2_,TIMER1_,TIMER0_,SPI_ON,USART0_,TWI_,WDT_);
 	PowerOpti_SPI_ON;
	store_deactivate_Slaves();
	digitalWrite(SPITab[spi_index].cs, LOW); // Chip select
	
	#if (not defined(OPTIMIZE_SPI)) || OPTIMIZE_SPI==0
  bitSet(SPCR, SPR0); // slow down to under 2.5 MHz
	#endif
}

void mySPI::xferEnd(uint8_t spi_index) {

  restore_Slaves();  
  digitalWrite(SPITab[spi_index].cs, HIGH); // Chip unselect  
  
  #if (not defined(OPTIMIZE_SPI)) || OPTIMIZE_SPI==0
  bitClear(SPCR, SPR0); //speed back up spi
  #endif
  
	//PowerOptimization(BOD_,ADC_,AIN_,TIMER2_,TIMER1_,TIMER0_,SPI_OFF,USART0_,TWI_,WDT_);
	PowerOpti_TIMER0_OFF;
	
	interrupts();
}

void mySPI::store_deactivate_Slaves() {

	const typeSPItable* tab = SPITab;
	uint8_t index=0;
	
	//store current SS status and deactivate all slaves
  while(tab->id != SPI_CONFIG_ID_INVALID) {
  	cs_buffer[index] = digitalRead(tab->cs);//bitRead(*tab->cs_port, tab->cs_pin);
  	digitalWrite(tab->cs, HIGH); // Chip unselect
  
  	//cfgInterrupt(NULL, getIntType(index), Block ,getIntPin(index)); // deactivate SPI interrupt
  		
  	tab++;
  	index++;
  } 
}

void mySPI::restore_Slaves() {

	const typeSPItable* tab = SPITab;
	uint8_t index=0;
		
	//reset CS status
  while(tab->id != SPI_CONFIG_ID_INVALID) {
  	//cfgInterrupt(NULL, getIntType(index), Release ,getIntPin(index)); // activate SPI interrupt
		
  	if(cs_buffer[index]) {
	  	digitalWrite(tab->cs, HIGH); // Chip unselect
	  } else {
	  	digitalWrite(tab->cs, LOW); // Chip select
	  }
  	tab++;
  	index++;
  }	
}


/************************************************************************************/
char mySPI::getIdent(uint8_t spi_index) {
	if(spi_index >= MAX_SPI_SLAVES) {
		//DS_P("getIdent: Could not find spi_id.\n");
		return '0'+(MAX_SPI_SLAVES-1);
	}
	return '0'+ SPITab[spi_index].id;
}

// transform spi_id into real struct index for each class
uint8_t mySPI::getClassID(uint8_t spi_id) {
	const typeSPItable* tab = SPITab;

	for(uint8_t i=0; (tab[i].id!=SPI_CONFIG_ID_INVALID && i<=MAX_SPI_SLAVES); i++) {
		if( tab[i].id == spi_id) {
			return i; //spi_index
		}
	} 
	//DS_P("getClassID: Could not find spi_id.\n");
	return 0;
}

/******************************** Interrupt Routines ********************************/
uint8_t mySPI::getIntPin(uint8_t spi_index) {
	if(spi_index >= MAX_SPI_SLAVES) {
		//DS_P("getIntType: Could not find spi_id.\n");
		return SPITab[(MAX_SPI_SLAVES-1)].irq;
	}
	return SPITab[spi_index].irq;
}

byte mySPI::getIntState(uint8_t spi_index) {
	return SPITab[spi_index].InterruptState;
}


