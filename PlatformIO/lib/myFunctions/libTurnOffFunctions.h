
//TODO: PowerOpti_AllPins_OFF soll alle Pins einschließen!

#if not defined(HAS_POWER_OPTIMIZATIONS)
	#define PowerOpti_TurnAllOff
	#define PowerOpti_AllPins_OFF
	#define PowerOpti_BOD_OFF
	#define PowerOpti_AIN_OFF
	#define PowerOpti_AIN_ON
	#define PowerOpti_ADC_OFF
	#define PowerOpti_ADC_ON
	#define PowerOpti_ADC0_OFF
	#define PowerOpti_ADC0_ON
	#define PowerOpti_ADC2_OFF
	#define PowerOpti_ADC2_ON
	#define PowerOpti_TIMER0_OFF
	#define PowerOpti_TIMER0_ON
	#define PowerOpti_TIMER1_OFF
	#define PowerOpti_TIMER1_ON
	#define PowerOpti_TIMER2_OFF
	#define PowerOpti_TIMER2_ON
	#define PowerOpti_SPI_OFF
	#define PowerOpti_SPI_ON
	#define PowerOpti_USART0_OFF
	#define PowerOpti_USART0_ON
	#define PowerOpti_USB_OFF
	#define PowerOpti_USB_ON
	#define PowerOpti_TWI_OFF
	#define PowerOpti_TWI_ON
	#define PowerOpti_WDT_OFF
	#define PowerOpti_WDT_ON

#else

	enum period_t
	{
		SLEEP_15MS,
		SLEEP_30MS,
		SLEEP_60MS,
		SLEEP_120MS,
		SLEEP_250MS,
		SLEEP_500MS,
		SLEEP_1S,
		SLEEP_2S,
		SLEEP_4S,
		SLEEP_8S,
		SLEEP_FOREVER
	};

	#if defined (__SAMD21G18A__)

	#define PowerOpti_TurnAllOff
	#define PowerOpti_AllPins_OFF 	{ for(uint8_t i=0; i<NUM_DIGITAL_PINS;i++) {FKT_PIN_OFF(i)} /*Digital Pins*/ }
	#define PowerOpti_BOD_OFF 			{ SYSCTRL->BOD33.reg = 0; }
	#define PowerOpti_AIN_OFF
	#define PowerOpti_AIN_ON
	#define PowerOpti_ADC_OFF				FKT_ADC_OFF
	#define PowerOpti_ADC_ON
	#define PowerOpti_ADC0_OFF
	#define PowerOpti_ADC0_ON
	#define PowerOpti_ADC2_OFF
	#define PowerOpti_ADC2_ON
	#define PowerOpti_TIMER_OFF			FKT_TIMER_OFF
	#define PowerOpti_TIMER0_OFF
	#define PowerOpti_TIMER0_ON
	#define PowerOpti_TIMER1_OFF
	#define PowerOpti_TIMER1_ON
	#define PowerOpti_TIMER2_OFF
	#define PowerOpti_TIMER2_ON
	#define PowerOpti_SPI_OFF
	#define PowerOpti_SPI_ON
	#define PowerOpti_USART0_OFF		FKT_USART0_OFF
	#define PowerOpti_USART0_ON			FKT_USART0_ON
	#define PowerOpti_USB_OFF				FKT_USB_OFF
	#define PowerOpti_USB_ON				FKT_USB_ON
	#define PowerOpti_TWI_OFF
	#define PowerOpti_TWI_ON
	#define PowerOpti_WDT_OFF				FKT_WDT_OFF
	#define PowerOpti_WDT_ON
	#define PowerOpti_DAC_OFF				FKT_DAC_OFF
	#define PowerOpti_DSU_OFF				FKT_DSU_OFF
	#define PowerOpti_DMA_OFF				FKT_DMA_OFF
	#define PowerOpti_RTC_OFF				FKT_RTC_OFF
	#define PowerOpti_EIC_OFF				FKT_EIC_OFF


	#define FKT_PIN_OFF(x) 					{pinMode((x), INPUT_PULLUP);}
	#define FKT_USB_OFF							{ USBDevice.detach(); USB->HOST.CTRLA.bit.ENABLE=0; USB->DEVICE.CTRLA.bit.ENABLE=0; \
																		PM->AHBMASK.reg &= ~PM_AHBMASK_USB; PM->APBBMASK.reg &= ~PM_APBBMASK_USB; } // usb.CTRLB.bit.DETACH = 1;
	#define FKT_USB_ON							{ USBDevice.init();USBDevice.attach(); } //usb.CTRLB.bit.DETACH = 0;
	#define FKT_USART0_ON						{ PM->APBCMASK.reg |= PM_APBCMASK_SERCOM0; }
	#define FKT_USART0_OFF					{ PM->APBCMASK.reg &= ~PM_APBCMASK_SERCOM0; }
	#define FKT_WDT_OFF							{ WDT->CTRL.reg = 0; PM->APBAMASK.reg &= ~PM_APBAMASK_WDT; }
	#define FKT_ADC_OFF							{ PM->APBCMASK.reg &= ~PM_APBCMASK_ADC; }
	#define FKT_SERCOM_OFF					{ PM->APBCMASK.reg &= ~(/*UART PM_APBCMASK_SERCOM0  | */ PM_APBCMASK_SERCOM1 | PM_APBCMASK_SERCOM2 | PM_APBCMASK_SERCOM3 | PM_APBCMASK_SERCOM4 | PM_APBCMASK_SERCOM5); }
	#define FKT_TIMER_OFF						{ PM->APBCMASK.reg &= ~(PM_APBCMASK_TCC0 | PM_APBCMASK_TCC1 | PM_APBCMASK_TCC2 | PM_APBCMASK_TC3 | PM_APBCMASK_TC4 | PM_APBCMASK_TC5); }
	#define FKT_DAC_OFF							{ PM->APBCMASK.reg &= ~PM_APBCMASK_DAC; }
	#define FKT_DSU_OFF							{ PM->APBBMASK.reg &= ~PM_APBBMASK_DSU; PM->AHBMASK.reg &= ~PM_AHBMASK_DSU;} //needed for Debugger
	#define FKT_DMA_OFF							{ PM->APBBMASK.reg &= ~PM_APBBMASK_DMAC; PM->AHBMASK.reg &= ~PM_AHBMASK_DMAC; }
	#define FKT_RTC_OFF							{ PM->APBAMASK.reg &= ~PM_APBAMASK_RTC; }
	#define FKT_EIC_OFF							{ PM->APBAMASK.reg &= ~PM_APBAMASK_EIC; }

