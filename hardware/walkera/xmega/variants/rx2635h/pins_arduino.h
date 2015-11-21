/*
  pins_arduino.h - Pin definition functions for Arduino
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2007 David A. Mellis

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA

  Updated for 'xmega' core by bob frazier, S.F.T. Inc. - http://mrp3.com/
  
  Updated for 'RX2635H' by Richard Prinz, http://www.min.at/prinz

  X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   
  
  
  
  The xmega code mods make a considerable number of assumptions
  about the pin number assignments (as defined here):

  DEFAULT MAPPING ('DIGITAL_IO_PIN_SHIFT' NOT DEFINED)
  ----------------------------------------------------
  PORTD - digital 0-7
  PORTC - digital 8-15
  PORTR - digital 16-17 (built-in LED on PORTR pin 1, aka '17')
  PORTA - analog 0-7, digital 18-25

  SPI is on PORTC (pins 4-7)
  
  Serial is implemented on PORTE pins 2 (RX), 3 (TX)
  Serial2 on PORTD, pins 6 (RX), 7 (TX)
  (no flow control) 
  
  PORTD pin 4 is connected to a LED.  
  Pin 4 is the 'built-in' LED, defined as 'LED_BUILTIN', and is active HIGH.

  Your Mileage May Vary, depending on your board layout.  Some boards shift the
  digital pin assignments by 2 so that digital I/O pin 0 is PORTD Rx, pin 13 is PORTC SCK, just
  like the Arduino ATmega board.  Then they align the physical pins so that a regular Arduino shield
  will work, and assign PORTD 0-1 to 2 different digital I/O pin numbers (such as 20 and 21).

  To facilitate that specific change, uncomment the #define for 'DIGITAL_IO_PIN_SHIFT', below.
  Alternately you can create a separate version of this file with a different variant name,
  such as 'xmega-compat', with the #define uncommented, stored in an appropriately named directory.

  ============================
  HARDWARE SERIAL FLOW CONTROL
  ============================

  This version of the xmega Arduino startup+support files supports 
  HARDWARE FLOW CONTROL on the default serial port (PORTE) via
  RTS (output) and CTS (input). CTS is implemented as input from remote 
  device's DTR. RTS is implemented as DTR output.

  To implement RTS/CTS, use definitions similar to the following in your 
  version of this header file

  NOTE: RTS(DTR) will be configured as an output, active low 
        (high tells sender to stop sending data to the UART)
		
        CTS will be configured as an input, active low 
		(high stops data from being sent out via UART)

  CTS high to low transition causes an interrupt that may result 
  in serial I/O (for faster response time).

  // RTS(DTR) as GPIO 6 (port D pin 6)
  #define SERIAL_0_RTS_PORT_NAME PORTD
  #define SERIAL_0_RTS_PIN_INDEX 6

  // CTS as GPIO 7 (port D pin 7)
  #define SERIAL_0_CTS_PORT_NAME PORTD
  #define SERIAL_0_CTS_PIN_INDEX 7

*/





#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <avr/pgmspace.h>

// for now, the default xmega uses a simple assignment of digital pin numbers, 
// beginning with port D to accomodate a useful "compatibility" shield design, 
// these pins can be shifted so that the pin that maps to 'digitalRead(0)' 
// would be D2 rather than D0. This also puts 'Serial' on pins 0 and 1
// exactly like the Arduino UNO.  
// For any other mapping, create your own 'pins_arduino.h' file.
//

// UNCOMMENT THIS to shift digital pin assignments for Arduino 
// shield compatibility 
//#define DIGITAL_IO_PIN_SHIFT 


// NOTE: 'E' series can have analog inputs on PORTD. It can also support ARef 
// on PORTA pin 0, or PORTD pin 0
// @@@ see 24.14.3 in 'E' manual - this is the REFCTRL bits for the reference 
// @@@ select, AREF on PORTA (PA0)
#define USE_AREF					0x2 

#define NUM_DIGITAL_PINS            20


#ifdef USE_AREF
// could be 15 in an alternate version if I use PORTD as analog inputs
#define NUM_ANALOG_INPUTS           11

// PORTA pin 0 is not a valid input, so the first bit would be '1'
#ifdef analogInPinToBit
#undef analogInPinToBit
#endif

