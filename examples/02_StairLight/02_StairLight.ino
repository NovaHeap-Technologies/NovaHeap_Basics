/*
  02_StairLight: a press turns the light on for 10 seconds.

  Like the timed light on a staircase. Pressing again while it's on starts the
  10 seconds over. A long press (1 second) turns it off straight away.

  Wiring:  push button between pin 2 and GND      (GPIO 5 on ESP32 and ESP8266)
           pin 8 -> 220 ohm resistor -> LED -> GND  (GPIO 4 on ESP32 and ESP8266)

  To switch a real lamp, drive a relay module instead of the LED, with NovaHeap_Relay.

  NovaHeap_Basics by NovaHeap Technologies
*/

#include <NovaHeap_Basics.h>

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
const int BUTTON_PIN = 5;  // GPIO 4 and 5 are free on every ESP32 and ESP8266 board
const int LIGHT_PIN = 4;
#else
const int BUTTON_PIN = 2;
const int LIGHT_PIN = 8;
#endif

const unsigned long ON_MS = 10000;

NHButton button(BUTTON_PIN);
NHTimeout light;

void setup() {
  pinMode(LIGHT_PIN, OUTPUT);
  button.begin();
}

void loop() {
  button.update();

  if (button.pressed()) {
    light.start(ON_MS);  // on for 10 s, or 10 s more
  }
  if (button.longPressed()) {
    light.stop();        // off now
  }

  digitalWrite(LIGHT_PIN, light.isRunning() ? HIGH : LOW);
}
