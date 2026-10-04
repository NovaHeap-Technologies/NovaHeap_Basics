/*
  04_SerialCommands: control the sketch by typing in the Serial Monitor.

  Set the Serial Monitor to 9600 baud and "Newline", then type:
      help         the list of commands
      on / off     the LED
      blink 250    blink every 250 ms
      status       what the LED is doing

  Each command is an ordinary function. add() connects a word to it.

  Wiring:  pin 8 -> 220 ohm resistor -> LED -> GND  (GPIO 4 on ESP32 and ESP8266)

  NovaHeap_Basics by NovaHeap Technologies
*/

#include <NovaHeap_Basics.h>

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
const int LED_PIN = 4;  // GPIO 4 and 5 are free on every ESP32 and ESP8266 board
#else
const int LED_PIN = 8;
#endif

NHCommands commands;
NHTimer blinkTimer(500);
bool blinking = false;
bool lit = false;

void setLed(bool on) {
  lit = on;
  digitalWrite(LED_PIN, lit ? HIGH : LOW);
}

void ledOn() {
  blinking = false;
  setLed(true);
  Serial.println("LED on");
}

void ledOff() {
  blinking = false;
  setLed(false);
  Serial.println("LED off");
}

void startBlinking() {
  if (commands.argCount() > 0) {
    long ms = commands.argInt(0);
    if (ms < 20 || ms > 10000) {
      Serial.println("Give a time from 20 to 10000 ms, like: blink 250");
      return;
    }
    blinkTimer.setInterval(ms);
  }
  blinking = true;
  Serial.print("Blinking every ");
  Serial.print(blinkTimer.interval());
  Serial.println(" ms");
}

void showStatus() {
  if (blinking) {
    Serial.print("Blinking every ");
    Serial.print(blinkTimer.interval());
    Serial.println(" ms");
  } else {
    Serial.println(lit ? "LED on" : "LED off");
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);

  commands.add("on", ledOn, F("switch the LED on"));
  commands.add("off", ledOff, F("switch the LED off"));
  commands.add("blink", startBlinking, F("blink [ms]: blink, every ms milliseconds"));
  commands.add("status", showStatus, F("what the LED is doing"));

  Serial.println("Type help for the commands.");
}

void loop() {
  commands.update();

  if (blinking && blinkTimer.ready()) {
    setLed(!lit);
  }
}
