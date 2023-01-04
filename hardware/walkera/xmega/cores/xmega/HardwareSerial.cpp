/*
  HardwareSerial.cpp - Hardware serial library for Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Modified 23 November 2006 by David A. Mellis
  Modified 28 September 2010 by Mark Sproul
  Modified 14 August 2012 by Alarus

  Updated for 'xmega' core by bob frazier, S.F.T. Inc. - http://mrp3.com/

  In some cases, the xmega updates make assumptions about the pin assignments.
  See 'pins_arduino.h' for more detail.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"
#include "pins_arduino.h"
#include "wiring_private.h"
#include "HardwareSerial.h"

#ifndef SERIAL_0_PORT_NAME
#define SERIAL_0_PORT_NAME PORTD
#define SERIAL_0_USART_NAME USARTD0
#define SERIAL_0_USART_DATA USARTD0_DATA
#define SERIAL_0_RXC_ISR ISR(USARTD0_RXC_vect)
#define SERIAL_0_DRE_ISR ISR(USARTD0_DRE_vect)
#define USARTD0_VECTOR_EXISTS
#define SERIAL_0_RX_PIN_INDEX 2
#define SERIAL_0_TX_PIN_INDEX 3
#else // check for new defs

#if !defined(USARTC0_VECTOR_EXISTS) && !defined(USARTD0_VECTOR_EXISTS) && !defined(USARTE0_VECTOR_EXISTS) && !defined(USARTF0_VECTOR_EXISTS) && !defined(USARTC1_VECTOR_EXISTS) && !defined(USARTD1_VECTOR_EXISTS) && !defined(USARTE1_VECTOR_EXISTS) && !defined(USARTF1_VECTOR_EXISTS)
#error you must define the 'USARTxx_VECTOR_EXISTS' macro for each serial port as of 1/14/2015 modifications
#endif // defined 'all that'

#endif // SERIAL_0_PORT_NAME

#ifndef SERIAL_1_PORT_NAME
#define SERIAL_1_PORT_NAME PORTC
#define SERIAL_1_USART_NAME USARTC0
#define SERIAL_1_USART_DATA USARTC0_DATA
#define SERIAL_1_RXC_ISR ISR(USARTC0_RXC_vect)
#define SERIAL_1_DRE_ISR ISR(USARTC0_DRE_vect)
#define USARTC0_VECTOR_EXISTS
#define SERIAL_1_RX_PIN_INDEX 2
#define SERIAL_1_TX_PIN_INDEX 3
#else // check for new defs

#if !defined(USARTC0_VECTOR_EXISTS) && !defined(USARTD0_VECTOR_EXISTS) && !defined(USARTE0_VECTOR_EXISTS) && !defined(USARTF0_VECTOR_EXISTS) && !defined(USARTC1_VECTOR_EXISTS) && !defined(USARTD1_VECTOR_EXISTS) && !defined(USARTE1_VECTOR_EXISTS) && !defined(USARTF1_VECTOR_EXISTS)
#error you must define the 'USARTxx_VECTOR_EXISTS' macro for each serial port as of 1/14/2015 modifications
#endif // defined 'all that'

#endif // SERIAL_1_PORT_NAME


// Define constants and variables for buffering incoming serial data.  We're
// using a ring buffer, in which 'head' is the index of the location to
// which to write the next incoming character and 'tail' is the index of the
// location from which to read.


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                          //
//   ____   _____  ____   ___     _     _           ____   _   _  _____  _____  _____  ____        ____  ___  _____ _____   //
//  / ___| | ____||  _ \ |_ _|   / \   | |         | __ ) | | | ||  ___||  ___|| ____||  _ \      / ___||_ _||__  /| ____|  //
//  \___ \ |  _|  | |_) | | |   / _ \  | |         |  _ \ | | | || |_   | |_   |  _|  | |_) |     \___ \ | |   / / |  _|    //
//   ___) || |___ |  _ <  | |  / ___ \ | |___      | |_) || |_| ||  _|  |  _|  | |___ |  _ <       ___) || |  / /_ | |___   //
//  |____/ |_____||_| \_\|___|/_/   \_\|_____|_____|____/  \___/ |_|    |_|    |_____||_| \_\_____|____/|___|/____||_____|  //
//                                           |_____|                                        |_____|                         //
//                                                                                                                          //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE:  in some cases I might want to override this.  It's now "overrideable" in 'pins_arduino.h'
// TODO:  support var length buffers from a pre-allocated linked list?  as an option?
#ifndef SERIAL_BUFFER_SIZE

#if !defined(SERIAL_2_PORT_NAME) && !defined(SERIAL_3_PORT_NAME) && !defined(SERIAL_4_PORT_NAME) && !defined(SERIAL_5_PORT_NAME) && !defined(SERIAL_6_PORT_NAME) && !defined(SERIAL_7_PORT_NAME)

// only 2 serial ports, use larger buffer because I can - this can be overridden in 'pins_arduino.h'
#define SERIAL_BUFFER_SIZE 128 /* I like... big... BUFFERS! */

#else // more than 2 serial ports

#define SERIAL_BUFFER_SIZE 64 /* reduce buffer size with *many* serial ports */

#endif // more than 2 serial ports?

#endif // SERIAL_BUFFER_SIZE

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//            _                 _              __   __                      //
//      _ __ (_) _ __    __ _  | |__   _   _  / _| / _|  ___  _ __  ___     //
//     | '__|| || '_ \  / _` | | '_ \ | | | || |_ | |_  / _ \| '__|/ __|    //
//     | |   | || | | || (_| | | |_) || |_| ||  _||  _||  __/| |   \__ \    //
//     |_|   |_||_| |_| \__, | |_.__/  \__,_||_|  |_|   \___||_|   |___/    //
//                      |___/                                               //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

struct ring_buffer
{
  unsigned char buffer[SERIAL_BUFFER_SIZE];
#if SERIAL_BUFFER_SIZE < 256
  // when buffer size is less than 256 bytes, use an unsigned char (it's faster, smaller)
  volatile uint8_t/*unsigned int*/ head;
  volatile uint8_t/*unsigned int*/ tail;
#else  // SERIAL_BUFFER_SIZE >= 256
  volatile unsigned int head;
  volatile unsigned int tail;
#endif // SERIAL_BUFFER_SIZE
};

// ring buffers for serial ports 1 and 2 (must zero head/tail before use)
// NOTE:  there are ALWAYS at LEAST 2 serial ports:
//        these are USARTD0 and USARTC0 (on pins 2,3) by default.

ring_buffer rx_buffer; //  =  { { 0 }, 0, 0 };  // SERIAL_0
ring_buffer tx_buffer; //  =  { { 0 }, 0, 0 };
ring_buffer rx_buffer2; //  =  { { 0 }, 0, 0 }; // SERIAL_1
ring_buffer tx_buffer2; //  =  { { 0 }, 0, 0 };

#ifdef SERIAL_2_PORT_NAME
ring_buffer rx_buffer3; //  =  { { 0 }, 0, 0 };
ring_buffer tx_buffer3; //  =  { { 0 }, 0, 0 };
#endif // SERIAL_2_PORT_NAME

#ifdef SERIAL_3_PORT_NAME
ring_buffer rx_buffer4; //  =  { { 0 }, 0, 0 };
ring_buffer tx_buffer4; //  =  { { 0 }, 0, 0 };
#endif // SERIAL_3_PORT_NAME

#ifdef SERIAL_4_PORT_NAME
ring_buffer rx_buffer5; //  =  { { 0 }, 0, 0 };
ring_buffer tx_buffer5; //  =  { { 0 }, 0, 0 };
#endif // SERIAL_4_PORT_NAME

#ifdef SERIAL_5_PORT_NAME
ring_buffer rx_buffer6; //  =  { { 0 }, 0, 0 };
ring_buffer tx_buffer6; //  =  { { 0 }, 0, 0 };
#endif // SERIAL_5_PORT_NAME

#ifdef SERIAL_6_PORT_NAME
ring_buffer rx_buffer7; //  =  { { 0 }, 0, 0 };
ring_buffer tx_buffer7; //  =  { { 0 }, 0, 0 };
#endif // SERIAL_6_PORT_NAME

