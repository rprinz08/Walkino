/*
  wiring_analog.c - analog input and output
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2005-2006 David A. Mellis

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

  Modified 28 September 2010 by Mark Sproul

  Updated for 'xmega' core by bob frazier, S.F.T. Inc. - http://mrp3.com/

  In some cases, the xmega updates make assumptions about the pin assignments.
  See 'pins_arduino.h' for more detail.

*/

#include "wiring_private.h"
#include "pins_arduino.h"

#define DEBUG_CODE
#ifdef DEBUG_CODE
extern void DebugOutL(unsigned long lVal);
extern void DebugOutP(const void * PROGMEM pStr);
#endif // DEBUG_CODE

#ifndef ADCA_SAMPCTRL
#define ADCA_SAMPCTRL  _SFR_MEM8(0x0208) /* missing from header for some reason, struct defines it as reserved_0x08 */
#endif // ADCA_SAMPCTRL


uint8_t analog_reference = 4;// the default analog reference is Vcc / 2

// adc_setup() - call this from init() and whenever you wake up from sleep mode
void adc_setup(void)
{
  // calibration is a 16-bit register - CAL0 + (CAL1 << 8)
  ADCA_CAL = (uint16_t)readCalibrationData((uint8_t)(uint16_t)&PRODSIGNATURES_ADCACAL0)
           | (((uint16_t)readCalibrationData((uint8_t)(uint16_t)&PRODSIGNATURES_ADCACAL1)) << 8);

  // must make sure power reduction register enables the ADC
  PR_PRPA &= ~PR_ADC_bm; // clear this bit to enable the ADC clock

  // assign clock prescaler for 100khz, and clear the interrupt bit
  // also make sure the 'interrupt enable' is OFF

  ADCA_EVCTRL = 0; // no triggering events (sect 22.14.4)

  ADCA_PRESCALER = ADC_PRESCALER_DIV256_gc; // 100khz, approximately, for 32Mhz clock

  ADCA_CTRLA = _BV(ADC_ENABLE_bp); // enables the ADC
  ADCA_CTRLB = _BV(6) | _BV(4); // medium current limit, signed mode [temporary]
  //   _BV(6) | _BV(5);     // section 22.14.2, 'HIGH' current limit, no apparent bit value constants in iox64d4.h
             // NOTE:  all other bits are zero - no 'freerun', 12-bit right-justified unsigned mode

#ifdef USE_AREF
  ADCA_REFCTRL = (USE_AREF << ADC_REFSEL_gp) & ADC_REFSEL_gm; // USE_AREF will ALSO be the correct bit assignment for 'REFSEL'
#else // USE_AREF
  ADCA_REFCTRL = _BV(ADC_REFSEL2_bp);         // bit 100 --> Vcc/2 as reference
#endif // USE_AREF
  // NOTE:  all other ADCA_REFCTRL bits are zero (bandgap, tempref) - section 22.14.3
  // TODO:  use an adjustable 'analog_reference'? (see also 'analogReference()' below)

  // TODO:  is this actually a RESERVED functionality?
  ADCA_SAMPCTRL = 24; // sect 22.14.8 - this value + 1 is # of "half cycles" used for sampling
                      // in this case, it's 25 "half cycles" at 100khz, or appx 8khz (125uS)
                      // adjust this for 'best accuracy' without taking for-freaking-evar
                      // also make sure the sample rate is lower than the 'HIGH LIMIT' max rate (see 22.14.2)

  // set up the channel (no offset calc at this time - later do an offset calc)
  ADCA_CH0_SCAN = 0;       // disable scan
  ADCA_CH0_INTCTRL = 0;    // no interrupts, flag on complete sect 22.15.3

  // NOTE:  the E5 has a significant difference in how it handles 'DIFF WITH GAIN'
  //        ADC_CH_INPUTMODE_DIFFWGAINL_gc uses A0-A3, GND, and internal GND (not same as D and earlier)
  //        ADC_CH_INPUTMODE_DIFFWGAINH_gc uses A4-A7 and GND (see E manual table 24-16,17 pg 366-7) (similar to D and earlier)
#if defined (__AVR_ATxmega16E5__) || defined (__AVR_ATxmega32E5__)

#ifdef USE_AREF
  // 'E' series has 2 modes for 'diff with gain', each using a different set of inputs (so A0-A3 for L, and A4-A7 for H mode)
#define ASSIGN_ADCA_CH0_CTRL ADC_CH_INPUTMODE_DIFFWGAINH_gc | /* MUXNEG 111b aka '7' still picks "GND" for 'diff with gain' for this one */ \
                             ADC_CH_GAIN_1X_gc;               /* 1X gain when I'm using 'AREF' */
#define MUXCTRL_MUXNEG 7 /* bits 111 which is GND for MUXNEG - see E manual 24.15.2 */
#else // USE_AREF
  // 'E' series has 2 modes for 'diff with gain', each using a different set of inputs (so A0-A3 for L, and A4-A7 for H mode)
#define ASSIGN_ADCA_CH0_CTRL ADC_CH_INPUTMODE_DIFFWGAINH_gc | /* MUXNEG 111b aka '7' still picks "GND" for 'diff with gain' for this one */ \
                             ADC_CH_GAIN_DIV2_gc;             /* (see 24.15.1 in E manual) */
#define MUXCTRL_MUXNEG 7 /* bits 111 which is GND for MUXNEG - see E manual 24.15.2 */
#endif // USE_AREF

#else // everything else not an 'E' series

#ifdef USE_AREF /* this is the same for 'A' series and 'D' series - double-check others */
#define ASSIGN_ADCA_CH0_CTRL ADC_CH_INPUTMODE_DIFF_gc | /* no gain if I use 'AREF' */ \
                             ADC_CH_GAIN_1X_gc;         /* (see 22.15.1 in D manual, 28.17.1 in A manual) */
#define MUXCTRL_MUXNEG 5 /* bits 101 which is GND for MUXNEG - see D manual 22.15.2, A manual 28.17.2 */
#else // USE_AREF
#define ASSIGN_ADCA_CH0_CTRL ADC_CH_INPUTMODE_DIFFWGAIN_gc | /* use gain of 1/2 if I don't use 'AREF' */ \
                             ADC_CH_GAIN_DIV2_gc;            /* (see 22.15.1 in D manual, 28.17.1 in A manual) */
#define MUXCTRL_MUXNEG 7 /* bits 111 which is GND for MUXNEG - see D manual 22.15.2, A manual 28.17.2 */
#endif // USE_AREF

#endif

  ADCA_CH0_CTRL = ASSIGN_ADCA_CH0_CTRL;

  // clear interrupt flag (probably not needed)
  ADCA_INTFLAGS = _BV(ADC_CH0IF_bp); // write a 1 to the interrupt bit (which clears it - 22.14.6)

  analogRead(0); // do a single conversion so that everything stabilizes

// these are taken care of at the beginning of the function, as a 16-bit register assignment to ADCA_CAL
// ADCA.CALL = readCalibrationData(&PRODSIGNATURES_ADCACAL0);
// ADCA.CALH = readCalibrationData(&PRODSIGNATURES_ADCACAL1);
}