// analog pin 0 = 1 (PORTA), analog pin 7 = 0 (PORTD)
#define analogInPinToBit(P)			(((P) + 1) & 7) 

#else

// could be 16 in an alternate version if I use PORTD as analog inputs
#define NUM_ANALOG_INPUTS           12

#endif



#define analogInputToAnalogPin(p)	((p)-A0)
#define analogInputToDigitalPin(p)	((p < NUM_ANALOG_INPUTS) ? (p) + NUM_DIGITAL_PINS : -1)


// pins with PWM
// no shift:  0, 5-15
// shift:  3-6, 8-13, 16-17
//#ifdef DIGITAL_IO_PIN_SHIFT
//#define digitalPinHasPWM(p)         (((p) >= 3 && (p) <= 13 && (p) != 7) || (p) == 16 || (p) == 17)
//#else
#define digitalPinHasPWM(p)         ((p) == 0 || ((p) >= 5 && (p) <= 15))
//#endif


// the first macro, 'digitalPinToInterrupt', is for the 'interruptNum' 
// parameter in 'attachInterrupt' and 'detachInterrupt'
// the second macro, 'digitalPinToIntMode', is for the 'mode' 
// parameter in 'attachInterrupt'.
#define digitalPinToInterrupt(p) \
  { register uint8_t uiPort = pgm_read_byte((&digital_pin_to_port_PGM[p])); \
    uiPort == _PD ? PORTD_INT0 : uiPort == _PC ? PORTC_INT0 : uiPort == _PA ? PORTA_INT0 : uiPort == _PR ? PORTR_INT0 : -1; }

#define digitalPinToIntMode(p) ((uint16_t)(pgm_read_byte(&(digital_pin_to_bit_mask_PGM[p]))) << INT_MODE_PIN_SHIFT)


// xmega-specific - Interrupt 'vector number' assignments:

// Interrupts are PORT-SPECIFIC, not pin specific.
// pin 2 on any port is always asynchronous (except for 'R' which doesn't have a pin 2)
// all other pins can manage synchronous interrupts. 'wakeup' from sleep mode
// and other async interrupts must be on a 'pin 2', on ports A through E
//

#define PORTD_INT0					0
#define PORTD_INT1					1
#define PORTC_INT0					2
#define PORTC_INT1					3
#define PORTE_INT0					4
#define PORTE_INT1					5
#define PORTA_INT0					6
#define PORTA_INT1					7
#define PORTB_INT0					8
#define PORTB_INT1					9
#define PORTR_INT0					10
#define PORTR_INT1					11

// defined here instead of wiring_private.h
#define EXTERNAL_NUM_INTERRUPTS		12

// was in wiring_external.h, moved here
#define EXTERNAL_INT_0				0
#define EXTERNAL_INT_1				1
#define EXTERNAL_INT_2				2
#define EXTERNAL_INT_3				3
#define EXTERNAL_INT_4				4
#define EXTERNAL_INT_5				5
#define EXTERNAL_INT_6				6
#define EXTERNAL_INT_7				7
#define EXTERNAL_INT_8				8
#define EXTERNAL_INT_9				9
#define EXTERNAL_INT_10				10
#define EXTERNAL_INT_11				11


// xmega 'A' series has 2 sets of UART and SPI.
// The default UART is assigned on Port E, pins PE2-3
// The default SPI is assigned on Port C, pins PC4-7
//
// Standard GPIO pins are assigned as follows:
//
// PD0-7 Digital 0-7
// PC0-7 Digital 8-15
// PE0-3 Digital 16-19
//
// PA0-7 analog A0-A7
// PB0-3 analog A8-A11
//
// ALL PORT REMAP registers must be assigned to 0 (default mappings for pins)
// this puts PWM output on pins 0-3 for PORT E (the timers are split for C and D)
// Additionally, CLKOUT should be 0 (no clock outputs on any port/pin).
//
// TIMERS
// Timer TC4 should be configured as 'TC5' (for 8 PWM outputs) by default, essentially
// as a dual 8-bit timer, more or less compatible with the Arduino's 3 timers and
// supporting all 8 pins on port C for PWM output.  Port C's timer supports
// the system clock. Port D's timer has only PD5, so only pins 4-7 can be used for
// PWM output.  TD5 won't remap the pins at all to 0-4.
//
// See 'E' manual (chapter 13?) on TC4/5 and TD5 for more on this


