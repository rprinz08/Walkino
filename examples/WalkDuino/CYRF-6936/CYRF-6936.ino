/*
  WalkDuino - program your Walkera XMEGA based quadcopter receiver board
  with the Arduino IDE.

  RX2635H CYRF-6936 Wireless USB example

  Trys to identify if CYRF-6936 is present.

  2016 richard.prinz@min.at
*/

#include <SPI.h>

// SPI SS - Slave Select
#define C3    11

void setup() {
  pinMode(C3, OUTPUT);
  digitalWrite(C3, HIGH);
  
  SPI.begin();  
  Serial.begin(9600);
}

void loop() {
  // according to documentation register 2 (hex 0x02) always
  // returns ....
  int r = readSPIregister(0x02, 0);
  Serial.print("Register 0x02: 0x");
  Serial.println(r, HEX);
  Serial.println("  (");
  Serial.println(r, BIN);
  Serial.println(")");
  
  delay(5000);
}

// Read a byte from the given register (reg) via SPI. If
// (readWord) is 1 then read a word (2 byte) from the given
// register (reg)
unsigned int readSPIregister(byte reg, byte readWord) {
  unsigned int result = 0;

  // bit 7 = 0 means read mode
  // bit 6 = 1 means auto increment address pointer
  reg = (reg & 0b00111111) | 0b01000000;    
  
  // take the chip select low to select the device
  digitalWrite(C3, LOW);
  
  // send the device the register you want to read
  SPI.transfer(reg);
  
  // send a value of 0 to read the first byte returned
  result = SPI.transfer(0x00);

  // if we want to read a word then read second byte
  if(readWord)
    result = (result << 8) | SPI.transfer(0x00);
  
  // take the chip select high to release chip
  digitalWrite(C3, HIGH);
  
  // return the result:
  return (result);
}