void analogReference(uint8_t mode)
{
  // can't actually set the register here because the default setting
  // will connect AVCC and the AREF pin, which would cause a short if
  // there's something connected to AREF.

  // NOTE: on atmega, this is definitely the case.  On xmega, there's no 'AREF' output.

  analog_reference = mode;

  // valid xmega modes are 0-4 (5-7 are reserved)

//  ADCA_REFCTRL = (ADCA_REFCTRL & ~ADC_REFSEL_gm)
//               | ((mode & 7) << ADC_REFSEL_gp);         // section 22.14.3
}

// For 100% atmega compatibility, analogRead will return a value of 0-1023
// for input voltages of 0 to Vcc (assuming AVCC is connected to VCC, etc.)
// by using a gain of 1/2, a comparison of Vcc/2, and signed conversion

int analogRead(uint8_t pin)
{
  short iRval;

  // this is pure XMEGA code

  if(pin >= A0)
  {
    if(pin >= (NUM_ANALOG_INPUTS + A0)) // pin number too high?
    {
      return 0; // not a valid analog input
    }
#ifdef analogInputToAnalogPin
    pin = analogInputToAnalogPin(pin);
#else // analogInputToAnalogPin
    pin -= A0; // this works when PA0-PA7 and PB0-PBn are in sequence for A0-An
#endif // analogInputToAnalogPin
  }
  else
  {
    // NOTE:  for pins less than 'A0', assume it's referring to the analog index (0 to NUM_ANALOG_INPUTS-1)

    if(pin >= NUM_ANALOG_INPUTS)
    {
      return 0; // not a valid analog input
    }

#ifdef analogInputToAnalogPin
    pin = analogInputToAnalogPin(pin + A0); // calc pin number (might not have 0 mapped to A0)
#endif // analogInputToAnalogPin
  }

  // ANALOG REFERENCE - in some cases I can map one of the analog inputs
  //                    as an analog reference.  For now, assume it's Vcc/2.
  // NOTE:  On the A-series processors with more than a handful of inputs,
  //        it is NOT possible to use 'diff input with gain' on MORE than
  //        A0-A7.  On later processors (like D series) it _IS_ possible.
  //        Because of this, the 'hack' that allows rail-rail measurements
  //        is no longer possible on PB0-PB7.  PA0-PA7 will still work.
  //        IF PA0 or PB0 is used as VRef (see 28.16.3 in AU manual) via 'REFCTRL'
  //        then you can read all 15 remaining values with whatever VRef you want.


  ADCA_CH0_SCAN = 0; // disable scan
  ADCA_CH0_MUXCTRL = (pin << ADC_CH_MUXPOS_gp) // sect 22.15.2 in 'D' manual, 28.17.2 in 'A' manual, 24.15.2 in 'E' manual
                   | MUXCTRL_MUXNEG; // GND is the 'other input' (NOTE:  this is MUXNEG, can be changed)

  ADCA_CH0_INTCTRL = 0;    // no interrupts, flag on complete sect 22.15.3

#ifdef ADC_CH_IF_bm /* iox16e5.h and iox32e5.h - probably the ATMel Studio version */
  ADCA_CH0_INTFLAGS = ADC_CH_IF_bm; // write a 1 to the interrupt bit (which clears it - 22.15.4)
#else // everyone else
  ADCA_CH0_INTFLAGS = ADC_CH_CHIF_bm; // write a 1 to the interrupt bit (which clears it - 22.15.4)
#endif // ADC_CH_IF_bm

// old code, for reference
//  ADCA_CH0_CTRL = ADC_CH_START_bm       // conversion start
//                | ADC_CH_INPUTMODE0_bm; // zero gain and input mode '01' (see 22.15.1)

  ADCA_CH0_CTRL = ADC_CH_START_bm |       // conversion start
                  ASSIGN_ADCA_CH0_CTRL;   // defined above on a per-cpu with pins_arduino.h modifications to select gain+mode

#ifdef ADC_CH_IF_bm /* iox16e5.h and iox32e5.h - probably the ATMel Studio version */
  while(!(ADCA_CH0_INTFLAGS & ADC_CH_IF_bm)) { }
#else // everyone else
  while(!(ADCA_CH0_INTFLAGS & ADC_CH_CHIF_bm)) { }
#endif // ADC_CH_IF_bm

  iRval = ADCA_CH0_RES;

  if(iRval < 0) // backward compatibility
  {
    return 0;
  }

  return iRval / 2;  // -1023 to 1023 [TODO:  clip at zero?]
}

