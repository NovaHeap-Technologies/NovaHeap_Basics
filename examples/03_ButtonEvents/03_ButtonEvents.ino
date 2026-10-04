/*
  03_ButtonEvents: everything a button can tell you, in the Serial Monitor.

  Press quickly for "clicked", hold for a second for "long press". Bounce is
  filtered out: each press shows once, however scratchy the switch.

  Wiring:  push button between pin 2 and GND  (GPIO 5 on ESP32 and ESP8266)

  NovaHeap_Basics by NovaHeap Technologies
*/

#include <NovaHeap_Basics.h>

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
const int BUTTON_PIN = 5;  // GPIO 4 and 5 are free on every ESP32 and ESP8266 board
#else
const int BUTTON_PIN = 2;
#endif

NHButton button(BUTTON_PIN);

void setup() {
  Serial.begin(9600);
  button.begin();
  // button.setLongPressTime(2000);  // a long press could be 2 s instead
}

void loop() {
  button.update();

  if (button.pressed()) {
    Serial.println("pressed");
  }
  if (button.longPressed()) {
    Serial.println("long press");
  }
  if (button.released()) {
    Serial.println("released");
  }
  if (button.clicked()) {
    Serial.println("clicked");
  }
}