#ifdef SERIAL_7_PORT_NAME
ring_buffer rx_buffer8; //  =  { { 0 }, 0, 0 };
ring_buffer tx_buffer8; //  =  { { 0 }, 0, 0 };
#endif // SERIAL_7_PORT_NAME


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//     _____  _                    ____               _                _    //
//    |  ___|| |  ___ __      __  / ___| ___   _ __  | |_  _ __  ___  | |   //
//    | |_   | | / _ \\ \ /\ / / | |    / _ \ | '_ \ | __|| '__|/ _ \ | |   //
//    |  _|  | || (_) |\ V  V /  | |___| (_) || | | || |_ | |  | (_) || |   //
//    |_|    |_| \___/  \_/\_/    \____|\___/ |_| |_| \__||_|   \___/ |_|   //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

//#ifdef SERIAL_0_CTS_ENABLED
//static char bWasCTS0;
//#endif // SERIAL_0_CTS_ENABLED
//#ifdef SERIAL_1_CTS_ENABLED
//static char bWasCTS1;
//#endif // SERIAL_1_CTS_ENABLED


// TODO:  _SOFT_ flow control enable/disable, arbitrary pin assignments for CTS/DTR

#if defined(SERIAL_0_CTS_ENABLED)
void InitSerialFlowControlInterrupt0(void)
{
register8_t *pCTRL;
uint8_t oldSREG;


  pCTRL = &(SERIAL_0_CTS_PORT->PIN0CTRL) + SERIAL_0_CTS_PIN_INDEX;

  SERIAL_0_CTS_PORT->DIR &= ~SERIAL_0_CTS_PIN; // it's an input

  *pCTRL = PORT_OPC_PULLUP_gc | PORT_ISC_BOTHEDGES_gc; //PORT_ISC_FALLING_gc; // interrupt on falling, pulldown resistor

  // this next section enables actual interrupts

  oldSREG = SREG; // store the interrupt flag basically

  cli(); // disable interrupts for a bit

  SERIAL_0_CTS_PORT->INT1MASK &= ~SERIAL_0_CTS_PIN;
      // TODO:  'E' series doesn't have 'INT1'
//  SERIAL_0_CTS_PORT->INTCTRL &= ~PORT_INT1LVL_gm;  // interrupt initially off

  SREG = oldSREG; // restore
}
#endif // defined(SERIAL_0_CTS_ENABLED)


#if defined(SERIAL_1_CTS_ENABLED)
static void InitSerialFlowControlInterrupt1(void)
{
register8_t *pCTRL;
uint8_t oldSREG;

  pCTRL = &(SERIAL_1_CTS_PORT->PIN0CTRL) + SERIAL_1_CTS_PIN_INDEX;

  SERIAL_1_CTS_PORT->DIR &= ~SERIAL_1_CTS_PIN; // it's an input

  *pCTRL = PORT_OPC_PULLUP_gc | PORT_ISC_BOTHEDGES_gc; //PORT_ISC_FALLING_gc; // interrupt on falling, pulldown resistor

  // this next section enables actual interrupts

  oldSREG = SREG; // store the interrupt flag basically

  cli(); // disable interrupts for a bit

  SERIAL_1_CTS_PORT->INT1MASK &= ~SERIAL_1_CTS_PIN; // interrupt off (for now)
      // TODO:  'E' series doesn't have 'INT1'
//  SERIAL_1_CTS_PORT->INTCTRL |= PORT_INT1LVL_gm; // max priority when I do this

  SREG = oldSREG; // restore
}
#endif // defined(SERIAL_1_CTS_ENABLED)


void InitSerialFlowControlInterrupts(void)
{
uint8_t oldSREG=SREG;

  cli(); // disable interrupts for a bit

#if defined(SERIAL_0_CTS_ENABLED)
  InitSerialFlowControlInterrupt0();
#endif // defined(SERIAL_0_CTS_ENABLED)

#if defined(SERIAL_1_CTS_ENABLED)
  InitSerialFlowControlInterrupt1();
#endif // defined(SERIAL_1_CTS_ENABLED)

  SREG = oldSREG; // restore
}


// helpers for hardware flow control
// these will send the 'next character' _NOW_ if one is available by
// restoring the 'DRE' interrupt.

void serial_0_cts_callback(void)
{
uint8_t oldSREG = SREG; // get this FIRST
#ifdef SERIAL_0_CTS_ENABLED
char bCTS = SERIAL_0_CTS_PORT->IN & SERIAL_0_CTS_PIN;
#endif // SERIAL_0_CTS_ENABLED


  cli(); // in case I'm currently doing somethign ELSE that affects tx_buffer

#ifdef SERIAL_0_CTS_ENABLED
  if(!bCTS) // it's cleared - turn off the interrupt
  {
    SERIAL_0_CTS_PORT->INT1MASK &= ~SERIAL_0_CTS_PIN;
      // TODO:  'E' series doesn't have 'INT1'
//    SERIAL_0_CTS_PORT->INTCTRL |= PORT_INT1LVL_gm; // max priority when I do this
  }
#endif // SERIAL_0_CTS_ENABLED

  if(tx_buffer.head != tx_buffer.tail) // only when there's something to send
  {
    // re-enable the DRE interrupt - this will cause transmission to
    // occur again without code duplication.  see HardwareSerial::write()

    (&(SERIAL_0_USART_NAME))->CTRLA /*USARTD0_CTRLA*/
      = _BV(USART_RXCINTLVL1_bp) | _BV(USART_RXCINTLVL0_bp)
      | _BV(USART_DREINTLVL1_bp) | _BV(USART_DREINTLVL0_bp); // set int bits for rx and dre (sect 19.14.3)
  }

  SREG=oldSREG; // interrupts re-enabled
}

void serial_1_cts_callback(void)
{
uint8_t oldSREG = SREG; // get this FIRST
#ifdef SERIAL_1_CTS_ENABLED
char bCTS = SERIAL_1_CTS_PORT->IN & SERIAL_1_CTS_PIN;
#endif // SERIAL_1_CTS_ENABLED


  cli(); // in case I'm currently doing somethign ELSE that affects tx_buffer

#ifdef SERIAL_1_CTS_ENABLED
  if(!bCTS) // it's cleared - turn off the interrupt
  {
    SERIAL_1_CTS_PORT->INT1MASK &= ~SERIAL_1_CTS_PIN;
      // TODO:  'E' series doesn't have 'INT1'
//    SERIAL_1_CTS_PORT->INTCTRL |= PORT_INT1LVL_gm; // max priority when I do this
  }
#endif // SERIAL_1_CTS_ENABLED

  if (tx_buffer2.head != tx_buffer2.tail) // only when there's something to send
  {
    // re-enable the DRE interrupt - this will cause transmission to
    // occur again without code duplication.  see HardwareSerial::write()

    USARTC0_CTRLA = _BV(USART_RXCINTLVL1_bp) | _BV(USART_RXCINTLVL0_bp)
                  | _BV(USART_DREINTLVL1_bp) | _BV(USART_DREINTLVL0_bp); // set int bits for rx and dre (sect 19.14.3)
  }

  SREG=oldSREG; // interrupts re-enabled
}



//////////////////////////////////////////////////////////////////////////////////////////
//                                                                                      //
//   _         _  _                 __                      _    _                      //
//  (_) _ __  | |(_) _ __    ___   / _| _   _  _ __    ___ | |_ (_)  ___   _ __   ___   //
//  | || '_ \ | || || '_ \  / _ \ | |_ | | | || '_ \  / __|| __|| | / _ \ | '_ \ / __|  //
//  | || | | || || || | | ||  __/ |  _|| |_| || | | || (__ | |_ | || (_) || | | |\__ \  //
//  |_||_| |_||_||_||_| |_| \___| |_|   \__,_||_| |_| \___| \__||_| \___/ |_| |_||___/  //
//                                                                                      //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

inline void store_char(unsigned char c, ring_buffer *buffer)
{
  unsigned int i = (unsigned int)(buffer->head + 1) % SERIAL_BUFFER_SIZE;

  // if we should be storing the received character into the location
  // just before the tail (meaning that the head would advance to the
  // current location of the tail), we're about to overflow the buffer
  // and so we don't write the character or advance the head.
  if (i != buffer->tail)
  {
    buffer->buffer[buffer->head] = c;
    buffer->head = i;
  }
}

