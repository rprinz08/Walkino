/*
  WalkDuino - program your Walkera XMEGA based quadcopter receiver board
  with the Arduino IDE.

  MMA-8452Q 3-axis accelerometer example

  2016 richard.prinz@min.at
*/

#include <Wire.h>
#include <MMA8452Q.h>

MMA8452Q accel;

void setup() {
  xmWireC.begin();
  accel.begin(&xmWireC);
}

void loop() {
  if (accel.available()) {
    accel.read();
	
    Serial.print("MMA8452Q X=");
    Serial.print(accel.cx, 5);
    Serial.print("\t Y=");
    Serial.print(accel.cy, 5);
    Serial.print("\t Z=");
    Serial.println(accel.cz, 5);
  }
}