// --------------------------------------------
// DEFINITIONS FOR SERIAL PORTS AND DEFAULT TWI
// --------------------------------------------

#define DEFAULT_TWI					TWIE
#define DEFAULT_SPI					SPIC
#define SPI_PORT                    SPIC

// serial port 0
#define SERIAL_0_PORT_NAME			PORTD
#define SERIAL_0_USART_NAME			USARTD1
#define SERIAL_0_USART_DATA			USARTD1_DATA
#define SERIAL_0_RXC_ISR			ISR(USARTD1_RXC_vect)
#define SERIAL_0_DRE_ISR			ISR(USARTD1_DRE_vect)
// define THIS to re-map the pins from PD6-7 to PC6-7
//#define SERIAL_0_REMAP			PORTD_REMAP 
// the bit needed to remap the port if SERIAL_0_REMAP is defined
#define SERIAL_0_REMAP_BIT			4   
// the pin number on the port, not the mapped digital pin number
#define SERIAL_0_RX_PIN_INDEX		6
// the pin number on the port, not the mapped digital pin number
#define SERIAL_0_TX_PIN_INDEX		7
#define USARTD1_VECTOR_EXISTS

// serial port 1
#define SERIAL_1_PORT_NAME			PORTC
#define SERIAL_1_USART_NAME			USARTC0
#define SERIAL_1_USART_DATA			USARTC0_DATA
#define SERIAL_1_RXC_ISR			ISR(USARTC0_RXC_vect)
#define SERIAL_1_DRE_ISR			ISR(USARTC0_DRE_vect)
// define THIS to re-map the pins from PC6-7 to PE2-3
//#define SERIAL_1_REMAP			PORTC_REMAP
// the bit needed to remap the port if SERIAL_1_REMAP
#define SERIAL_1_REMAP_BIT			4
// the pin number on the port, not the mapped digital pin number
#define SERIAL_1_RX_PIN_INDEX		2
// the pin number on the port, not the mapped digital pin number
#define SERIAL_1_TX_PIN_INDEX		3
#define USARTC0_VECTOR_EXISTS


// For atmega/Arduino Uno shield compatibility, with DIGITAL_IO_PIN_SHIFT defined,
// the typical board/pin layout might be like this (for shield pins):
//
// TWI is on TWIC (port C pins 0/1).  pins marked as '~' have PWM
//
//               M M
//             S I O   T R
//         A   C S S S x x
//     S S R G K O I S 2 2              T R
//     C D E N 1 1 1 1                  x x
//     L A F D 3 2 1 0 9 8  7 6 5 4 3 2 1 0
// ----o-o-o-o-o-o-o-o-o-o--o-o-o-o-o-o-o-o----
//     P P P   P P P P P P  P P P P P P P P
//     C C A   C C C C C C  D D D D D D D D
//     1 0 0   5 6 7 4 3 2  1 7 6 5 4 0 3 2
//     ~ ~     ~ ~ ~ ~ ~ ~    ~ ~ ~ ~
//
//               T O P   V I E W
//
//                           p
//               3   3       w  P P P P P P
//               V   V 5     r  A A A A A A
//               3   3 V     +  6 5 4 3 2 1
// ------------o-o-o-o-o-o-o-o--o-o-o-o-o-o----
//             U I R 3 5 G G V  A A A A A A
//             N O E . V N N i  5 4 3 2 1 0
//             U R S 3   D D n
//             S E E V
//             E F T
//             D
//
// RESERVED PINS (not brought out):  PA7, PR0, PD4 (connected to LED_BUILTIN)
//
// ARef should have a 100 ohm (or similar) resistor to Vcc with a decoupling capacitor
// IOREF must be connected to 3.3V so that compatible shields can detect 3.3V logic
// Vin connects to the 'power in' pin (may have 9V or more on this pin)
// 5V can be left unconnected, or use a separate voltage reg for 5V
// 3.3v must be regulated and able to supply 1A or more for shields.
//
// NOTE:  PA0 is connected to 'AREF', and PORTA pins are shifted to PA1 through PA6 for
//        A0 through A5.
//
// Uno has automatic switching between 5V reg out and 5V from USB power.  When Vin is more
// than 6.6v, the Vin source supplies 5V (and 3.3V) to the board.  otherwise, the USB
// 5V power supplies 5V (and 3.3V) to the board.
//
// NOTE:  on the E5, PC7 is MOSI, PC5 is SCK (it's the other way on the other CPUs)
//
// Summary Map:  D0   D1   D2   D3   D4   D5   D6   D7   D8   D9   D10  D11  D12  D13
//               PD2  PD3  PD0  PD4  PD5  PD6  PD7  PD1  PC2  PC3  PC4  PC7  PC6  PC5
//               Rx   Tx                                 Rx2  Tx2  SS   MOSI MISO SCK
//
//               A0   A1   A2   A3   A4   A5
//               PA1  PA2  PA3  PA4  PA5  PA6
//
// TWIC:  PC0 (SDA), PC1 (SCL)
// AREF:  PA0 (~10k to Vcc)
//
// ASYNC INTERRUPTS:  A1, D0, D8 (can be used for 'wakeup')
//