inline char set_not_rts(ring_buffer *buffer)
{
  unsigned int i1 = (unsigned int)(buffer->head + 3) % SERIAL_BUFFER_SIZE;
  unsigned int i2 = (unsigned int)(buffer->head + 2) % SERIAL_BUFFER_SIZE;
  unsigned int i3 = (unsigned int)(buffer->head + 1) % SERIAL_BUFFER_SIZE;

  // if we should be storing the received character into the location
  // just before the tail (meaning that the head would advance to the
  // current location of the tail), we're about to overflow the buffer
  // and so we don't write the character or advance the head.
  return i1 == buffer->tail || i2 == buffer->tail || i3 == buffer->tail;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                          //
//   ___         _                                   _     _   _                    _  _                    //
//  |_ _| _ __  | |_  ___  _ __  _ __  _   _  _ __  | |_  | | | |  __ _  _ __    __| || |  ___  _ __  ___   //
//   | | | '_ \ | __|/ _ \| '__|| '__|| | | || '_ \ | __| | |_| | / _` || '_ \  / _` || | / _ \| '__|/ __|  //
//   | | | | | || |_|  __/| |   | |   | |_| || |_) || |_  |  _  || (_| || | | || (_| || ||  __/| |   \__ \  //
//  |___||_| |_| \__|\___||_|   |_|    \__,_|| .__/  \__| |_| |_| \__,_||_| |_| \__,_||_| \___||_|   |___/  //
//                                           |_|                                                            //
//                                                                                                          //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


SERIAL_0_RXC_ISR // ISR(USARTD0_RXC_vect)
{
unsigned char c;

#ifdef SERIAL_0_RTS_ENABLED
  if(set_not_rts(&rx_buffer)) // do I need to turn off RTS ?
  {
    SERIAL_0_RTS_PORT->OUT |= SERIAL_0_RTS_PIN; // set to '1'
    SERIAL_0_RTS_PORT->DIR |= SERIAL_0_RTS_PIN; // make sure it's an output
  }
#endif // SERIAL_0_RTS_ENABLED

  if((&(SERIAL_0_USART_NAME))->STATUS /*USARTD0_STATUS*/ & _BV(USART_RXCIF_bp)) // if there is data available
  {
    c = SERIAL_0_USART_DATA; //USARTD0_DATA;
    store_char(c, &rx_buffer);
  }
  else // I got an interrupt for some reason, just eat data from data reg
  {
    c = SERIAL_0_USART_DATA; //USARTD0_DATA;
  }
}

SERIAL_1_RXC_ISR // ISR(USARTC0_RXC_vect)
{
unsigned char c;

#ifdef SERIAL_1_RTS_ENABLED
  if(set_not_rts(&rx_buffer2)) // do I need to turn off RTS ?
  {
    SERIAL_1_RTS_PORT->OUT |= SERIAL_1_RTS_PIN; // set to '1'
    SERIAL_1_RTS_PORT->DIR |= SERIAL_1_RTS_PIN; // make sure it's an output
  }
#endif // SERIAL_0_RTS_ENABLED

  if((&(SERIAL_1_USART_NAME))->STATUS /*USARTC0_STATUS*/ & _BV(USART_RXCIF_bp)) // if there is data available
  {
    c = SERIAL_1_USART_DATA; //USARTC0_DATA;
    store_char(c, &rx_buffer2);
  }
  else // I got an interrupt for some reason, just eat data from data reg
  {
    c = SERIAL_1_USART_DATA; //USARTC0_DATA;
  }
}

#ifdef SERIAL_2_PORT_NAME
SERIAL_2_RXC_ISR // ISR(USARTE0_RXC_vect)
{
unsigned char c;

  if((&(SERIAL_2_USART_NAME))->STATUS /*USARTE0_STATUS*/ & _BV(USART_RXCIF_bp)) // if there is data available
  {
    c = SERIAL_2_USART_DATA; //USARTE0_DATA;
    store_char(c, &rx_buffer3);
  }
  else // I got an interrupt for some reason, just eat data from data reg
  {
    c = SERIAL_2_USART_DATA; //USARTE0_DATA;
  }
}
#endif // SERIAL_2_PORT_NAME

#ifdef SERIAL_3_PORT_NAME
SERIAL_3_RXC_ISR // ISR(USARTF0_RXC_vect)
{
unsigned char c;

  if((&(SERIAL_3_USART_NAME))->STATUS /*USARTF0_STATUS*/ & _BV(USART_RXCIF_bp)) // if there is data available
  {
    c = SERIAL_3_USART_DATA; //USARTF0_DATA;
    store_char(c, &rx_buffer4);
  }
  else // I got an interrupt for some reason, just eat data from data reg
  {
    c = SERIAL_3_USART_DATA; //USARTF0_DATA;
  }
}
#endif // SERIAL_3_PORT_NAME

#ifdef SERIAL_4_PORT_NAME
SERIAL_4_RXC_ISR
{
unsigned char c;

  if((&(SERIAL_4_USART_NAME))->STATUS & _BV(USART_RXCIF_bp)) // if there is data available
  {
    c = SERIAL_4_USART_DATA;
    store_char(c, &rx_buffer4);
  }
  else // I got an interrupt for some reason, just eat data from data reg
  {
    c = SERIAL_4_USART_DATA;
  }
}
#endif // SERIAL_4_PORT_NAME

#ifdef SERIAL_5_PORT_NAME
SERIAL_5_RXC_ISR
{
unsigned char c;

  if((&(SERIAL_5_USART_NAME))->STATUS & _BV(USART_RXCIF_bp)) // if there is data available
  {
    c = SERIAL_5_USART_DATA;
    store_char(c, &rx_buffer4);
  }
  else // I got an interrupt for some reason, just eat data from data reg
  {
    c = SERIAL_5_USART_DATA;
  }
}
#endif // SERIAL_5_PORT_NAME

#ifdef SERIAL_6_PORT_NAME
SERIAL_6_RXC_ISR
{
unsigned char c;

  if((&(SERIAL_6_USART_NAME))->STATUS & _BV(USART_RXCIF_bp)) // if there is data available
  {
    c = SERIAL_6_USART_DATA;
    store_char(c, &rx_buffer4);
  }
  else // I got an interrupt for some reason, just eat data from data reg
  {
    c = SERIAL_6_USART_DATA;
  }
}
#endif // SERIAL_6_PORT_NAME

#ifdef SERIAL_7_PORT_NAME
SERIAL_7_RXC_ISR
{
unsigned char c;

  if((&(SERIAL_7_USART_NAME))->STATUS & _BV(USART_RXCIF_bp)) // if there is data available
  {
    c = SERIAL_7_USART_DATA;
    store_char(c, &rx_buffer4);
  }
  else // I got an interrupt for some reason, just eat data from data reg
  {
    c = SERIAL_7_USART_DATA;
  }
}
#endif // SERIAL_7_PORT_NAME


SERIAL_0_DRE_ISR // ISR(USARTD0_DRE_vect)
{
#ifdef SERIAL_0_CTS_ENABLED
uint8_t oldSREG;
char bCTS = SERIAL_0_CTS_PORT->IN & SERIAL_0_CTS_PIN;
#endif // SERIAL_0_CTS_ENABLED


  if (
#ifdef SERIAL_0_CTS_ENABLED
      bCTS ||
#endif // SERIAL_0_CTS_ENABLED
      tx_buffer.head == tx_buffer.tail)
  {
#ifdef SERIAL_0_CTS_ENABLED
    if(bCTS)
    {
      oldSREG = SREG; // store the interrupt flag basically

      cli(); // disable interrupts for a bit (in case they were enabled)

//      bWasCTS0 = 1; // to mark that I set the interrupt

      SERIAL_0_CTS_PORT->INT1MASK |= SERIAL_0_CTS_PIN;
      // TODO:  'E' series doesn't have 'INT1'
      SERIAL_0_CTS_PORT->INTCTRL |= PORT_INT1LVL_gm; // max priority when I do this

      SREG = oldSREG; // restore
    }
#endif // SERIAL_0_CTS_ENABLED

    // Buffer empty, so disable interrupts
    // section 19.14.3 - the CTRLA register (interrupt stuff)
    (&(SERIAL_0_USART_NAME))->CTRLA /*USARTD0_CTRLA*/
      = _BV(USART_RXCINTLVL1_bp) | _BV(USART_RXCINTLVL0_bp); // only set these 2 (the DRE int is now OFF)
  }
  else
  {
    // There is more data in the output buffer. Send the next byte
    register unsigned char c = tx_buffer.buffer[tx_buffer.tail];
    tx_buffer.tail = (tx_buffer.tail + 1) % SERIAL_BUFFER_SIZE;

    SERIAL_0_USART_DATA = c; //USARTD0_DATA = c;
  }
}

SERIAL_1_DRE_ISR // ISR(USARTC0_DRE_vect)
{
#ifdef SERIAL_1_CTS_ENABLED
uint8_t oldSREG;
char bCTS = SERIAL_1_CTS_PORT->IN & SERIAL_1_CTS_PIN;
#endif // SERIAL_1_CTS_ENABLED


  if (
#ifdef SERIAL_1_CTS_ENABLED
      bCTS ||
#endif // SERIAL_1_CTS_ENABLED
      tx_buffer2.head == tx_buffer2.tail)

  {
#ifdef SERIAL_1_CTS_ENABLED
    if(bCTS)
    {
      oldSREG = SREG; // store the interrupt flag basically

      cli(); // disable interrupts for a bit

//      bWasCTS1 = 1; // to mark that I set the interrupt

      SERIAL_1_CTS_PORT->INT1MASK |= SERIAL_1_CTS_PIN;
      // TODO:  'E' series doesn't have 'INT1'
      SERIAL_1_CTS_PORT->INTCTRL |= PORT_INT1LVL_gm; // max priority when I do this

      SREG = oldSREG; // restore
    }
#endif // SERIAL_1_CTS_ENABLED

    // Buffer empty, so disable interrupts
    // section 19.14.3 - the CTRLA register (interrupt stuff)
    (&(SERIAL_1_USART_NAME))->CTRLA /*USARTC0_CTRLA*/
      = _BV(USART_RXCINTLVL1_bp) | _BV(USART_RXCINTLVL0_bp); // only set these 2 (the DRE int is now OFF)
  }
  else
  {
    // There is more data in the output buffer. Send the next byte
    register unsigned char c = tx_buffer2.buffer[tx_buffer2.tail];
    tx_buffer2.tail = (tx_buffer2.tail + 1) % SERIAL_BUFFER_SIZE;

    SERIAL_1_USART_DATA = c; //USARTC0_DATA = c;
  }
}

#ifdef SERIAL_2_PORT_NAME
SERIAL_2_DRE_ISR // ISR(USARTE0_DRE_vect)
{
  if (tx_buffer3.head == tx_buffer3.tail)
  {
    // Buffer empty, so disable interrupts
    // section 19.14.3 - the CTRLA register (interrupt stuff)
    (&(SERIAL_2_USART_NAME))->CTRLA /*USARTE0_CTRLA*/
      = _BV(USART_RXCINTLVL1_bp) | _BV(USART_RXCINTLVL0_bp); // only set these 2 (the DRE int is now OFF)
  }
  else
  {
    // There is more data in the output buffer. Send the next byte
    register unsigned char c = tx_buffer3.buffer[tx_buffer3.tail];
    tx_buffer3.tail = (tx_buffer3.tail + 1) % SERIAL_BUFFER_SIZE;

    SERIAL_2_USART_DATA = c; //USARTE0_DATA = c;
  }
}
#endif // SERIAL_2_PORT_NAME

#ifdef SERIAL_3_PORT_NAME
SERIAL_3_DRE_ISR // ISR(USARTF0_DRE_vect)
{
  if (tx_buffer4.head == tx_buffer4.tail)
  {
    // Buffer empty, so disable interrupts
    // section 19.14.3 - the CTRLA register (interrupt stuff)
    (&(SERIAL_3_USART_NAME))->CTRLA /*USARTF0_CTRLA*/
      = _BV(USART_RXCINTLVL1_bp) | _BV(USART_RXCINTLVL0_bp); // only set these 2 (the DRE int is now OFF)
  }
  else
  {
    // There is more data in the output buffer. Send the next byte
    register unsigned char c = tx_buffer4.buffer[tx_buffer4.tail];
    tx_buffer4.tail = (tx_buffer4.tail + 1) % SERIAL_BUFFER_SIZE;

    SERIAL_3_USART_DATA = c; //USARTE0_DATA = c;
  }
}
#endif // SERIAL_3_PORT_NAME

#ifdef SERIAL_4_PORT_NAME
SERIAL_4_DRE_ISR
{
  if (tx_buffer4.head == tx_buffer4.tail)
  {
    // Buffer empty, so disable interrupts
    // section 19.14.3 - the CTRLA register (interrupt stuff)
    (&(SERIAL_4_USART_NAME))->CTRLA
      = _BV(USART_RXCINTLVL1_bp) | _BV(USART_RXCINTLVL0_bp); // only set these 2 (the DRE int is now OFF)
  }
  else
  {
    // There is more data in the output buffer. Send the next byte
    register unsigned char c = tx_buffer4.buffer[tx_buffer4.tail];
    tx_buffer4.tail = (tx_buffer4.tail + 1) % SERIAL_BUFFER_SIZE;

    SERIAL_4_USART_DATA = c;
  }
}
#endif // SERIAL_4_PORT_NAME

#ifdef SERIAL_5_PORT_NAME
SERIAL_5_DRE_ISR
{
  if (tx_buffer4.head == tx_buffer4.tail)
  {
    // Buffer empty, so disable interrupts
    // section 19.14.3 - the CTRLA register (interrupt stuff)
    (&(SERIAL_5_USART_NAME))->CTRLA
      = _BV(USART_RXCINTLVL1_bp) | _BV(USART_RXCINTLVL0_bp); // only set these 2 (the DRE int is now OFF)
  }
  else
  {
    // There is more data in the output buffer. Send the next byte
    register unsigned char c = tx_buffer4.buffer[tx_buffer4.tail];
    tx_buffer4.tail = (tx_buffer4.tail + 1) % SERIAL_BUFFER_SIZE;

    SERIAL_5_USART_DATA = c;
  }
}
#endif // SERIAL_5_PORT_NAME

#ifdef SERIAL_6_PORT_NAME
SERIAL_6_DRE_ISR
{
  if (tx_buffer4.head == tx_buffer4.tail)
  {
    // Buffer empty, so disable interrupts
    // section 19.14.3 - the CTRLA register (interrupt stuff)
    (&(SERIAL_6_USART_NAME))->CTRLA
      = _BV(USART_RXCINTLVL1_bp) | _BV(USART_RXCINTLVL0_bp); // only set these 2 (the DRE int is now OFF)
  }
  else
  {
    // There is more data in the output buffer. Send the next byte
    register unsigned char c = tx_buffer4.buffer[tx_buffer4.tail];
    tx_buffer4.tail = (tx_buffer4.tail + 1) % SERIAL_BUFFER_SIZE;

    SERIAL_6_USART_DATA = c;
  }
}
#endif // SERIAL_6_PORT_NAME

#ifdef SERIAL_7_PORT_NAME
SERIAL_7_DRE_ISR
{
  if (tx_buffer4.head == tx_buffer4.tail)
  {
    // Buffer empty, so disable interrupts
    // section 19.14.3 - the CTRLA register (interrupt stuff)
    (&(SERIAL_7_USART_NAME))->CTRLA
      = _BV(USART_RXCINTLVL1_bp) | _BV(USART_RXCINTLVL0_bp); // only set these 2 (the DRE int is now OFF)
  }
  else
  {
    // There is more data in the output buffer. Send the next byte
    register unsigned char c = tx_buffer4.buffer[tx_buffer4.tail];
    tx_buffer4.tail = (tx_buffer4.tail + 1) % SERIAL_BUFFER_SIZE;

    SERIAL_7_USART_DATA = c;
  }
}
#endif // SERIAL_7_PORT_NAME


// this helper function calls the ISR directly whenever the status reg has the appropriate bit set, then clears the bit
// call this function when you're waiting for I/O, whenever interrupts are disabled.
void call_isr(volatile USART_t *pPort)
{
  if(pPort->STATUS & _BV(USART_RXCIF_bp))
  {
    if(0)
    {
      // so I can use 'else if' for everything else
    }
#ifdef USARTD0_VECTOR_EXISTS
    else if(pPort == &(USARTD0))
    {
      USARTD0_RXC_vect();
    }
#endif // USARTD0_VECTOR_EXISTS
#ifdef USARTD1_VECTOR_EXISTS
    else if(pPort == &USARTD1)
    {
      USARTD1_RXC_vect();
    }
#endif // USARTD1_VECTOR_EXISTS
#ifdef USARTC0_VECTOR_EXISTS
    else if(pPort == &USARTC0)
    {
      USARTC0_RXC_vect();
    }
#endif // USARTC0_VECTOR_EXISTS
#ifdef USARTC1_VECTOR_EXISTS
    else if(pPort == &USARTC1)
    {
      USARTC1_RXC_vect();
    }
#endif // USARTC1_VECTOR_EXISTS
#ifdef USARTE0_VECTOR_EXISTS
    else if(pPort == &USARTE0)
    {
      USARTE0_RXC_vect();
    }
#endif // USARTE0_VECTOR_EXISTS
#ifdef USARTE1_VECTOR_EXISTS
    else if(pPort == &USARTE1)
    {
      USARTE1_RXC_vect();
    }
#endif // USARTE1_VECTOR_EXISTS
#ifdef USARTF0_VECTOR_EXISTS
    else if(pPort == &USARTF0)
    {
      USARTF0_RXC_vect();
    }
#endif // USARTF0_VECTOR_EXISTS
#ifdef USARTF1_VECTOR_EXISTS
    else if(pPort == &USARTF1)
    {
      USARTF1_RXC_vect();
    }
#endif // USARTF1_VECTOR_EXISTS

    pPort->STATUS = _BV(USART_RXCIF_bp); // clear THIS one.  other bits must be written as zero
  }

  if(pPort->STATUS & _BV(USART_DREIF_bp))
  {
    if(0)
    {
      // so I can use 'else if' below
    }
#ifdef USARTD0_VECTOR_EXISTS
    else if(pPort == &USARTD0)
    {
      USARTD0_DRE_vect();
    }
#endif // USARTD0_VECTOR_EXISTS
#ifdef USARTD1_VECTOR_EXISTS
    else if(pPort == &USARTD1)
    {
      USARTD1_DRE_vect();
    }
#endif // USARTD1_VECTOR_EXISTS
#ifdef USARTC0_VECTOR_EXISTS
    else if(pPort == &USARTC0)
    {
      USARTC0_DRE_vect();
    }
#endif // USARTC0_VECTOR_EXISTS
#ifdef USARTC1_VECTOR_EXISTS
    else if(pPort == &USARTC1)
    {
      USARTC1_DRE_vect();
    }
#endif // USARTC1_VECTOR_EXISTS
#ifdef USARTE0_VECTOR_EXISTS
    else if(pPort == &USARTE0)
    {
      USARTE0_DRE_vect();
    }
#endif // USARTE0_VECTOR_EXISTS
#ifdef USARTE1_VECTOR_EXISTS
    else if(pPort == &USARTE1)
    {
      USARTE1_DRE_vect();
    }
#endif // USARTE1_VECTOR_EXISTS
#ifdef USARTF0_VECTOR_EXISTS
    else if(pPort == &USARTF0)
    {
      USARTF0_DRE_vect();
    }
#endif // USARTF0_VECTOR_EXISTS
#ifdef USARTF1_VECTOR_EXISTS
    else if(pPort == &USARTF1)
    {
      USARTF1_DRE_vect();
    }
#endif // USARTF1_VECTOR_EXISTS

    pPort->STATUS = _BV(USART_DREIF_bp); // clear THIS one.  other bits must be written as zero
  }
}


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//    ____               _         _   _____                     _          //
//   / ___|   ___  _ __ (_)  __ _ | | | ____|__   __ ___  _ __  | |_  ___   //
//   \___ \  / _ \| '__|| | / _` || | |  _|  \ \ / // _ \| '_ \ | __|/ __|  //
//    ___) ||  __/| |   | || (_| || | | |___  \ V /|  __/| | | || |_ \__ \  //
//   |____/  \___||_|   |_| \__,_||_| |_____|  \_/  \___||_| |_| \__||___/  //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

void serialEvent() __attribute__((weak));
void serialEvent() {}
#define serialEvent_implemented

void serialEvent2() __attribute__((weak));
void serialEvent2() {}
#define serialEvent2_implemented

#ifdef SERIAL_2_PORT_NAME
void serialEvent3() __attribute__((weak));
void serialEvent3() {}
#define serialEvent3_implemented
#endif // SERIAL_2_PORT_NAME

#ifdef SERIAL_3_PORT_NAME
void serialEvent4() __attribute__((weak));
void serialEvent4() {}
#define serialEvent4_implemented
#endif // SERIAL_3_PORT_NAME

#ifdef SERIAL_4_PORT_NAME
void serialEvent5() __attribute__((weak));
void serialEvent5() {}
#define serialEvent5_implemented
#endif // SERIAL_4_PORT_NAME

#ifdef SERIAL_5_PORT_NAME
void serialEvent6() __attribute__((weak));
void serialEvent6() {}
#define serialEvent6_implemented
#endif // SERIAL_5_PORT_NAME

#ifdef SERIAL_6_PORT_NAME
void serialEvent7() __attribute__((weak));
void serialEvent7() {}
#define serialEvent7_implemented
#endif // SERIAL_6_PORT_NAME

#ifdef SERIAL_7_PORT_NAME
void serialEvent8() __attribute__((weak));
void serialEvent8() {}
#define serialEvent8_implemented
#endif // SERIAL_7_PORT_NAME

void serialEventRun(void)
{
// TODO: support this

#ifdef serialEvent_implemented
#ifdef USBCON
  if (Serial1.available())
#else // normal
  if (Serial.available())
#endif // USBCON, normal
  {
    serialEvent();
  }
#endif // serialEvent_implemented

#ifdef serialEvent2_implemented
  if (Serial2.available())
  {
    serialEvent2();
  }
#endif // serialEvent2_implemented

#ifdef serialEvent3_implemented
  if (Serial3.available())
  {
    serialEvent3();
  }
#endif // serialEvent3_implemented

#ifdef serialEvent4_implemented
  if (Serial4.available())
  {
    serialEvent4();
  }
#endif // serialEvent4_implemented

#ifdef serialEvent5_implemented
  if (Serial5.available())
  {
    serialEvent5();
  }
#endif // serialEvent5_implemented

#ifdef serialEvent6_implemented
  if (Serial6.available())
  {
    serialEvent6();
  }
#endif // serialEvent6_implemented

#ifdef serialEvent7_implemented
  if (Serial7.available())
  {
    serialEvent7();
  }
#endif // serialEvent7_implemented

}


///////////////////////////////////////////////////////////////////////////////////
//                                                                               //
//   ____                     _   ____         _           ____        _         //
//  | __ )   __ _  _   _   __| | |  _ \  __ _ | |_  ___   / ___| __ _ | |  ___   //
//  |  _ \  / _` || | | | / _` | | |_) |/ _` || __|/ _ \ | |    / _` || | / __|  //
//  | |_) || (_| || |_| || (_| | |  _ <| (_| || |_|  __/ | |___| (_| || || (__   //
//  |____/  \__,_| \__,_| \__,_| |_| \_\\__,_| \__|\___|  \____|\__,_||_| \___|  //
//                                                                               //
//                                                                               //
///////////////////////////////////////////////////////////////////////////////////

// this function that uses 'canned' baud rate values is 'temporary'.  A proper one that
// calculates the baud rate values will appear at some point in the future, once I have
// a nice bullet-proof algorithm for it.
//
// See page 233 XMEGA-A-Manual.pdf

uint16_t temp_get_baud(unsigned long baud, uint8_t use_u2x)
{
    uint16_t i1;

    // standard baud rates
    static const unsigned long aBaud[] PROGMEM =
    {
        1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600,
        /*76800,*/ 115200, 230400, 460800, 921600
    };

#if F_CPU == 32000000
    // Note that this values assume F_CPU==32000000
    // 2x constants for standard baud rates
    static const uint16_t a2x[] PROGMEM =
    {
        0,                          // 1200
        (7 << 12) | 12,             // 2400
        (6 << 12) | 12,             // 4800
        (5 << 12) | 12,             // 9600
        (1 << 12) | 138,            // 14400
        (4 << 12) | 12,             // 19200
        138,                        // 28800
        (3 << 12) | 12,             // 38400
        (uint16_t)(-1 << 12) | 137, // 57600
        //(2 << 12) | 12,             // 76800
        (uint16_t)(-2 << 12) | 135, // 115200
        (uint16_t)(-3 << 12) | 131, // 230400
        (uint16_t)(-4 << 12) | 123, // 460800
        (uint16_t)(-5 << 12) | 107  // 921600
    };

    // 1x constants for standard baud rates
    static const uint16_t a1x[] PROGMEM =
    {
        0,                          // 1200
        (6 << 12) | 12,             // 2400
        (5 << 12) | 12,             // 4800
        (4 << 12) | 12,             // 9600
        138,                        // 14400
        (3 << 12) | 12,             // 19200
        (uint16_t)(-1 << 12) | 137, // 28800
        (2 << 12) | 12,             // 38400
        (uint16_t)(-2 << 12) | 135, // 57600
        //(1 << 12) | 12,             // 76800
        (uint16_t)(-3 << 12) | 131, // 115200
        (uint16_t)(-4 << 12) | 123, // 230400
        (uint16_t)(-5 << 12) | 107, // 460800
        (uint16_t)(-6 << 12) | 75   // 921600
    };
#elif F_CPU == 16000000
    // Note that this values assume F_CPU==16000000
    // 2x constants for standard baud rates
    static const uint16_t a2x[] PROGMEM =
    {
        (15U << 12) + 3331,  // 1200
        (14U << 12) + 3329,  // 2400
        (13U << 12) + 3325,  // 4800
        (12U << 12) + 3317,  // 9600
        (12U << 12) + 2206,  // 14400
        (11U << 12) + 3301,  // 19200
        (11U << 12) + 2190,  // 28800
        (10U << 12) + 3269,  // 38400
        (10U << 12) + 2158,  // 57600
        (10U << 12) + 1047,  // 115200
        (9U << 12) + 983,    // 230400
        (9U << 12) + 428,    // 460800
        (9U << 12) + 150     // 921600
    };

    // 1x constants for standard baud rates
    static const uint16_t a1x[] PROGMEM =
    {
        (14U << 12) + 3329,  // 1200
        (13U << 12) + 3325,  // 2400
        (12U << 12) + 3317,  // 4800
        (11U << 12) + 3301,  // 9600
        (11U << 12) + 2190,  // 14400
        (10U << 12) + 3269,  // 19200
        (10U << 12) + 2158,  // 28800
        (9U << 12) + 3205,   // 38400
        (9U << 12) + 2094,   // 57600
        (9U << 12) + 983,    // 115200
        (9U << 12) + 428,    // 230400
        (9U << 12) + 150,    // 460800
        (9U << 12) + 11      // 921600
    };
#else
#error Unsupported F_CPU for serial baud value generation
#endif

    // TODO:  binary search is faster, but uses more code
    for(i1 = 0; i1 < sizeof(aBaud) / sizeof(aBaud[0]); i1++)
    {
        unsigned long dw1 = pgm_read_dword(&aBaud[i1]);

        if(baud == dw1)
        {
            if(use_u2x)
                return pgm_read_word(&a2x[i1]);
            else
                return pgm_read_word(&a1x[i1]);
        }
    }

    // NOTE:  baud <= F_CPU / 16 for 1x, F_CPU / 8 for 2x
    //
    // X = clk_2x ? 8 : 16    bscale >= 0:  bsel = F_CPU / ( (2 ^ bscale) * X * baud) - 1
    //                                      baud = F_CPU / ( (2 ^ bscale) * X * (bsel + 1) )
    //                        bscale < 0:   bsel = (1 / (2 ^ (bscale))) * (F_CPU / (X * baud) - 1)
    //                                      baud = F_CPU / ( X * (((2 ^ bscale) * bsel) + 1) )
    //
    // NOTE:  if bsel is zero for a given bscale, then use bscale=0 and bsel=2^(bscale - 1)
    //        see section 19.3.1
    //
    // find 'best fit baud' by calculating the best 'bscale' and 'bsel' for a given baud
    // bscale is -7 through +7 so this can be done in a simple loop

    // for now [half the maximum baud rate]
    return 1;
}




/////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                         //
//   _   _                  _                             ____               _         _   //
//  | | | |  __ _  _ __  __| |__      __ __ _  _ __  ___ / ___|   ___  _ __ (_)  __ _ | |  //
//  | |_| | / _` || '__|/ _` |\ \ /\ / // _` || '__|/ _ \\___ \  / _ \| '__|| | / _` || |  //
//  |  _  || (_| || |  | (_| | \ V  V /| (_| || |  |  __/ ___) ||  __/| |   | || (_| || |  //
//  |_| |_| \__,_||_|   \__,_|  \_/\_/  \__,_||_|   \___||____/  \___||_|   |_| \__,_||_|  //
//                                                                                         //
//                                                                                         //
/////////////////////////////////////////////////////////////////////////////////////////////

// Constructors ////////////////////////////////////////////////////////////////

/*
void HardwareSerial::init(
    ring_buffer *rx_buffer0,
    ring_buffer *tx_buffer0,
    uint16_t usart0)
{
    register ring_buffer *pR = rx_buffer0;
    register ring_buffer *pT = tx_buffer0;

    _rx_buffer = pR; //rx_buffer0;
    _tx_buffer = pT; //tx_buffer0;
    _usart = (volatile USART_t *)usart0;

    pR->head = 0;
    pR->tail = 0;
    pT->head = 0;
    pT->tail = 0;
    // memset(rx_buffer0, 0, sizeof(*rx_buffer0));
    // memset(tx_buffer0, 0, sizeof(*tx_buffer0));
}
*/

HardwareSerial::HardwareSerial(
    ring_buffer *rx_buffer0,
    ring_buffer *tx_buffer0,
    uint16_t usart0) /*__attribute__ ((noinline))*/
{
    register ring_buffer *pR = rx_buffer0;
    register ring_buffer *pT = tx_buffer0;

    // init(rx_buffer0, tx_buffer0, usart0); this is larger code, left for reference

    _rx_buffer = pR; //rx_buffer0;
    _tx_buffer = pT; //tx_buffer0;
    _usart = (volatile USART_t *)usart0;

    pR->head = 0;
    pR->tail = 0;
    pT->head = 0;
    pT->tail = 0;
    // memset(rx_buffer0, 0, sizeof(*rx_buffer0));
    // memset(tx_buffer0, 0, sizeof(*tx_buffer0));
}

// Public Methods //////////////////////////////////////////////////////////////

// 'D' manual, section 19.5
// USART Initialization
// USART initialization should use the following sequence:
// 1. Set the TxD pin value high, and optionally set the XCK pin low.
// 2. Set the TxD and optionally the XCK pin as output.
// 3. Set the baud rate and frame format.
// 4. Set the mode of operation (enables XCK pin output in synchronous mode).
// 5. Enable the transmitter or the receiver, depending on the usage.
// For interrupt-driven USART operation, global interrupts should be disabled during the initialization.
// Before doing a re-initialization with a changed baud rate or frame format, be sure that there are no ongoing transmissions
// while the registers are changed.

void HardwareSerial::begin(
    unsigned long baud)
{
    begin(baud, SERIAL_8N1);
}

void HardwareSerial::begin(
    unsigned long baud,
    byte config)
{
    uint16_t baud_setting;
    uint8_t use_u2x;
    uint8_t bit, bitTX=3, bitRX=2; // defaults
    volatile uint8_t *reg;
    volatile uint8_t *out;
    volatile uint8_t *ctrlT;
    volatile uint8_t *ctrlR;
    uint8_t oldSREG;


//    if (baud <= 57600)
    if (baud <= 57600)
        use_u2x = 0;
    else
        // enable CLK2X - bit 2 in the CTRLB
        // register (section 19.14.4)
        use_u2x = _BV(USART_CLK2X_bp);

    // pre-assign
    transmitting = false;

    // baud rate calc - page 220 table 19-E5
    //                  [for standard values]
    //                  table 19-1 (page 211)
    //                  for calculation formulae
    // (also see theory discussion on page 219)
    baud_setting = temp_get_baud(baud, use_u2x);

    // save old to restore interrupts as they
    // were
    oldSREG = SREG;
    // clear interrupt flag until I'm done
    // assigning pin stuff
    cli();

    // pin re-mapping register and port/pin
    // assignments

    if(_usart == &SERIAL_0_USART_NAME)
    {
        bitTX = (SERIAL_0_TX_PIN_INDEX);
        bitRX = (SERIAL_0_RX_PIN_INDEX);
#ifdef SERIAL_0_REMAP
        // enable re-mapping for this port
        SERIAL_0_REMAP |= SERIAL_0_REMAP_BIT;
#endif // SERIAL_0_REMAP
    }
    else if(_usart == &SERIAL_1_USART_NAME)
    {
        bitTX = (SERIAL_1_TX_PIN_INDEX);
        bitRX = (SERIAL_1_RX_PIN_INDEX);
#ifdef SERIAL_1_REMAP
        // enable re-mapping for this port
        SERIAL_1_REMAP |= SERIAL_1_REMAP_BIT;
#endif // SERIAL_0_REMAP
    }
#ifdef SERIAL_2_PORT_NAME
    else if(_usart == &SERIAL_2_USART_NAME)
    {
        bitTX = (SERIAL_2_TX_PIN_INDEX);
        bitRX = (SERIAL_2_RX_PIN_INDEX);
#ifdef SERIAL_2_REMAP
        // enable re-mapping for this port
        SERIAL_2_REMAP |= SERIAL_2_REMAP_BIT;
#endif // SERIAL_2_REMAP
    }
#endif // SERIAL_2_PORT_NAME
#ifdef SERIAL_3_PORT_NAME
    else if(_usart == &SERIAL_3_USART_NAME)
    {
        bitTX = (SERIAL_3_TX_PIN_INDEX);
        bitRX = (SERIAL_3_RX_PIN_INDEX);
#ifdef SERIAL_3_REMAP
        // enable re-mapping for this port
        SERIAL_3_REMAP |= SERIAL_3_REMAP_BIT;
#endif // SERIAL_3_REMAP
    }
#endif // SERIAL_3_PORT_NAME
#ifdef SERIAL_4_PORT_NAME
    else if(_usart == &SERIAL_4_USART_NAME)
    {
        bitTX = (SERIAL_4_TX_PIN_INDEX);
        bitRX = (SERIAL_4_RX_PIN_INDEX);
#ifdef SERIAL_4_REMAP
        // NOTE: no remap for serial 4 through
        // Serial 7
#warning pin remap not supported for 'SERIAL_4'
#endif // SERIAL_4_REMAP
    }
#endif // SERIAL_4_PORT_NAME
#ifdef SERIAL_5_PORT_NAME
    else if(_usart == &SERIAL_5_USART_NAME)
    {
        bitTX = (SERIAL_5_TX_PIN_INDEX);
        bitRX = (SERIAL_5_RX_PIN_INDEX);
#ifdef SERIAL_5_REMAP
        // NOTE: no remap for serial 5
        // through Serial 7
#warning pin remap not supported for 'SERIAL_5'
#endif // SERIAL_5_REMAP
    }
#endif // SERIAL_5_PORT_NAME
#ifdef SERIAL_6_PORT_NAME
    else if(_usart == &SERIAL_6_USART_NAME)
    {
        bitTX = (SERIAL_6_TX_PIN_INDEX);
        bitRX = (SERIAL_6_RX_PIN_INDEX);
#ifdef SERIAL_6_REMAP
        // NOTE: no remap for serial 6
        // through Serial 7
#warning pin remap not supported for 'SERIAL_6'
#endif // SERIAL_6_REMAP
    }
#endif // SERIAL_6_PORT_NAME
#ifdef SERIAL_7_PORT_NAME
    else if(_usart == &SERIAL_7_USART_NAME)
    {
        bitTX = (SERIAL_7_TX_PIN_INDEX);
        bitRX = (SERIAL_7_RX_PIN_INDEX);
#ifdef SERIAL_7_REMAP
        // NOTE: no remap for serial 7
        // through Serial 7
#warning pin remap not supported for 'SERIAL_7'
#endif // SERIAL_7_REMAP
    }
#endif // SERIAL_7_PORT_NAME
    else
    {
        // not valid
        goto exit_point;
    }

// USART setup - for existing ports only
// (otherwise we just return)

    if(_usart == &USARTD0
#ifdef USARTD1
        || _usart == &USARTD1
#endif
        )
    {
        reg = &PORTD_DIR;
        out = &PORTD_OUT;
        ctrlT = &PORTD_PIN0CTRL + bitTX;
        ctrlR = &PORTD_PIN0CTRL + bitRX;
    }
    else if(_usart == &USARTC0
#ifdef USARTC1
        || _usart == &USARTC1
#endif
        )
    {
        reg = &PORTC_DIR;
        out = &PORTC_OUT;
        ctrlT = &PORTC_PIN0CTRL + bitTX;
        ctrlR = &PORTC_PIN0CTRL + bitRX;
    }
#ifdef USARTE0
    else if(_usart == &USARTE0
#ifdef USARTE1
        || _usart == &USARTE1
#endif
        )
    {
        reg = &PORTE_DIR;
        out = &PORTE_OUT;
        ctrlT = &PORTE_PIN0CTRL + bitTX;
        ctrlR = &PORTE_PIN0CTRL + bitRX;
    }
#endif
#ifdef USARTF0
    else if(_usart == &USARTF0
#ifdef USARTF1
        || _usart == &USARTF1
#endif
        )
    {
        reg = &PORTF_DIR;
        out = &PORTF_OUT;
        ctrlT = &PORTF_PIN0CTRL + bitTX;
        ctrlR = &PORTF_PIN0CTRL + bitRX;
    }
#endif
    else
    {
         // not valid
        goto exit_point;
    }

    // port config, transmit bit
    bit = 1 << bitTX;
    *ctrlT = 0; // trigger on BOTH, totem, no pullup
    *out |= bit; // set to 'HIGH'
    *reg |= bit; // set as output

    // port config, receive bit
    bit = 1 << bitRX;
    *ctrlR = 0; // triger on BOTH, no pullup
    *out &= ~bit; // off
    *reg &= ~bit; // set as input

    // section 19.4.4
    // enable clock 2x when set
    // (everything else disabled)
    _usart->CTRLB = use_u2x;


    // section 19.14.5 - USART mode, parity,
    // bits
    // CMODE 7:6   00 [async]
    // PMODE 5:4   00=none  10=even 11=odd
    // SBMODE 3    0=1 stop  1=2 stop
    // CHSIZE 2:0  000=5 bit 001=6 bit  010=7 bit  011=8 bit  111=9 bit
    _usart->CTRLC = config & ~(_BV(USART_CMODE1_bp)|_BV(USART_CMODE0_bp)); // make sure bits 6 and 7 are cleared
#ifdef USARTD0_CTRLD
    // E5 has this register, must assign to zero
    _usart->CTRLD = 0;
#endif

    // assign the baud_setting, a.k.a. ubbr
    // (USART Baud Rate Register)
    _usart->BAUDCTRLA = (uint8_t)(baud_setting & 0xff);
    _usart->BAUDCTRLB = (uint8_t)(baud_setting >> 8);

    // section 19.4.4
    // enable RX, enable TX.
    // Bit 2 will be 1 or 0 based on
    // clock 2x/1x.  multi-processor disabled.
    // bit 9 = 0
    _usart->CTRLB = use_u2x | _BV(USART_RXEN_bp) | _BV(USART_TXEN_bp);

    // priority 3 for RX interrupts.
    // DRE and TX interrupts OFF (for now).
    _usart->CTRLA = _BV(USART_RXCINTLVL1_bp) | _BV(USART_RXCINTLVL0_bp);

exit_point:
    // restore interrupt flag
    // (now that I'm done assigning things)
    SREG = oldSREG;
}

void HardwareSerial::end()
{
    // wait for transmission of outgoing data
    // TODO:  check for 'in an ISR' or stalled?

    // interrupts are enabled
    if(SREG & CPU_I_bm)
    {
        // if ints not disabled it's safe to
        // do this - wait for tx_buffer to empty

        while (_tx_buffer->head != _tx_buffer->tail) { }
    }

    // disable RX, TX
    _usart->CTRLB = 0;
    // disable interrupts
    _usart->CTRLA = 0;

    // clear any received data
    _rx_buffer->head = _rx_buffer->tail;
}

int HardwareSerial::available(void)
{
    register int iRval;
    // save old to restore interrupts as they were
    register uint8_t oldSREG = SREG;

    // clear interrupt flag to prevent
    // inconsistency
    cli();

    iRval = (int)((unsigned int)(SERIAL_BUFFER_SIZE + (unsigned int)_rx_buffer->head - (unsigned int)_rx_buffer->tail)
                % SERIAL_BUFFER_SIZE);

    // restore interrupt flag
    SREG = oldSREG;

    return iRval;
}

int HardwareSerial::peek(void)
{
  register int iRval;
  register uint8_t oldSREG = SREG; // save old to restore interrupts as they were

  cli(); // clear interrupt flag to prevent inconsistency

  if (_rx_buffer->head == _rx_buffer->tail)
  {
    iRval = -1;
  }
  else
  {
    iRval = _rx_buffer->buffer[_rx_buffer->tail];
  }

  SREG = oldSREG; // restore interrupt flag

  return iRval;
}

int HardwareSerial::read(void)
{
  register int iRval;
  uint8_t oldSREG = SREG;

  cli(); // clear interrupt flag for consistency

  // each time I'm ready to read a byte, double-check that the RTS (when enabled)
  // needs to be set to a 0 value [which enables things to be sent to me].  As
  // I deplete the buffer, RTS will enable, and as I fill it, RTS will disable.

  // This section is the 'deplete' part.  So I'll set RTS to 'LOW' which is 'ok to send'
  // if the buffer is _NOT_ too full (the set_not_rts() function determines that)

#ifdef SERIAL_0_RTS_ENABLED
  if(_rx_buffer == &rx_buffer && // it's serial #0
     !set_not_rts(&rx_buffer))   // do I need to turn off RTS ?
  {
    SERIAL_0_RTS_PORT->OUT &= ~SERIAL_0_RTS_PIN; // set to '0'
    SERIAL_0_RTS_PORT->DIR |= SERIAL_0_RTS_PIN; // make sure it's an output
  }
#endif // SERIAL_0_RTS_ENABLED

#ifdef SERIAL_1_RTS_ENABLED
  if(_rx_buffer == &rx_buffer2 && // it's serial #1
     !set_not_rts(&rx_buffer2))   // do I need to turn off RTS ?
  {
    SERIAL_1_RTS_PORT->OUT &= ~SERIAL_1_RTS_PIN; // set to '0'
    SERIAL_1_RTS_PORT->DIR |= SERIAL_1_RTS_PIN; // make sure it's an output
  }
#endif // SERIAL_1_RTS_ENABLED

  // back to regular serial I/O handling

  // if the head isn't ahead of the tail, we don't have any characters
  if (_rx_buffer->head == _rx_buffer->tail)
  {
    iRval = -1;
  }
  else
  {
    iRval = (int)(_rx_buffer->buffer[_rx_buffer->tail]);

    _rx_buffer->tail = (unsigned int)(_rx_buffer->tail + 1) % SERIAL_BUFFER_SIZE;
  }

  SREG = oldSREG; // restore interrupt flag

  return iRval;
}

void HardwareSerial::flush()
{
  // TODO:  force an 'sei' here?

  // DATA is kept full while the buffer is not empty, so TXCIF triggers when EMPTY && SENT
  while (transmitting && !(_usart->STATUS & _BV(USART_TXCIF_bp))) // TXCIF bit 6 indicates transmit complete
    ;

  transmitting = false;
}

size_t HardwareSerial::write(uint8_t c)
{
register unsigned int i1;
uint8_t oldSREG;


  oldSREG = SREG; // get this FIRST
  cli(); // in case I'm currently doing somethign ELSE that affects the _tx_buffer

  i1 = (unsigned int)((_tx_buffer->head + 1) % SERIAL_BUFFER_SIZE); // next head after this char

  // If the output buffer is full, there's nothing for it other than to
  // wait for the interrupt handler to empty it a bit

  if (i1 == _tx_buffer->tail) // the buffer is still 'full'?
  {
    // if the interrupt flag is cleared in 'oldSREG' we must call the ISR directly
    // otherwise we can set the int flag and wait for it

    if(oldSREG & CPU_I_bm) // interrupts were enabled
    {
      // make sure that the USART's RXC and DRE interupts are enabled
      _usart->CTRLA = _BV(USART_RXCINTLVL1_bp) | _BV(USART_RXCINTLVL0_bp)
                    | _BV(USART_DREINTLVL1_bp) | _BV(USART_DREINTLVL0_bp); // set int bits for rx and dre (sect 19.14.3)
    }

    do
    {
      if(oldSREG & CPU_I_bm) // interrupts were enabled
      {
        sei(); // re-enable interrupts
        __builtin_avr_delay_cycles((F_CPU / 2000000) + 1); // delay ~17 cycles, enough time to allow for an interrupt to happen
      }
      else
      {
        call_isr(_usart); // this will block until there is a serial interrupt condition, but in an ISR-safe manner
      }

      i1 = (unsigned int)((_tx_buffer->head + 1) % SERIAL_BUFFER_SIZE); // next head after this char

      cli(); // do this regardless (smaller than another 'if' block, no harm if already clear)

    } while (i1 == _tx_buffer->tail); // while the buffer is still 'full'
  }

  // if I didn't have to wait for buffer space, I'm already covered

  _tx_buffer->buffer[_tx_buffer->head] = c;
  _tx_buffer->head = i1; // I already incremented it earlier, assume nobody ELSE modifies this

  // NOTE:  this messes with flow control.  it will still work, however
//  _usart->CTRLA |= _BV(1) | _BV(0); // make sure I (re)enable the DRE interrupt (sect 19.14.3)
  _usart->CTRLA = _BV(USART_RXCINTLVL1_bp) | _BV(USART_RXCINTLVL0_bp)
                | _BV(USART_DREINTLVL1_bp) | _BV(USART_DREINTLVL0_bp); // set int bits for rx and dre (sect 19.14.3)

  transmitting = true;
//  sbi(_usart->STATUS,6);  // clear the TXCIF bit by writing a 1 to its location (sect 19.14.2)
  _usart->STATUS = _BV(USART_TXCIF_bp); // other bits must be written as zero

  SREG=oldSREG; // interrupts re-enabled

  return 1;
}

HardwareSerial::operator bool()
{
  return true;
}


//////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                          //
//    ___   _      _              _     ___              _                                  //
//   / _ \ | |__  (_)  ___   ___ | |_  |_ _| _ __   ___ | |_  __ _  _ __    ___  ___  ___   //
//  | | | || '_ \ | | / _ \ / __|| __|  | | | '_ \ / __|| __|/ _` || '_ \  / __|/ _ \/ __|  //
//  | |_| || |_) || ||  __/| (__ | |_   | | | | | |\__ \| |_| (_| || | | || (__|  __/\__ \  //
//   \___/ |_.__/_/ | \___| \___| \__| |___||_| |_||___/ \__|\__,_||_| |_| \___|\___||___/  //
//              |__/                                                                        //
//                                                                                          //
//////////////////////////////////////////////////////////////////////////////////////////////

// Preinstantiate Objects //////////////////////////////////////////////////////

// NOTE:  object naming in 'approximate' accordance with the way it's being done with Arduino
//        except that 'Serial1' is *special*, and only defined when USB becomes 'Serial'
//        and 'Serial2' will always be the '2nd' serial port ('Serial1' *could* become an alias)

#ifdef USBCON
HardwareSerial Serial1(&rx_buffer, &tx_buffer, (uint16_t)&(SERIAL_0_USART_NAME)); // name changes to 'Serial1' when USB present
#else // normal
HardwareSerial Serial(&rx_buffer, &tx_buffer, (uint16_t)&(SERIAL_0_USART_NAME));
#endif // USBCON or normal

HardwareSerial Serial2(&rx_buffer2, &tx_buffer2, (uint16_t)&(SERIAL_1_USART_NAME));

#ifdef SERIAL_2_PORT_NAME  /* note these names are off by 1 with the 'Serial_N_' objects */
HardwareSerial Serial3(&rx_buffer3, &tx_buffer3, (uint16_t)&(SERIAL_2_USART_NAME));
#endif // SERIAL_2_PORT_NAME

#ifdef SERIAL_3_PORT_NAME
HardwareSerial Serial4(&rx_buffer4, &tx_buffer4, (uint16_t)&(SERIAL_3_USART_NAME));
#endif // SERIAL_3_PORT_NAME

#ifdef SERIAL_4_PORT_NAME
HardwareSerial Serial5(&rx_buffer5, &tx_buffer5, (uint16_t)&(SERIAL_3_USART_NAME));
#endif // SERIAL_4_PORT_NAME

#ifdef SERIAL_5_PORT_NAME
HardwareSerial Serial6(&rx_buffer6, &tx_buffer6, (uint16_t)&(SERIAL_3_USART_NAME));
#endif // SERIAL_5_PORT_NAME

#ifdef SERIAL_6_PORT_NAME
HardwareSerial Serial7(&rx_buffer7, &tx_buffer7, (uint16_t)&(SERIAL_3_USART_NAME));
#endif // SERIAL_6_PORT_NAME

#ifdef SERIAL_7_PORT_NAME
HardwareSerial Serial8(&rx_buffer8, &tx_buffer8, (uint16_t)&(SERIAL_3_USART_NAME));
#endif // SERIAL_7_PORT_NAME


