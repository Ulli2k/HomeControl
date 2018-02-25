
#if not defined(EMONLIBPRO_CONFIG_h) && defined(HAS_POWER_MONITOR_CT)
#define EMONLIBPRO_CONFIG_h

	#include "myBaseModule.h"
	// Berechnungsbasis für EmonLibPro: http://www.atmel.com/images/atmel-2566-single-phase-power-energy-meter-with-tamper-detection_ap-notes_avr465.pdf
	#if defined(HAS_POWER_MONITOR_VCC_1) && defined(HAS_POWER_MONITOR_VCC_2)
		#error Just one VCC channel can be configured!
	#endif
		
	//--------------------------------------------------------------------------------------------------
	// Hardware Configuration Variables
	//--------------------------------------------------------------------------------------------------
	//ADC sampling pin order map. First values must be Voltage, last Current. If no voltage sensors are used, don't put any pin number.
	//const byte adc_pin_order[] = {4,1,2,3,0,5};		
	const byte adc_pin_order[] = {
		#if defined(HAS_POWER_MONITOR_VCC_1)
			0,
		#elif defined(HAS_POWER_MONITOR_VCC_2)
			1,
		#endif
		#ifdef HAS_POWER_MONITOR_CT1_1
			6,
		#endif
		#ifdef HAS_POWER_MONITOR_CT1_2
			7,
		#endif
		#ifdef HAS_POWER_MONITOR_CT2_1
			4,
		#endif
		#ifdef HAS_POWER_MONITOR_CT2_2
			5,
		#endif
		#ifdef HAS_POWER_MONITOR_CT3_1
			2,
		#endif
		#ifdef HAS_POWER_MONITOR_CT3_2
			3,
		#endif
	};


	//--------------------------------------------------------------------------------------------------
	// User configurable Section
	//--------------------------------------------------------------------------------------------------
	#if defined(HAS_POWER_MONITOR_VCC_1) || defined(HAS_POWER_MONITOR_VCC_2)
		#define VOLTSCOUNT 		1        // Number of V sensors installed, can be 0 or 1, multiple voltage sensors are not supported this time
	#else
		#define VOLTSCOUNT 		0
	#endif

	// Number of CT sensors installed can be 1 to 3. (>3 --> IXCAL erweitern, was noch?)
	#if HAS_POWER_MONITOR_CT<=0
		#error Please define number of CTs with "HAS_POWER_MONITOR_CT=X"
	#endif
	#define CURRENTCOUNT 	HAS_POWER_MONITOR_CT
	
	#define CONSTVOLTAGE 	 220    // Only used if VOLTSCOUNT = 0 - Voltage RMS fixed value 
	#define CONSTFREQ     	50    // Only used if VOLTSCOUNT = 0 - Line frequency fixed value

	//#define V1CAL 245.23      // Gain for Voltage1 calculated value is 243:10.9 (for transformer) x 11:1 (for resistor divider) = 122.61
	#define V1CAL 	219        // Gain for Voltage1 calculated value is 243:10.9 (for transformer) x 12:1 (for resistor divider) = 122.61
																//216.5		//230(Main Voltage) × 13(VoltageDevider 10k+120k) ÷ ( 9 (Secondary Voltage) × 1.20(transformer without load) ) = 276,85 (emonTX v3)

	#define I1CAL 	13.333*POWER_MONITOR_CT_ADJUST            // Gain for CT1 calculated value is 20A:0.02A (for CT spirals) / 100 Ohms (for burden resistor) = 10
	#define I2CAL 	13.333*POWER_MONITOR_CT_ADJUST            // Gain for CT2 100:0.05 / 150 Ohm = 13,333
	#define I3CAL 	13.333*POWER_MONITOR_CT_ADJUST            // Gain for CT3

	#if VOLTSCOUNT!=0
		#define AUTOSAMPLERATE      // ADC sample rate is auto tracked for max sample rate. Requires a voltage sensor.
		//#define USEPLL            // PLL is active to track zero crosses as close to 0vac as possible. (not recommended as induces gitter on Hz calculation when pll is unlocked)
	#endif
	//#define DIAG                // Will populate CycleArray with one cycle data for diagnostics

	// Only used if not using AUTOSAMPLERATE
	// Samples per second (one sample unit includes all sensors)
	//#define SAMPLESPSEC   1250  // Samples per second (50Hz ok)
	//#define SAMPLESPSEC   1600  // Samples per second (50Hz ok)
	//#define SAMPLESPSEC   2000  // Samples per second (50Hz ok) (for 3 CT, 1 volt sensors)
	//#define SAMPLESPSEC   2500  // Samples per second (50Hz ok)
	#define SAMPLESPSEC   3200  // Samples per second (50Hz ok) (for 2 CT, 0 Voltage sensors)
	//#define SAMPLESPSEC   4000  // Samples per second (50Hz ok) (for 1 CT, 1 Voltage sensors)
	//#define SAMPLESPSEC   5000  // 250khz adc Samples per second (for 2 CT, 0 Voltage sensors)
	//#define SAMPLESPSEC   6400  // Samples per second (50Hz ok) (for 1 CT, 0 Voltage sensors)

	#define CALCULATECYCLES 				50 * 2 // Number of line cycles to activate FlagCALC_READY (50cycles x 5 = 5 secs)
	#define SAMPLE_CYCLE_TIME_MS		((CALCULATECYCLES * 1/CONSTFREQ)*1000)

	#define SUPPLY_VOLTS (VCC_SUPPLY_mV/1000.0)  // used here because it's more accurate than the internal band-gap reference

#endif