/*
	#define FKT_WDT_OFF							{ WDT->CTRL.reg = 0; while(WDT->STATUS.bit.SYNCBUSY); }
	#define FKT_RTC_OFF							{ RTC->MODE0.CTRL.bit.ENABLE=0; while(RTC->MODE0.STATUS.bit.SYNCBUSY); \
		 																RTC->MODE1.CTRL.bit.ENABLE=0; while(RTC->MODE1.STATUS.bit.SYNCBUSY); \
																		RTC->MODE2.CTRL.bit.ENABLE=0; while(RTC->MODE2.STATUS.bit.SYNCBUSY); \
																		PM->APBAMASK.reg &= ~PM_APBAMASK_RTC; \
																	}
	#define FKT_TIMER_OFF						{ TCC0->CTRLA.bit.ENABLE=0; TCC1->CTRLA.bit.ENABLE=0; TCC2->CTRLA.bit.ENABLE=0; }
	#define FKT_AC_OFF							{ AC->CTRLA.bit.ENABLE=0; }
	#define FKT_DAC_OFF							{ DAC->CTRLA.bit.ENABLE=0; }
	#define FKT_ADC_OFF							{ ADC->CTRLA.bit.ENABLE=0; }
	#define FKT_I2C_OFF							{ I2S->CTRLA.bit.ENABLE=0; }
	#define FKT_UART0_OFF						{ SERCOM0->USART.CTRLA.bit.ENABLE=0; PM->APBCMASK.reg &= ~PM_APBCMASK_SERCOM0; }
	#define FKT_SERCOM_OFF					{ SERCOM0->USART.CTRLA.bit.ENABLE=0; \
																		SERCOM1->USART.CTRLA.bit.ENABLE=0; \
																		SERCOM2->USART.CTRLA.bit.ENABLE=0; \
																		SERCOM3->USART.CTRLA.bit.ENABLE=0; \
																		SERCOM4->USART.CTRLA.bit.ENABLE=0; \
																		SERCOM5->USART.CTRLA.bit.ENABLE=0; \
																		PM->APBCMASK.reg &= ~(PM_APBCMASK_SERCOM0 | PM_APBCMASK_SERCOM1 | PM_APBCMASK_SERCOM2 | PM_APBCMASK_SERCOM3 | PM_APBCMASK_SERCOM4 | PM_APBCMASK_SERCOM5); \
																	}
*/
/* from http://www.rocketscream.com/blog/docs-item/using-rtc-in-low-power-mode-on-mini-ultra-pro/#tab-id-2
// Switch unused pins as input and enabled built-in pullup
 for (pinNumber = 0; pinNumber < 23; pinNumber++)
 {
 pinMode(pinNumber, INPUT_PULLUP);
 }

 for (pinNumber = 32; pinNumber < 42; pinNumber++)
 {
 pinMode(pinNumber, INPUT_PULLUP);
 }

 pinMode(25, INPUT_PULLUP);
 pinMode(26, INPUT_PULLUP);
 */
