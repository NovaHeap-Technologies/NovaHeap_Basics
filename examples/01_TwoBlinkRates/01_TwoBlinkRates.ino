/*
  01_TwoBlinkRates: two LEDs blinking at their own speeds, without delay().

  delay() stops the whole sketch, so two delay() blinks can't keep their own time.
  An NHTimer just answers "is it time yet?", and loop() keeps running.

  Wiring:  pin 8 -> 220 ohm resistor -> LED -> GND   (GPIO 4 on ESP32 and ESP8266)
           pin 9 -> 220 ohm resistor -> LED -> GND   (GPIO 5 on ESP32 and ESP8266)

  NovaHeap_Basics by NovaHeap Technologies
*/

#include <NovaHeap_Basics.h>

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
const int FAST_LED = 4;  // GPIO 4 and 5 are free on every ESP32 and ESP8266 board
const int SLOW_LED = 5;
#else
const int FAST_LED = 8;
const int SLOW_LED = 9;
#endif

const unsigned long FAST_MS = 200;
const unsigned long SLOW_MS = 700;

NHTimer fastTimer(FAST_MS);
NHTimer slowTimer(SLOW_MS);
bool fastOn = false;
bool slowOn = false;

void setup() {
  pinMode(FAST_LED, OUTPUT);
  pinMode(SLOW_LED, OUTPUT);
}

void loop() {
  if (fastTimer.ready()) {
    fastOn = !fastOn;  // flip the LED
    digitalWrite(FAST_LED, fastOn ? HIGH : LOW);
  }
  if (slowTimer.ready()) {
    slowOn = !slowOn;
    digitalWrite(SLOW_LED, slowOn ? HIGH : LOW);
  }
}