// default SPI
static const uint8_t SS   = 12;
static const uint8_t MOSI = 13;
static const uint8_t MISO = 14;
static const uint8_t SCK  = 15;

// primary SPI on PC4-7
static const uint8_t SS0   = 12;
static const uint8_t MOSI0 = 13;
static const uint8_t MISO0 = 14;
static const uint8_t SCK0  = 15;

// secondary SPI on PD4-7
static const uint8_t SS1   = 4;
static const uint8_t MOSI1 = 5;
static const uint8_t MISO1 = 6;
static const uint8_t SCK1  = 7;

// default 2-wire on PC0,PC1 - TWIC
static const uint8_t SDA = 8;
static const uint8_t SCL = 9;

// default 'status' LED on PD4
static const uint8_t LED_BUILTIN = 4;

// analog ports
static const uint8_t A0 = 20;
static const uint8_t A1 = 21;
static const uint8_t A2 = 22;
static const uint8_t A3 = 23;
static const uint8_t A4 = 24;
static const uint8_t A5 = 25;
static const uint8_t A6 = 26;
static const uint8_t A7 = 27;

static const uint8_t A8 = 28;
static const uint8_t A9 = 29;
static const uint8_t A10 = 30;
static const uint8_t A11 = 31;

// on the xmega 'E' series, PA2, PC2, and PD2 are asynchronous ints.  Others 
// are 'synchronous' which means that they must be held in their 
// 'interrupt state' long enough for the system to detect them. In any case
// all digital input pins can be use as interrupts, synchronous or otherwise.


#ifdef ARDUINO_MAIN

const uint16_t PROGMEM port_to_mode_PGM[] = {
  NOT_A_PORT,                  // 0
  (uint16_t) &PORTA_DIR,       // PA
  (uint16_t) &PORTB_DIR,       // PB
  (uint16_t) &PORTC_DIR,       // PC
  (uint16_t) &PORTD_DIR,       // PD
  (uint16_t) &PORTE_DIR,       // PE
  NOT_A_PORT,                  // A series has no 'port R'
};

const uint16_t PROGMEM port_to_output_PGM[] = {
  NOT_A_PORT,                  // 0
  (uint16_t) &PORTA_OUT,       // PA
  (uint16_t) &PORTB_OUT,       // PB
  (uint16_t) &PORTC_OUT,       // PC
  (uint16_t) &PORTD_OUT,       // PD
  (uint16_t) &PORTE_OUT,       // PE
  NOT_A_PORT,                  // A series has no 'port R'
};

const uint16_t PROGMEM port_to_input_PGM[] = {
  NOT_A_PORT,                  // 0
  (uint16_t) &PORTA_IN,        // PA
  (uint16_t) &PORTB_IN,        // PB
  (uint16_t) &PORTC_IN,        // PC
  (uint16_t) &PORTD_IN,        // PD
  (uint16_t) &PORTE_IN,        // PE
  NOT_A_PORT,                  // A series has no 'port R'
};

