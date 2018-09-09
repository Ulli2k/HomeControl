
#ifndef _MY_PWMPIN_h
#define _MY_PWMPIN_h

#include <myBaseModule.h>

typedef struct {
    uint8_t index;
    uint8_t reg;
    uint8_t uTCCx;
    uint8_t channel;
} multiplexPinDecription;

const multiplexPinDecription pinDescription[] = {
/* 0 */   { 1, PORT_PMUX_PMUXO_E, 1 , 1 },
/* 1 */   { 1, PORT_PMUX_PMUXE_E, 1 , 0 },
/* 2 */   { 2, PORT_PMUX_PMUXE_F, 0 , 0 },
/* 3 */   { 4, PORT_PMUX_PMUXO_F, 1 , 1 },
/* 4 */   { 4, PORT_PMUX_PMUXE_F, 1 , 0 },
/* 5 */   { 2, PORT_PMUX_PMUXO_F, 0 , 1 },
/* 6 */   { 6, PORT_PMUX_PMUXE_F, 0 , 2 },
/* 7 */   { 8, PORT_PMUX_PMUXO_E, 1 , 0 },
/* 8 */   { 6, PORT_PMUX_PMUXO_F, 0 , 3 },
/* 9 */   { 8, PORT_PMUX_PMUXE_E, 1 , 1 },
/* 10 */  { 10, PORT_PMUX_PMUXE_F, 0 , 2 },
/* 11 */  { 11, PORT_PMUX_PMUXE_E, 2, 0 },
/* 12 */  { 10, PORT_PMUX_PMUXO_F, 0 , 3 },
/* 13 */  { 11, PORT_PMUX_PMUXO_E, 2 , 1 },
/* 0 */   { 0 , 0, 0, 0 },
/* 0 */   { 0 , 0, 0, 0 },
/* 0 */   { 0 , 0, 0, 0 },
/* A3 */  { A3, PORT_PMUX_PMUXO_E, 0 , 0 }, //A3
/* A4 */  { A3, PORT_PMUX_PMUXE_E, 0 , 1 }, //A4
};

enum TCCPrescaler {
  DIV1      = TCC_CTRLA_PRESCALER_DIV1_Val,
  DIV2      = TCC_CTRLA_PRESCALER_DIV2_Val,
  DIV4      = TCC_CTRLA_PRESCALER_DIV4_Val,
  DIV8      = TCC_CTRLA_PRESCALER_DIV8_Val,
  DIV16     = TCC_CTRLA_PRESCALER_DIV16_Val,
  DIV64     = TCC_CTRLA_PRESCALER_DIV64_Val,
  DIV256    = TCC_CTRLA_PRESCALER_DIV256_Val,
  DIIV1024  = TCC_CTRLA_PRESCALER_DIV1024_Val,
};

enum GCLKSource {
  XOSC      = GCLK_GENCTRL_SRC_XOSC_Val,      /**< \brief (GCLK_GENCTRL) XOSC oscillator output */
  GCLKIN    = GCLK_GENCTRL_SRC_GCLKIN_Val,    /**< \brief (GCLK_GENCTRL) Generator input pad */
  GCLKGEN1  = GCLK_GENCTRL_SRC_GCLKGEN1_Val,  /**< \brief (GCLK_GENCTRL) Generic clock generator 1 output */
  OSCULP32K = GCLK_GENCTRL_SRC_OSCULP32K_Val, /**< \brief (GCLK_GENCTRL) OSCULP32K oscillator output */
  OSC32K    = GCLK_GENCTRL_SRC_OSC32K_Val,    /**< \brief (GCLK_GENCTRL) OSC32K oscillator output */
  XOSC32K   = GCLK_GENCTRL_SRC_XOSC32K_Val,   /**< \brief (GCLK_GENCTRL) XOSC32K oscillator output */
  OSC8M     = GCLK_GENCTRL_SRC_OSC8M_Val,     /**< \brief (GCLK_GENCTRL) OSC8M oscillator output */
  DFLL48M   = GCLK_GENCTRL_SRC_DFLL48M_Val,   /**< \brief (GCLK_GENCTRL) DFLL48M output */
};

class libPWM  {

  uint8_t _pin;
  const multiplexPinDecription *pinDes;
  Tcc* TCCx;
  uint32_t clockFactor;

  uint16_t getMaxValue(uint16_t frequency) {
    //Compare Register = [CPU_Hz / (Prescaler * TimerFreq_Hz)] - 1
    // return (uint16_t) ((48000000/*GCLK*//(1/*prescaler*/*frequency))-1);
    return (uint16_t) ((clockFactor/frequency)-1);
  }

