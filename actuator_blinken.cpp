/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */

// Arduinos unfortunately dont use any kind of #define for the board type so the 
// standard blinkplay fails on for example the WEMOS boards.
// Arduino_UNO Pin 13 has an LED connected on most Arduino boards but doesnt define BUILTIN_LED
// ESP8266 has it on pin 2, but strangely BUILTIN_LED responds as ifndef !
/*
#ifndef BUILTIN_LED
#define BUILTIN_LED 13
#endif
*/

void setup() {                
  // Comment out this line unless debugging port numbers
  /*
  Serial.begin(57600); // Initialize IO port TODO move to somewhere Forth wants it
  Serial.print("BUILTIN_LED="); Serial.print(BUILTIN_LED);
  Serial.print(" INPUT="); Serial.print(INPUT); 
  Serial.print(" OUTPUT="); Serial.print(OUTPUT); 
  Serial.print(" INPUT_PULLUP="); Serial.print(INPUT_PULLUP); 
  Serial.print(" HIGH="); Serial.print(HIGH); 
  Serial.print(" LOW="); Serial.print(LOW); 
  
  Serial.println();
  */
  // initialize the digital pin as an output.
  pinMode(BUILTIN_LED, OUTPUT);
}

void loop() {
  digitalWrite(BUILTIN_LED, HIGH);   // set the LED on
  delay(2000);              // wait for a second
  digitalWrite(BUILTIN_LED, LOW);    // set the LED off
  delay(200);              // wait for a second
}