// xmega has a per-pin config register as well.
// Normally these will be:
//		00000111 for analog
//		00000000 for digital 'totem pole'
//		00011111 for 'INPUT_PULLUP'
// 
// bits 2:0 (trigger)
//		000 both edges
//		001 rising
//		010 falling
//		011 level
//		111 input buffer disabled
//
// NOTE: 'input buffer disabled' required to use the 'IN' register 
//			(so default here) also port R does not support 'INTPUT_DISABLED' 
//			so use BOTHEDGES [0] instead
//			
// bits 5:3 (out/pull)
//		000 TOTEM [normal],
//		001 bus keeper [sticky],
//		010 pulldown,
//		011 pullup,
//		100 wired 'or'
//		101 wired 'and'
//		110 wired 'or' pulldown
//		111 wired 'and' pullup
//
// bit 6: (invert logic) 
//		0 normal
//		1 inverted
//
// bit 7: (unused)
//		0 must be zero
//
// NOTE: PORTA through PORTE support 'input buffer disabled' and this setting 
//		is recommended for analog inputs.

const uint16_t PROGMEM digital_pin_to_control_PGM[] = {
	(uint16_t) &PORTD_PIN0CTRL,  // PD 0 ** 0 **
	(uint16_t) &PORTD_PIN1CTRL,  // PD 1 ** 1 **
	(uint16_t) &PORTD_PIN2CTRL,  // PD 2 ** 2 ** USARTD_RX		ASYNC
	(uint16_t) &PORTD_PIN3CTRL,  // PD 3 ** 3 ** USARTD_TX
	(uint16_t) &PORTD_PIN4CTRL,  // PD 4 ** 4 ** default LED
	(uint16_t) &PORTD_PIN5CTRL,  // PD 5 ** 5 **
	(uint16_t) &PORTD_PIN6CTRL,  // PD 6 ** 6 **
	(uint16_t) &PORTD_PIN7CTRL,  // PD 7 ** 7 **

	(uint16_t) &PORTC_PIN0CTRL,  // PC 0 ** 8 ** SDA
	(uint16_t) &PORTC_PIN1CTRL,  // PC 1 ** 9 ** SCL
	(uint16_t) &PORTC_PIN2CTRL,  // PC 2 ** 10 **				ASYNC
	(uint16_t) &PORTC_PIN3CTRL,  // PC 3 ** 11 **
	(uint16_t) &PORTC_PIN4CTRL,  // PC 4 ** 12 ** SPI_SS
	(uint16_t) &PORTC_PIN5CTRL,  // PC 5 ** 13 ** SPI_MOSI
	(uint16_t) &PORTC_PIN6CTRL,  // PC 6 ** 14 ** SPI_MISO
	(uint16_t) &PORTC_PIN7CTRL,  // PC 7 ** 15 ** SPI_SCK

	(uint16_t) &PORTE_PIN0CTRL,  // PE 0 ** 16 **
	(uint16_t) &PORTE_PIN1CTRL,  // PE 1 ** 17 **				ASYNC 
	(uint16_t) &PORTE_PIN2CTRL,  // PE 2 ** 18 **
	(uint16_t) &PORTE_PIN3CTRL,  // PE 3 ** 19 ** 

#ifndef USE_AREF
	// AREF (when USE_AREF is defined, and then it won't be mapped)
	(uint16_t) &PORTA_PIN0CTRL,  // PA 0 ** 20 ** A0
#endif
	(uint16_t) &PORTA_PIN1CTRL,  // PA 1 ** 21 ** A1
	(uint16_t) &PORTA_PIN2CTRL,  // PA 2 ** 22 ** A2			ASYNC
	(uint16_t) &PORTA_PIN3CTRL,  // PA 3 ** 23 ** A3
	(uint16_t) &PORTA_PIN4CTRL,  // PA 4 ** 24 ** A4
	(uint16_t) &PORTA_PIN5CTRL,  // PA 5 ** 25 ** A5
	(uint16_t) &PORTA_PIN6CTRL,  // PA 6 ** 26 ** A6
	(uint16_t) &PORTA_PIN7CTRL,  // PA 7 ** 27 ** A7
	
	(uint16_t) &PORTB_PIN0CTRL,  // PB 1 ** 28 ** A8
	(uint16_t) &PORTB_PIN1CTRL,  // PB 2 ** 29 ** A9			ASYNC
	(uint16_t) &PORTB_PIN2CTRL,  // PB 3 ** 30 ** A10
	(uint16_t) &PORTB_PIN3CTRL,  // PB 4 ** 31 ** A11
};

