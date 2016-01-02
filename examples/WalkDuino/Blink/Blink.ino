/*
  WalkDuino - program your Walkera XMEGA based quadcopter receiver board
  with the Arduino IDE.

  Blink example

  Turns on the on board LED on for one second, then off for one second,
  repeatedly.

  2016 richard.prinz@min.at
 */

// on the RX2635H receiver the LED is on pin 4
#define LED     4

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin 4 as an output.
  pinMode(LED, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(3000);               // wait for a second
  digitalWrite(LED, LOW);    // turn the LED off by making the voltage LOW
  delay(3000);               // wait for a second
}