#warning turn off clocks in PM not needed in idle mode?
//https://github.com/nebs/arduino-zero-timer-demo/blob/master/.pioenvs/zero/FrameworkDeviceInc/samd21/include/component/pm.h
//or Turnoff Clocks
	// PM->APBBMASK.reg &= ~PM_APBBMASK_PORT;
	// PM->APBBMASK.reg &= ~PM_APBBMASK_DMAC;
	// PM->APBCMASK.reg &= ~PM_APBCMASK_SERCOM0;
	// PM->APBCMASK.reg &= ~PM_APBCMASK_SERCOM1;
	//
	#define FKT_WDT_WAKEUP_EVENT 						(true) //check ob WDT oder Interrupt
	#define FKT_WDT_POWERDOWN(periode) 			{ SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; __DSB(); __WFI(); } //periode will be set due RTC
	#define	FKT_POWERDOWN_FOREVER						FKT_WDT_POWERDOWN(0)

	enum idle_t {	IDLE_0, IDLE_1,	IDLE_2 };
	// void	idle(idle_t idleMode) { }
	// void	standby() { }

	#elif defined(__AVR_ATmega328P__)

	#include <avr/power.h>
	#include <avr/wdt.h>
	#include <avr/sleep.h>

	/*******************************************************************************
	*
	* Argument  	Description
	* =========  	===========
	*
	* 2. adc		ADC module disable control:
	*				(a) ADC_OFF - Turn off ADC module
	*				(b) ADC_ON - Leave ADC module in its default state
	*
	* 3. timer2		Timer 2 module disable control:
	*				(a) TIMER2_OFF - Turn off Timer 2 module
	*				(b) TIMER2_ON - Leave Timer 2 module in its default state
	*
	* 4. timer1		Timer 1 module disable control:
	*				(a) TIMER1_OFF - Turn off Timer 1 module
	*				(b) TIMER1_ON - Leave Timer 1 module in its default state
	*
	* 5. timer0		Timer 0 module disable control:
	*				(a) TIMER0_OFF - Turn off Timer 0 module
	*				(b) TIMER0_ON - Leave Timer 0 module in its default state
	*
	* 6. spi		SPI module disable control:
	*				(a) SPI_OFF - Turn off SPI module
	*				(b) SPI_ON - Leave SPI module in its default state
	*
	* 7. usart0		USART0 module disable control:
	*				(a) USART0_OFF - Turn off USART0  module
	*				(b) USART0_ON - Leave USART0 module in its default state
	*
	* 8. twi		TWI module disable control:
	*				(a) TWI_OFF - Turn off TWI module
	*				(b) TWI_ON - Leave TWI module in its default state
	*
	*******************************************************************************/

		#define PowerOpti_TurnAllOff 		FKT_TurnAllOff
		#define PowerOpti_AllPins_OFF
		#define PowerOpti_BOD_OFF				FKT_BOD_OFF
		#define PowerOpti_AIN_OFF				FKT_AIN_OFF
		#define PowerOpti_AIN_ON				FKT_AIN_ON
		#define PowerOpti_ADC_OFF				FKT_ADC_OFF
		#define PowerOpti_ADC_ON				FKT_ADC_ON
		#define PowerOpti_ADC0_OFF			FKT_ADC0_OFF
		#define PowerOpti_ADC0_ON				FKT_ADC0_ON
		#define PowerOpti_ADC2_OFF			FKT_ADC2_OFF
		#define PowerOpti_ADC2_ON				FKT_ADC2_ON
		#define PowerOpti_TIMER0_OFF		FKT_TIMER0_OFF
		#define PowerOpti_TIMER0_ON			FKT_TIMER0_ON
		#define PowerOpti_TIMER1_OFF		FKT_TIMER1_OFF
		#define PowerOpti_TIMER1_ON			FKT_TIMER1_ON
		#define PowerOpti_TIMER2_OFF		FKT_TIMER2_OFF
		#define PowerOpti_TIMER2_ON			FKT_TIMER2_ON
		#define PowerOpti_SPI_OFF				FKT_SPI_OFF
		#define PowerOpti_SPI_ON				FKT_SPI_ON
		#define PowerOpti_USART0_OFF		FKT_USART0_OFF
		#define PowerOpti_USART0_ON			FKT_USART0_ON
		#define PowerOpti_USB_OFF
		#define PowerOpti_USB_ON
		#define PowerOpti_TWI_OFF				FKT_TWI_OFF
		#define PowerOpti_TWI_ON				FKT_TWI_ON
		#define PowerOpti_WDT_OFF				FKT_WDT_OFF
		#define PowerOpti_WDT_ON				FKT_WDT_ON
		#define PowerOpti_TIMER_OFF
		#define PowerOpti_DAC_OFF
		#define PowerOpti_DSU_OFF
		#define PowerOpti_DMA_OFF
		#define PowerOpti_EIC_OFF

		#define FKT_USB_OFF
		#define FKT_USB_ON
		#define FKT_SERCOM_OFF

	#define FKT_TurnAllOff			{\
															FKT_BOD_OFF;\
															FKT_AIN_OFF;\
															FKT_ADC_OFF;\
															FKT_TIMER0_OFF;\
															FKT_TIMER1_OFF;\
															FKT_TIMER2_OFF;\
															FKT_SPI_OFF;\
															FKT_USART0_OFF;\
															FKT_TWI_OFF;\
															FKT_WDT_OFF;\
															}
	#define FKT_BOD_OFF {\
				/* turn off brown-out enable in software*/\
				MCUCR = bit (BODS) | bit (BODSE);\
				MCUCR = bit (BODS);\
				}

	#define FKT_AIN_OFF			{	DIDR1 |= (1 << AIN1D) | (1 << AIN0D);	}
	#define FKT_AIN_ON			{	DIDR1 &= ~((1 << AIN1D) | (1 << AIN0D));	}

	#define FKT_ADC_OFF			{\
				ADCSRA &= ~(1 << ADEN);\
				DIDR0 |= (1 << ADC5D) | (1 << ADC4D) | (1 << ADC3D) | (1 << ADC2D) | (1 << ADC1D) | (1 << ADC0D);\
				power_adc_disable();\
				}
	#define FKT_ADC_ON			{\
				power_adc_enable();\
				DIDR0 &= ~((1 << ADC5D) | (1 << ADC4D) | (1 << ADC3D) | (1 << ADC2D) | (1 << ADC1D) | (1 << ADC0D));\
				ADCSRA |= (1 << ADEN);\
				}

	#define FKT_ADC0_OFF			{\
				DIDR0 |= (1 << ADC0D);\
				if((DIDR0 & 0x3F) == 0x3F) {\
					ADCSRA &= ~(1 << ADEN);\
					power_adc_disable();\
				}\
				}
	#define FKT_ADC0_ON			{\
				power_adc_enable();\
				DIDR0 &= ~(1 << ADC0D);\
				ADCSRA |= (1 << ADEN);\
				}
	#define FKT_ADC2_OFF			{\
				DIDR0 |= (1 << ADC2D);\
				if((DIDR0 & 0x3F) == 0x3F) {\
					ADCSRA &= ~(1 << ADEN);\
					power_adc_disable();\
				}\
				}
	#define FKT_ADC2_ON			{\
				power_adc_enable();\
				DIDR0 &= ~(1 << ADC2D);\
				ADCSRA |= (1 << ADEN);\
				}

	#define FKT_TIMER0_OFF	{	power_timer0_disable();	}
	#define FKT_TIMER0_ON 	{	power_timer0_enable();	}

	#define FKT_TIMER1_OFF	{	power_timer1_disable();	}
	#define FKT_TIMER1_ON 	{	power_timer1_enable();	}

	#define FKT_TIMER2_OFF	{\
				if (TCCR2B & CS22) clockSource |= (1 << CS22);\
				if (TCCR2B & CS21) clockSource |= (1 << CS21);\
				if (TCCR2B & CS20) clockSource |= (1 << CS20);\
				/* Remove the clock source to shutdown Timer2 */\
				TCCR2B &= ~(1 << CS22);\
				TCCR2B &= ~(1 << CS21);\
				TCCR2B &= ~(1 << CS20);\
				power_timer2_disable();\
				}
	#define FKT_TIMER2_ON		{\
				if (clockSource & CS22) TCCR2B |= (1 << CS22);\
				if (clockSource & CS21) TCCR2B |= (1 << CS21);\
				if (clockSource & CS20) TCCR2B |= (1 << CS20);\
				power_timer2_enable();\
				}

	#define FKT_SPI_OFF {\
				SPCR &= ~(1<<SPE);\
				power_spi_disable();\
				}
	#define FKT_SPI_ON {\
				power_spi_enable();\
				SPCR |= (1<<SPE);\
				}

	#define FKT_USART0_OFF	{\
				UCSR0B &= ~((1<<RXEN0) | (1<<TXEN0));\
			 	power_usart0_disable();\
			 	}
	#define FKT_USART0_ON {\
				power_usart0_enable();\
				UCSR0B |= (1<<RXEN0) | (1<<TXEN0);\
				}

	#define FKT_TWI_OFF {\
				TWCR &= ~(1<<TWEN);\
				power_twi_disable();\
				}
	#define FKT_TWI_ON {\
				power_twi_enable();\
				TWCR |= (1<<TWEN);\
				}

	#define FKT_WDT_OFF	{\
				wdt_reset();\
				wdt_disable();\
				WDTCSR |= (1<<WDCE) | (1<<WDE);\
				WDTCSR = 0x00;\
				}

	#define FKT_WDT_ON {\
				wdt_enable();\
				WDTCSR |= (1 << WDIE);\
				}

	#define FKT_WDT_PERIODE_ON(periode) {\
				wdt_enable((periode));\
				WDTCSR |= (1 << WDIE);\
				}

	#define FKT_WDT_WAKEUP_EVENT (!(WDTCSR)) //WDTCSR will be cleared after a WD event, otherwhise it was an ExtInterrupt

	#define FKT_WDT_POWERDOWN(periode) {\
		FKT_WDT_PERIODE_ON(periode);\
		lowPowerBodOff(SLEEP_MODE_PWR_DOWN);\
	}

	//with BOD_OFF only
	#define	FKT_POWERDOWN_FOREVER {\
				FKT_WDT_OFF;\
				lowPowerBodOff(SLEEP_MODE_PWR_DOWN);\
			}

	#define FKT_POWERDOWN(periode,cycles) {\
			for(uint16_t i=0; i<=cycles; i++) {\
					FKT_WDT_POWERDOWN(periode);\
			}\
			TIMING::configure();\
			idleCycles(1);\
	}
	/*
	void	TurnOffFunktions(bod_t bod, adc_t adc, ain_t ain, timer2_t timer2, timer1_t timer1, timer0_t timer0, spi_t spi, usart0_t usart0,	twi_t twi, wdt_t wdt) {
		// Temporary clock source variable
		unsigned char clockSource = 0;

		if(bod == BOD_OFF) {
			// turn off brown-out enable in software
			MCUCR = bit (BODS) | bit (BODSE);  // turn on brown-out enable select
			MCUCR = bit (BODS);        // this must be done within 4 clock cycles of above
		}

		if (timer2 == TIMER2_OFF) {
			if (TCCR2B & CS22) clockSource |= (1 << CS22);
			if (TCCR2B & CS21) clockSource |= (1 << CS21);
			if (TCCR2B & CS20) clockSource |= (1 << CS20);

			// Remove the clock source to shutdown Timer2
			TCCR2B &= ~(1 << CS22);
			TCCR2B &= ~(1 << CS21);
			TCCR2B &= ~(1 << CS20);

			power_timer2_disable();
		} else if(timer2 == TIMER2_ON) {
			if (clockSource & CS22) TCCR2B |= (1 << CS22);
			if (clockSource & CS21) TCCR2B |= (1 << CS21);
			if (clockSource & CS20) TCCR2B |= (1 << CS20);

			power_timer2_enable();
		}

		if (ain == AIN_OFF) {
			DIDR1 |= (1 << AIN1D) | (1 << AIN0D);
		} else if(ain == AIN_ON) {
			DIDR1 &= ~((1 << AIN1D) | (1 << AIN0D));
		}

		switch(adc) {
			case ADC_OFF:
				ADCSRA &= ~(1 << ADEN);
				DIDR0 |= (1 << ADC5D) | (1 << ADC4D) | (1 << ADC3D) | (1 << ADC2D) | (1 << ADC1D) | (1 << ADC0D);
				power_adc_disable();
			break;
			case ADC_ON:
				power_adc_enable();
				DIDR0 &= ~((1 << ADC5D) | (1 << ADC4D) | (1 << ADC3D) | (1 << ADC2D) | (1 << ADC1D) | (1 << ADC0D));
				ADCSRA |= (1 << ADEN);
			break;
			case ADC0_OFF:
				DIDR0 |= (1 << ADC0D);
				if((DIDR0 & 0x3F) == 0x3F) {
					ADCSRA &= ~(1 << ADEN);
					power_adc_disable();
				}
			break;
			case ADC0_ON:
				power_adc_enable();
				DIDR0 &= ~(1 << ADC0D); //Just Turn ADC0 on
				ADCSRA |= (1 << ADEN);
			break;
			case ADC2_OFF:
				DIDR0 |= (1 << ADC2D);
				if((DIDR0 & 0x3F) == 0x3F) {
					ADCSRA &= ~(1 << ADEN);
					power_adc_disable();
				}
			break;
			case ADC2_ON:
				power_adc_enable();
				DIDR0 &= ~(1 << ADC2D); //Just Turn ADC0 on
				ADCSRA |= (1 << ADEN);
			break;
			default: break;
		}

		if (timer1 == TIMER1_OFF)	{
			power_timer1_disable();
		} else if(timer1 == TIMER1_ON) {
			power_timer1_enable();
		}

		if (timer0 == TIMER0_OFF)	{
			power_timer0_disable();
		} else if(timer0 == TIMER0_ON) {
			power_timer0_enable();
		}

		if (spi == SPI_OFF) {
			SPCR &= ~(1<<SPE);
			power_spi_disable();
		} else if(spi == SPI_ON) {
			power_spi_enable();
			SPCR |= (1<<SPE);
		}

		if (usart0 == USART0_OFF)	{
			UCSR0B &= ~((1<<RXEN0) | (1<<TXEN0));
		 	power_usart0_disable();
		} else if(usart0 == USART0_ON) {
			power_usart0_enable();
			UCSR0B |= (1<<RXEN0) | (1<<TXEN0);
		}

		if (twi == TWI_OFF) {
			TWCR &= ~(1<<TWEN);
			power_twi_disable();
		} else if(twi == TWI_ON) {
			power_twi_enable();
			TWCR |= (1<<TWEN);
		}

		if (wdt == WDT_OFF)	{
			wdt_reset();
			wdt_disable();
			WDTCSR |= (1<<WDCE) | (1<<WDE);
			WDTCSR = 0x00;
	//	} else {
	//		wdt_enable();
	//		WDTCSR |= (1 << WDIE);
	//	}

	}
	*/

	/*******************************************************************************
	* Name: ISR (WDT_vect)
	* Description: Watchdog Timer interrupt service routine. This routine is
	*		           required to allow automatic WDIF and WDIE bit clearance in
	*			         hardware.
	*
	*******************************************************************************/
	#if LIBCALL_TurnOffFunctions
	ISR (WDT_vect) {
		// WDIE & WDIF is cleared in hardware upon entering this ISR
		wdt_disable();
	}
	#endif

	// Only Pico Power devices can change BOD settings through software
	#ifndef sleep_bod_disable
	#define sleep_bod_disable() 										\
	do { 																\
	  unsigned char tempreg; 													\
	  __asm__ __volatile__("in %[tempreg], %[mcucr]" "\n\t" 			\
	                       "ori %[tempreg], %[bods_bodse]" "\n\t" 		\
	                       "out %[mcucr], %[tempreg]" "\n\t" 			\
	                       "andi %[tempreg], %[not_bodse]" "\n\t" 		\
	                       "out %[mcucr], %[tempreg]" 					\
	                       : [tempreg] "=&d" (tempreg) 					\
	                       : [mcucr] "I" _SFR_IO_ADDR(MCUCR), 			\
	                         [bods_bodse] "i" (_BV(BODS) | _BV(BODSE)), \
	                         [not_bodse] "i" (~_BV(BODSE))); 			\
	} while (0)
	#endif

	// Only Pico Power devices can change BOD settings through software
	#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega1284P__)
	#define	lowPowerBodOff(mode)\
	do { 						\
	      set_sleep_mode(mode); \
	      cli();				\
	      sleep_enable();		\
				sleep_bod_disable(); \
	      sei();				\
	      sleep_cpu();			\
	      sleep_disable();		\
	      sei();				\
	} while (0);
	#endif

	#endif
#endif