  uint16_t getDutyCycleValue(uint8_t duty) { // 0-100
    return (uint16_t) (TCCx->PER.reg*(duty/100.));
  }

  void setGCLK(GCLKSource src=DFLL48M, uint8_t id=4, uint8_t div=1) {

    GCLK->GENCTRL.reg = GCLK_GENCTRL_IDC |			// Set the duty cycle to 50/50 HIGH/LOW
                        GCLK_GENCTRL_GENEN |		        // Enable GCLK4
                        GCLK_GENCTRL_SRC(src) |	    // Set the 48MHz clock source
                        GCLK_GENCTRL_ID(id);			  // Select GCLK4
    while (GCLK->STATUS.bit.SYNCBUSY);				// Wait for synchronization

    //set GCLK Prescaler
    GCLK->GENDIV.reg =  GCLK_GENDIV_DIV(div) |	// Divide the 48MHz clock source by divisor div: e.g. 48MHz/4=12MHz
                        GCLK_GENDIV_ID(id);			    // Select Generic Clock (GCLK) 4
    while (GCLK->STATUS.bit.SYNCBUSY);				    // Wait for synchronization
  }

  void mapGLK2TCC(uint8_t id) {
    // Feed GCLK4 to TCC0
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN |		// Enable GCLK4 to TCCx
                        GCLK_CLKCTRL_GEN(id) | // Select GCLK4
                        (pinDes->uTCCx <= 1 ? GCLK_CLKCTRL_ID_TCC0_TCC1 : GCLK_CLKCTRL_ID_TCC2_TC3);		// Feed GCLK4 to TCC0 and TCC1
    while (GCLK->STATUS.bit.SYNCBUSY);			// Wait for synchronization
  }

  void setTCCPrescaler(TCCPrescaler div=DIV1) {
    TCCx->CTRLA.reg |=  TCC_CTRLA_PRESCALER(div);// Divide GCLK4 by 1
    while (TCCx->SYNCBUSY.bit.ENABLE);			// Wait for synchronization
  }

public:
  libPWM(uint8_t pin) {
    _pin = pin;
    pinDes = &pinDescription[_pin];
    TCCx = (Tcc*) GetTC(pinDes->uTCCx);
  }

  void configClocks(uint8_t id, GCLKSource src=DFLL48M, TCCPrescaler div=DIV1) {
    setGCLK(src,id, 1); //GCLK Prescaler always 1
    mapGLK2TCC(id);
    setTCCPrescaler(DIV1);
    clockFactor = (src==DFLL48M?48000000:src==OSC8M?8000000:32000) / (div==DIV1?1:div==DIV2?2:div==DIV4?4:div==DIV8?8:div==DIV16?16:div==DIV64?64:div==DIV256?256:1024);
  }

  void multipexTCC2Pin() {
    PORT->Group[g_APinDescription[_pin].ulPort].PINCFG[g_APinDescription[_pin].ulPin].bit.PMUXEN = 1;
    PORT->Group[g_APinDescription[pinDes->index].ulPort].PMUX[g_APinDescription[pinDes->index].ulPin >> 1].reg |= pinDes->reg;
  }

  void setFrequence(uint16_t freq) {
    TCCx->WAVE.reg |= TCC_WAVE_WAVEGEN_NPWM; //single slope PWM; //TCC_WAVE_POL(0xF) /*Reverse the output polarity*/
    while (TCCx->SYNCBUSY.bit.WAVE);
    TCCx->PER.reg = TCC_PER_PER(getMaxValue(freq));					// Set the frequency of the PWM on TCC0
    while (TCCx->SYNCBUSY.bit.PER);				// Wait for synchronization
    TCCx->COUNT.reg = 0;
  }

  void setDutyCycle(uint8_t duty) { //0-100
    TCCx->CC[pinDes->channel].reg = TCC_CC_CC(getDutyCycleValue(duty));
    while (TCCx->SYNCBUSY.reg & TCC_SYNCBUSY_MASK);
  }