const uint8_t PROGMEM digital_pin_to_port_PGM[] = {
  // PORTLIST
  // -------------------------------------------
  _PD,  // PD 0 ** 0 **
  _PD,  // PD 1 ** 1 **
  _PD,  // PD 2 ** 2 ** USARTD_RX
  _PD,  // PD 3 ** 3 ** USARTD_TX
  _PD,  // PD 4 ** 4 ** default LED
  _PD,  // PD 5 ** 5 **
  _PD,  // PD 6 ** 6 **
  _PD,  // PD 7 ** 7 **
  
  _PC,  // PC 0 ** 8 ** SDA
  _PC,  // PC 1 ** 9 ** SCL
  _PC,  // PC 2 ** 10 **
  _PC,  // PC 3 ** 11 **
  _PC,  // PC 4 ** 12 ** SPI_SS
  _PC,  // PC 5 ** 13 ** SPI_MOSI
  _PC,  // PC 6 ** 14 ** SPI_MISO
  _PC,  // PC 7 ** 15 ** SPI_SCK
  
  _PE,  // PE 0 ** 16 **
  _PE,  // PE 1 ** 17 **
  _PE,  // PE 2 ** 18 **
  _PE,  // PE 3 ** 19 **
  
#ifndef USE_AREF
  _PA,  // PA 0 ** 20 ** A0
#endif
  _PA,  // PA 1 ** 21 ** A1
  _PA,  // PA 2 ** 22 ** A2
  _PA,  // PA 3 ** 23 ** A3
  _PA,  // PA 4 ** 24 ** A4
  _PA,  // PA 5 ** 25 ** A5
  _PA,  // PA 6 ** 26 ** A6
  _PA,  // PA 7 ** 27 ** A7
  
  _PB,  // PB 0 ** 28 ** A8
  _PB,  // PB 1 ** 29 ** A9
  _PB,  // PB 2 ** 30 ** A10
  _PB,  // PB 3 ** 31 ** A11
};

const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[] = {
  // PIN IN PORT
  // -------------------------------------------
  _BV( 0 ),  // PD 0 ** 0 **
  _BV( 1 ),  // PD 1 ** 1 **
  _BV( 2 ),  // PD 2 ** 2 ** USARTD_RX
  _BV( 3 ),  // PD 3 ** 3 ** USARTD_TX
  _BV( 4 ),  // PD 4 ** 4 ** default LED
  _BV( 5 ),  // PD 5 ** 5 **
  _BV( 6 ),  // PD 6 ** 6 **
  _BV( 7 ),  // PD 7 ** 7 **
  
  _BV( 0 ),  // PC 0 ** 8 ** SDA
  _BV( 1 ),  // PC 1 ** 9 ** SCL
  _BV( 2 ),  // PC 2 ** 10 **
  _BV( 3 ),  // PC 3 ** 11 **
  _BV( 4 ),  // PC 4 ** 12 ** SPI_SS
  _BV( 5 ),  // PC 5 ** 13 ** SPI_MOSI
  _BV( 6 ),  // PC 6 ** 14 ** SPI_MISO
  _BV( 7 ),  // PC 7 ** 15 ** SPI_SCK
  
  _BV( 0 ),  // PE 0 ** 16 **
  _BV( 1 ),  // PE 1 ** 17 **
  _BV( 2 ),  // PE 2 ** 18 **
  _BV( 3 ),  // PE 3 ** 19 **
  
#ifndef USE_AREF
  _BV( 0 ),  // PA 0 ** 20 ** A0
#endif
  _BV( 1 ),  // PA 1 ** 21 ** A1
  _BV( 2 ),  // PA 2 ** 22 ** A2
  _BV( 3 ),  // PA 3 ** 23 ** A3
  _BV( 4 ),  // PA 4 ** 24 ** A4
  _BV( 5 ),  // PA 5 ** 25 ** A5
  _BV( 6 ),  // PA 6 ** 26 ** A6
  _BV( 7 ),  // PA 7 ** 27 ** A7
  
  _BV( 0 ),  // PB 0 ** 28 ** A8
  _BV( 1 ),  // PB 1 ** 29 ** A9
  _BV( 2 ),  // PB 2 ** 30 ** A10
  _BV( 3 ),  // PB 3 ** 31 ** A11
};


