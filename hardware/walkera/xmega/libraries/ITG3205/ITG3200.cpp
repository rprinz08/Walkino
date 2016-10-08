/*
 *   itg3200.cpp
 *
 *   Copyright (C) 2010  Ricardo Arturo Cabral <ing dot cabral dot mejia at gmail dot com>. All rights reserved.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  See <license.txt>, if not, see 
 *   <http://www.gnu.org/licenses/>.
 *
 *   Modified for WalkDuino XMega (Walkera RC receiver boards) 10.2015
 *   by <richard dot prinz at min dot at> 
 */
#include <Wire.h>
#include "ITG3200.h"


ITG3200::ITG3200(byte address)
{
  _gyro_address = address;
};

#ifdef __AVR_XMEGA__
void ITG3200::begin(xmWire *Wire)
{
  this->wire = Wire;
#else
void ITG3200::begin()
{
  this->wire = &Wire;
#endif
  this->wire->begin();

  // Power Management
  write(GYRO_REG_PWR_MGM, 0);
  // Sample rate divider
  write(GYRO_REG_SMPLRT_DIV, 0xFF);
  // Frequency select and digital low pass filter
  write(GYRO_REG_DLPF_FS, 0x1E);
  // Interrupt configuration
  write(GYRO_REG_INT_CFG, 0);
}

#ifdef __AVR_XMEGA__
void ITG3200::begin(xmWire *Wire, byte pwr_mgm, byte fs_lpf, byte smplrt_div, byte int_cfg)
{
  this->wire = Wire;
#else
void ITG3200::begin(byte pwr_mgm, byte fs_lpf, byte smplrt_div, byte int_cfg)
{
  this->wire = &Wire;
#endif	
  this->wire->begin();

  // Power Management
  write(GYRO_REG_PWR_MGM, pwr_mgm);
  
  // Sample rate divider
  //_smplrt_div = smplrt_div;
  write(GYRO_REG_SMPLRT_DIV, smplrt_div);
  
  // Frequency select and digital low pass filter
  //_fs_lpf = fs_lpf;
  write(GYRO_REG_DLPF_FS, 0x1F & fs_lpf);
  
  // Interrupt configuration
  //_int_cfg = int_cfg;
  write(GYRO_REG_INT_CFG, 0xF5 & int_cfg);
}

float ITG3200::getX() {
  return (float)(read(GYRO_REG_X_L) | read(GYRO_REG_X_H)<<8)/GYRO_SENSITIVITY; 
}

float ITG3200::getY() {
  return (float)(read(GYRO_REG_Y_L) | read(GYRO_REG_Y_H)<<8)/GYRO_SENSITIVITY; 
}

float ITG3200::getZ() {
  return (float)(read(GYRO_REG_Z_L) | read(GYRO_REG_Z_H)<<8)/GYRO_SENSITIVITY; 
}

float ITG3200::getTemperature(){
  return (((float)((read(GYRO_REG_TEMP_L) | read(GYRO_REG_TEMP_H)<<8) + GYRO_TEMP_OFFSET))/GYRO_TEMP_SENSITIVITY) + GYRO_TEMP_OFFSET_CELSIUS;
}

void ITG3200::reset() {
  write(GYRO_REG_PWR_MGM, GYRO_RESET);
}

void ITG3200::sleep() {
  byte t = read(GYRO_REG_PWR_MGM);
  write(GYRO_REG_PWR_MGM, t | GYRO_SLEEP);
}

void ITG3200::wake(){
  byte t = read(GYRO_REG_PWR_MGM);
  write(GYRO_REG_PWR_MGM, t & ~GYRO_SLEEP);
}

void ITG3200::standBy(byte axis) {
  byte t = read(GYRO_REG_PWR_MGM);
  write(GYRO_REG_PWR_MGM, t & ~axis);
}

byte ITG3200::getAddress()
{
  return read(GYRO_REG_WHOAMI);
}

void ITG3200::setAddress(byte newAddress)
{
  write(GYRO_REG_WHOAMI, newAddress);
}

void ITG3200::setInterruptConfig(byte config)
{
  // bit 3 and 1 must be zero
  write(GYRO_REG_INT_CFG, 0xF5 & config);
}

bool ITG3200::isInterruptRawDataReady()
{
  byte result = read(GYRO_REG_INT_STS);
  return (result & GYRO_INT_DATA) == GYRO_INT_DATA;
}

bool ITG3200::isInterruptReady()
{
  byte result = read(GYRO_REG_INT_STS);
  return (result & GYRO_INT_READY) == GYRO_INT_READY;
}

byte ITG3200::getInterruptConfig()
{
  return read(GYRO_REG_INT_CFG);
}

void ITG3200::setClockSource(byte clockSource)
{
  if (clockSource >= 6) // 6 and 7 are reserved
    return;
  write(GYRO_REG_PWR_MGM, 0xF8 & clockSource);
}

void ITG3200::write(byte reg, byte val) {
  this->wire->beginTransmission(_gyro_address);
  this->wire->write(reg);
  this->wire->write(val);
  this->wire->endTransmission();
}

byte ITG3200::read(byte reg) {
  this->wire->beginTransmission(_gyro_address);
  this->wire->write(reg);
  this->wire->endTransmission();
  this->wire->beginTransmission(_gyro_address);
  this->wire->requestFrom(_gyro_address,  1);
  // block unil data is available
  while(this->wire->available() ==0) 
    ;
  byte buf = this->wire->receive();
  this->wire->endTransmission();
  return buf;
}