  void enableTCC() {
    // TCCx->INTENSET.reg = 0;                 // disable all interrupts
    // TCCx->INTENSET.bit.OVF = 1;          // enable overfollow
    // TCCx->INTENSET.bit.MC0 = 1;          // enable compare match to CC0
    // TCCx->INTENSET.bit.MC1 = 1;          // enable compare match to CC0
    // NVIC_ClearPendingIRQ(TCC0_IRQn);
    // NVIC_EnableIRQ(TCC0_IRQn);
    TCCx->CTRLA.reg |=  TCC_CTRLA_ENABLE;		// Enable the TCC0 output
    while (TCCx->SYNCBUSY.bit.ENABLE);			// Wait for synchronization
  }

  void disableTCC() {
    // NVIC_DisableIRQ(TCC0_IRQn);
    TCCx->CTRLA.reg &=  ~TCC_CTRLA_ENABLE;		// Enable the TCC0 output
    while (TCCx->SYNCBUSY.bit.ENABLE);			// Wait for synchronization
  }
};

// void TCC0_Handler() {
//
//   uint32_t counter = TCC0->COUNT.reg;
//   DS("time:");DU(millis()-last_ms,0);DNL();
//   DS("Cnt:");Serial1.println(counter);DNL();
//   DS("CC:"); Serial1.println(TCC0->CC[1].reg);
//   DS("Pin:");DU(digitalRead(5),0);DNL();
//   if (TCC0->INTFLAG.bit.MC0 == 1 || TCC0->INTFLAG.bit.MC1 == 1) {  // A compare to cc0 caused the interrupt
//     // DC('-');DU(micros()-last_ms,0);DNL();
//     TCC0->INTFLAG.bit.MC0 = 1;    // writing a one clears the flag ovf flag
//     TCC0->INTFLAG.bit.MC1 = 1;    // writing a one clears the flag ovf flag
//   } else if (TCC0->INTFLAG.bit.OVF == 1) {  // A overflow caused the interrupt
//     // DC('_');DU(micros()-last_ms,0);DNL();
//     TCC0->INTFLAG.bit.OVF = 1;    // writing a one clears the flag ovf flag
//   }
//   // TCC0->CC[0].reg = 15709;
//
//   last_ms = millis();
// }

template <uint8_t Pin, uint16_t frequency, uint8_t dutyCycle, uint8_t id=Pin>
class pwmPin : public myBaseModule, private Alarm { /*, protected SoftPWM*/

  libPWM pwm;
  bool active;

public:
  pwmPin() : pwm(Pin), Alarm(0), active(false) { async(true); }

  virtual void trigger (__attribute__((unused)) AlarmClock& clock) {
      pwm.disableTCC();
      active=false;
  }

  const char* getFunctionCharacter() { return "b"; };

  void initialize() {
    pwm.disableTCC();
    pwm.configClocks(4);
    pwm.multipexTCC2Pin();
    pwm.setFrequence(frequency);
    pwm.setDutyCycle(dutyCycle);
  }

  void send(char *cmd, uint8_t typecode) {
    if((cmd[0]-'0')!=id) return;
    cmd++;

    switch(typecode) {
      case MODULE_PWM_PIN_SIMPLE:
        if(!strlen(cmd) || active) return;
        sysclock.cancel(*this);
        tick = millis2ticks(atoi(cmd));
        active=true;
        pwm.enableTCC();
        sysclock.add(*this);
      break;
    }
  }

  void printHelp() {
    DS_P("\n ## PWM PIN - ID:");DU(id,0);DS(" ##\n");
    DS_P(" * [PWM]  b<id><ms>\n");
  }

  // void blockingPWM (const char* cmd) { //2300Hz --> 217µs on/off [HEX D9]
  //
  // 	if(strlen(cmd)<5) return;
  //   char s_freq[] = { cmd[0], cmd[1], cmd[2], cmd[3], 0x0 }; //max 99
  //   long freq = atol(s_freq); //1000000/freq/2; // calculate the delay value between transitions
  // 	long ms = atol(cmd+4);
  //   long delayValue = 250000/freq; // (1/f)*1000[ms]*1000[µs]/2*0,5 //calculate the delay value between transitions
  //   long numCycles = freq * ms/1000; // calculate the number of cycles for proper timing
  //
  //   pinMode(Pin, OUTPUT);
  //   for (long i=0; i < numCycles; i++) {
  //     digitalWrite(Pin,HIGH); // write the buzzer pin high to push out the diaphram
  //     delayMicroseconds(delayValue); // wait for the calculated delay value
  //     digitalWrite(Pin,LOW); // write the buzzer pin low to pull back the diaphram
  //     delayMicroseconds(delayValue); // wait again or the calculated delay value
  //   }
  //   pinMode(Pin, INPUT);
  // }

};
#endif
