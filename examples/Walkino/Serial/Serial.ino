/*
  WalkDuino - program your Walkera XMEGA based quadcopter receiver board
  with the Arduino IDE.

  Serial example

  Sends "tick" and "tock" every socond on the main RX2635H serial port
  with 9600 baud, no parity and 1 stop bit.

  2016 richard.prinz@min.at
*/

void setup() {
  Serial.begin(9600);
}

void loop() {
  Serial.println("tick");
  delay(1000);
  Serial.println("tock");
  delay(1000);
}
