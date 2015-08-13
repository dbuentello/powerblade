/*
 *
 */

#ifndef POWERBLADE_TEST_H_
#define POWERBLADE_TEST_H_

/**************************************************************************
   SAMPLE BUFFER SECTION
 **************************************************************************/
#define SAM_BUFSIZE	100

/**************************************************************************
   SYSTEN ENABLE SECTION
 **************************************************************************/
#ifdef VERSION0
   #define SYS_EN_DIR	PJDIR
   #define SYS_EN_OUT	PJOUT
   #define SYS_EN_PIN	BIT3
#endif
#ifdef VERSION1
	#define SYS_EN_DIR	P2DIR
	#define SYS_EN_OUT	P2OUT
	#define SYS_EN_PIN	BIT2
#endif
#ifdef VERSION31
	#define SYS_EN_DIR	P1DIR
	#define SYS_EN_OUT	P1OUT
	#define SYS_EN_PIN	BIT6
#endif

/**************************************************************************
   SENSE ENABLE SECTION
 **************************************************************************/
#if defined (VERSION0) | defined (VERSION1)
	#define SEN_EN_DIR	P1DIR
	#define SEN_EN_OUT	P1OUT
	#define SEN_EN_PIN	BIT7
#endif
#if defined (VERSION31)
	#define SEN_EN_DIR	P2DIR
	#define SEN_EN_OUT	P2OUT
	#define SEN_EN_PIN	BIT2
#endif

/**************************************************************************
   ANALOG SECTION
 **************************************************************************/
/* Supply cutoff information
	Rdiv = 1/3
	Vcc = 3.3V

	Vmin = 3.8V
	Vmin,div = 1.266V
	Nadc,min = 255 * (1.266 / 3.3) = 98 (0x62)

	Vchg = 8.5V
	Vchg,div = 2.833
	Nadc,chg = 255 * (2.833 / 3.3) = 219 (0xDB)

	Vmax = 9.5V
	Vmax,div = 3.17V
	Nadc,max = 255 * (3.17 / 3.3) = 245 (0xF5)
*/
//#define ADC_VMIN	0xB3
#define ADC_VMIN	0x73
#define ADC_VCHG	0xDB
#define ADC_VMAX	0xF5

#define ADC_VCC2	0x80
#define V_VCC2		ADC_VCC2
#define I_VCC2		0x7F

//#define ADC_PERUS	500
//#define ADC_PERCT	ADC_PERUS*(32768/1E6)
//#define ADC_PERUS	793.65
#define ADC_PERCT	26

/**************************************************************************
   METERING SECTION
 **************************************************************************/
// V_SENSE RESISTORS
#define RI			220
#define RF			1.5

/**************************************************************************
   SENSING CONSTANTS SECTION
 **************************************************************************/
#define CUROFF		6
#define PHASEOFF	1	// zero for in-phase
#define SAMCOUNT	42

#endif