const uint8_t PROGMEM digital_pin_to_timer_PGM[] = {
  // TIMERS
  // -------------------------------------------
  // The timers on the E5 are a bit different than the others
  // Since TIMERD5 only goes tp PD4, PD5, PD6, PD7, I have to
  // map them to places that have PWM.

  TIMERD5,       // PD 0 ** 0 **
  TIMERD5,       // PD 1 ** 1 **
  TIMERD5,       // PD 2 ** 2 ** USARTD_RX
  TIMERD5,       // PD 3 ** 3 ** USARTD_TX
  TIMERD5,       // PD 4 ** 4 ** PWM 3
  TIMERD5,       // PD 5 ** 5 ** PWM 4
  TIMERD5,       // PD 6 ** 6 ** PWM 5
  TIMERD5,       // PD 7 ** 7 ** PWM 6
  
  TIMERC4,       // PC 0 ** 8 ** SDA
  TIMERC4,       // PC 1 ** 9 ** SCL
  TIMERC4,       // PC 2 ** 10 **
  TIMERC4,       // PC 3 ** 11 **
  TIMERC4,       // PC 4 ** 12 ** SPI_SS
  TIMERC4,       // PC 5 ** 13 ** SPI_MOSI
  TIMERC4,       // PC 6 ** 14 ** SPI_MISO
  TIMERC4,       // PC 7 ** 15 ** SPI_SCK
  
  NOT_ON_TIMER,  // PE 0 ** 16 **
  NOT_ON_TIMER,  // PE 1 ** 17 **
  NOT_ON_TIMER,  // PE 2 ** 18 **
  NOT_ON_TIMER,  // PE 3 ** 19 **

#ifndef USE_AREF
  NOT_ON_TIMER,  // PA 0 ** 20 ** A0
#endif
  NOT_ON_TIMER,  // PA 1 ** 21 ** A1
  NOT_ON_TIMER,  // PA 2 ** 22 ** A2
  NOT_ON_TIMER,  // PA 3 ** 23 ** A3
  NOT_ON_TIMER,  // PA 4 ** 24 ** A4
  NOT_ON_TIMER,  // PA 5 ** 25 ** A5
  NOT_ON_TIMER,  // PA 6 ** 26 ** A6
  NOT_ON_TIMER,  // PA 7 ** 27 ** A7
  
  NOT_ON_TIMER,  // PB 0 ** 28 ** A8
  NOT_ON_TIMER,  // PB 1 ** 29 ** A9
  NOT_ON_TIMER,  // PB 2 ** 30 ** A10
  NOT_ON_TIMER,  // PB 3 ** 31 ** A11
};
#endif


// These serial port names are intended to allow libraries and 
// architecture-neutral sketches to automatically default to the correct port 
// name for a particular type of use. For example, a GPS module would normally 
// connect to SERIAL_PORT_HARDWARE_OPEN, the first hardware serial port 
// whose RX/TX pins are not dedicated to another use.
//
// Port which normally prints to the Arduino Serial Monitor
// SERIAL_PORT_MONITOR
//
// Port which is USB virtual serial
// SERIAL_PORT_USBVIRTUAL
//
// Port which connects to a Linux system via Bridge library
// SERIAL_PORT_LINUXBRIDGE
//
// Hardware serial port, physical RX & TX pins.
// SERIAL_PORT_HARDWARE
//
// Hardware serial ports which are open for use.  Their RX & TX
// pins are NOT connected to anything by default.
// SERIAL_PORT_HARDWARE_OPEN

#define SERIAL_PORT_MONITOR			Serial
#define SERIAL_PORT_HARDWARE		Serial
#define SERIAL_HARDWARE_OPEN		Serial2

#endif

 