// Right now, PWM output only works on the pins with hardware support.
// These are defined in the appropriate pins_arduino.h file.  For the
// rest of the pins, we default to digital output with a 1 or 0

#ifdef TCC4
static void DoAnalogWriteForPort(TC4_t *port, uint8_t bit, uint8_t val);
#elif defined(TCC2)
static void DoAnalogWriteForPort(TC2_t *port, uint8_t bit, uint8_t val);
#else // TCC0
static void DoAnalogWriteForPort(TC0_t *port, uint8_t bit, uint8_t val);
#endif // TCC4, TCC2, TCC0

void analogWrite(uint8_t pin, int val)
{
  // We need to make sure the PWM output is enabled for those pins
  // that support it, as we turn it off when digitally reading or
  // writing with them.  Also, make sure the pin is in output mode
  // for consistenty with Wiring, which doesn't require a pinMode
  // call for the analog output pins.

  // NOTE:  period registers all contain zeros, which is the MAXIMUM period of 0-255
#ifdef TCC4 /* 'E' series and later that have TCC4 */
  uint8_t mode;
#endif // TCC4
  uint8_t bit = digitalPinToBitMask(pin);

  pinMode(pin, OUTPUT); // forces 'totem pole' - TODO allow for something different?

  // note 'val' is a SIGNED INTEGER.  deal with 'out of range' values accordingly

  if (val <= 0)
  {
    digitalWrite(pin, LOW);
  }
  else if (val >= 255)
  {
    digitalWrite(pin, HIGH);
  }
  else
  {
    // NOTE:  according to the docs, 16-bit registers MUST be accessed
    //        low byte first, then high byte, before the actual value
    //        is transferred to the register.  THIS code will.
    //        see A1U manual sect. 3.11 (and others as well)

    switch(digitalPinToTimer(pin))
    {
#ifdef TCC4 /* 'E' series and later that have TCC4 */

      case TIMERC4:
#ifdef DEBUG_CODE
        DebugOutP(PSTR("TIMERC4 "));
        DebugOutL(bit);
        DebugOutP(PSTR(","));
        DebugOutL(val);
        DebugOutP(PSTR("\r\n"));
#endif // DEBUG_CODE

        DoAnalogWriteForPort(&TCC4, bit, val); // TODO: smaller if inlined here?
        break;

      case TIMERD5:
        // THIS code is unique to the E5, most likely, so it's inlined
        if(bit == 1 || bit == 16) // TODO:  either bit?  not sure if I can re-map these to 0-3
        {
          *((volatile uint16_t *)&(TCD5_CCA)) = (TCD5_CCA & 0xff00) | (val & 0xff);
          mode = (TCD5_CTRLE & ~TC5_LCCAMODE_gm) | TC5_LCCAMODE0_bm;
        }
        else if(bit == 2 || bit == 32)
        {
          *((volatile uint16_t *)&(TCD5_CCB)) = (TCD5_CCB & 0xff00) | (val & 0xff);
          mode = (TCD5_CTRLE & ~TC5_LCCBMODE_gm) | TC5_LCCBMODE0_bm;
        }
        else if(bit == 4 || bit == 64)
        {
          *((volatile uint16_t *)&(TCD5_CCA)) = (TCD5_CCA & 0xff) | ((val << 8) & 0xff00);
          mode = (TCD5_CTRLF & ~TC5_HCCAMODE_gm) | TC5_HCCAMODE0_bm;
        }
        else if(bit == 8 || bit == 128)
        {
          *((volatile uint16_t *)&(TCD5_CCB)) = (TCD5_CCB & 0xff) | ((val << 8) & 0xff00);
          mode = (TCD5_CTRLF & ~TC5_HCCBMODE_gm) | TC5_HCCBMODE0_bm;
        }
        else
        {
          break;
        }

#ifdef DEBUG_CODE
        DebugOutP(PSTR("TIMERD5 "));
        DebugOutL(bit);
        DebugOutP(PSTR(","));
        DebugOutL(val);
        DebugOutP(PSTR(","));
        DebugOutL(TCD5_CCA);
        DebugOutP(PSTR(","));
        DebugOutL(TCD5_CCB);
        DebugOutP(PSTR(","));
        DebugOutL(mode);
        DebugOutP(PSTR("\r\n"));
#endif // DEBUG_CODE

        if(bit == 1 || bit == 2 ||  bit == 16 || bit == 32)
        {
          *((volatile uint8_t *)&(TCD5_CTRLE)) = mode;
#ifdef DEBUG_CODE
          DebugOutP(PSTR("E!\r\n"));
#endif // DEBUG_CODE
        }
        else
        {
          *((volatile uint8_t *)&(TCD5_CTRLF)) = mode;
#ifdef DEBUG_CODE
          DebugOutP(PSTR("F!\r\n"));
#endif // DEBUG_CODE
        }

        break;

#else // everything else NOT an 'E' series

      case TIMERD2:
#ifndef TCD2
        DoAnalogWriteForPort(&TCD0, bit, val);
#else // TCD2 defined
        DoAnalogWriteForPort(&TCD2, bit, val);
#endif // TCD2 defined
        break;

      case TIMERC2:

#ifndef TCC2
        DoAnalogWriteForPort(&TCC0, bit, val);
#else // TCC2 defined
        DoAnalogWriteForPort(&TCC2, bit, val);
#endif // TCC2 defined
        break;

#if NUM_DIGITAL_PINS > 22 /* meaning there is a PORT E available with 8 pins */

      case TIMERE2: // TIMER 'E2' for 8-bits

#ifndef TCE2
        DoAnalogWriteForPort(&TCE0, bit, val);
#else // TCE2 defined
        DoAnalogWriteForPort(&TCE2, bit, vao);
#endif // TCE2 defined
        break;

#if NUM_DIGITAL_PINS > 30 /* meaning there is a PORT F available */

      case TIMERF2:

#ifndef TCF2
        DoAnalogWriteForPort(&TCF0, bit, val);
#else // TCF2 defined
        DoAnalogWriteForPort(&TCF2, bit, val);
#endif // TCF2 defined
        break;


#endif // NUM_DIGITAL_PINS > 30

#elif NUM_DIGITAL_PINS > 18 /* meaning there is a PORT E available but with only 4 pins */

      case TIMERE0:
        // timer E0 counts UP, but a value of 0 would still generate a '0' output because
        // the output STARTS at a 1, and flips to 0 when the CTR reaches the CC register
        // Similarly, a value of 255 would generate a '1'.  see section 12.8.3 in the 'D' manual
        if(bit == 1)
        {
          TCE0_CCA = val; // NOTE:  these are 16-bit registers (but I'm in 8-bit mode so it's fine)
        }
        else if(bit == 2)
        {
          TCE0_CCB = val;
        }
        else if(bit == 4)
        {
          TCE0_CCC = val;
        }
        else if(bit == 8)
        {
          TCE0_CCD = val;
        }

// this is a reminder that the low nybble should be assigned the correct value for single-slope PWM mode
//        TCE0_CTRLB = TC_WGMODE_SS_gc; // single-slope PWM.  NOTE:  this counts UP, whereas the other timers count DOWN

        TCE0_CTRLB |= (bit << 4); // enables output (0-3 only, but that's all PORT E has anyway)
                                  // note that the 'enable' bits are in CTRLB and in upper nybble
        break;


#endif // NUM_DIGITAL_PINS >= 18, 24

#endif // TCC4 check

      case NOT_ON_TIMER:
      default:
        if (val < 128)
        {
          digitalWrite(pin, LOW);
        }
        else
        {
          digitalWrite(pin, HIGH);
        }
    }
  }
}



#ifdef TCC4
void DoAnalogWriteForPort(TC4_t *port, uint8_t bit, uint8_t val)
{
uint8_t modeE, modeF;

  modeE = port->CTRLE;
  modeF = port->CTRLF;

  if(bit == 1)
  {
    port->CCA = (port->CCA & 0xff00) | (val & 0xff);
    modeE = (modeE & ~TC4_LCCAMODE_gm) | TC45_LCCAMODE_COMP_gc;
  }
  else if(bit == 2)
  {
    port->CCB = (port->CCB & 0xff00) | (val & 0xff);
    modeE = (modeE & ~TC4_LCCBMODE_gm) | TC45_LCCBMODE_COMP_gc;
  }
  else if(bit == 4)
  {
    port->CCC = (port->CCC & 0xff00) | (val & 0xff);
    modeE = (modeE & ~TC4_LCCCMODE_gm) | TC45_LCCCMODE_COMP_gc;
  }
  else if(bit == 8)
  {
    port->CCD = (port->CCD & 0xff00) | (val & 0xff);
    modeE = (modeE & ~TC4_LCCDMODE_gm) | TC45_LCCDMODE_COMP_gc;
  }
  else if(bit == 16)
  {
    port->CCA = (port->CCA & 0xff) | ((val << 8) & 0xff00);
    modeF = (modeF & ~TC4_HCCAMODE_gm) | TC45_HCCAMODE_COMP_gc;
  }
  else if(bit == 32)
  {
    port->CCB = (port->CCB & 0xff) | ((val << 8) & 0xff00);
    modeF = (modeF & ~TC4_HCCBMODE_gm) | TC45_HCCBMODE_COMP_gc;
  }
  else if(bit == 64)
  {
    port->CCC = (port->CCC & 0xff) | ((val << 8) & 0xff00);
    modeF = (modeF & ~TC4_HCCCMODE_gm) | TC45_HCCCMODE_COMP_gc;
  }
  else if(bit == 128)
  {
    port->CCD = (port->CCD & 0xff) | ((val << 8) & 0xff00);
    modeF = (modeF & ~TC4_HCCDMODE_gm) | TC45_HCCDMODE_COMP_gc;
  }
  else
  {
    return;
  }

  port->CTRLE = modeE;
  port->CTRLF = modeF;

//  if(bit <= 8)
//  {
//    port->CTRLE = mode;
//  }
//  else
//  {
//    port->CTRLF = mode;
//  }
}
#elif defined(TCC2)
void DoAnalogWriteForPort(TC2_t *port, uint8_t bit, uint8_t val)
{
  // NOTE:  timers C2 and D2 count DOWN, always.  However, the output starts at zero
  //        and flips to 1 when CTR reaches the CMP value.  So a value of 255 would be
  //        '1' and 0 would be '0', as is expected.  See 'D' manual 13.6.2
  if(bit == 1)
  {
    port->LCMPA = val;
  }
  else if(bit == 2)
  {
    port->LCMPB = val;
  }
  else if(bit == 4)
  {
    port->LCMPC = val;
  }
  else if(bit == 8)
  {
    port->LCMPD = val;
  }
  else if(bit == 16)
  {
    port->HCMPA = val;
  }
  else if(bit == 32)
  {
    port->HCMPB = val;
  }
  else if(bit == 64)
  {
    port->HCMPC = val;
  }
  else if(bit == 128)
  {
    port->HCMPD = val;
  }

// this is a reminder that the low nybble should be assigned the correct value for single-slope PWM mode
//        port->CTRLB = TC45_WGMODE_SINGLESLOPE_gc;
  port->CTRLB |= bit; // enables output
}
#else // TCC0
void DoAnalogWriteForPort(TC0_t *port, uint8_t bit, uint8_t val)
{
  // NOTE:  timers C2 and D2 count DOWN, always.  However, the output starts at zero
  //        and flips to 1 when CTR reaches the CMP value.  So a value of 255 would be
  //        '1' and 0 would be '0', as is expected.  See 'D' manual 13.6.2
  if(bit == 1)
  {
    port->CCA = (port->CCA & 0xff00) | (val & 0xff);
  }
  else if(bit == 2)
  {
    port->CCB = (port->CCB & 0xff00) | (val & 0xff);
  }
  else if(bit == 4)
  {
    port->CCC = (port->CCC & 0xff00) | (val & 0xff);
  }
  else if(bit == 8)
  {
    port->CCD = (port->CCD & 0xff00) | (val & 0xff);
  }
  else if(bit == 16)
  {
    port->CCA = (port->CCA & 0xff) | ((val << 8) & 0xff00);
  }
  else if(bit == 32)
  {
    port->CCB = (port->CCB & 0xff) | ((val << 8) & 0xff00);
  }
  else if(bit == 64)
  {
    port->CCC = (port->CCC & 0xff) | ((val << 8) & 0xff00);
  }
  else if(bit == 128)
  {
    port->CCD = (port->CCD & 0xff) | ((val << 8) & 0xff00);
  }

// this is a reminder that the low nybble should be assigned the correct value for single-slope PWM mode
//        port->CTRLB = TC45_WGMODE_SINGLESLOPE_gc;
  port->CTRLB |= bit; // enables output
}
#endif // TCC4, TCC2, TCC0


