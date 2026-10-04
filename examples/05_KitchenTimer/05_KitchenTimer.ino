/*
  05_KitchenTimer: a countdown timer, using everything in this library.

  Set the time in the Serial Monitor (9600 baud, "Newline"), and run it from there or
  with the button:
      set 90       90 seconds
      start        start, or carry on after a pause
      pause
      reset
  Button: click to start or pause, hold for a second to reset. When the time is up,
  the LED flashes until you click.

  Wiring:  push button between pin 2 and GND      (GPIO 5 on ESP32 and ESP8266)
           pin 8 -> 220 ohm resistor -> LED -> GND  (GPIO 4 on ESP32 and ESP8266)

  NovaHeap_Basics by NovaHeap Technologies
*/

#include <NovaHeap_Basics.h>

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
const int BUTTON_PIN = 5;  // GPIO 4 and 5 are free on every ESP32 and ESP8266 board
const int LED_PIN = 4;
#else
const int BUTTON_PIN = 2;
const int LED_PIN = 8;
#endif

const unsigned long PRINT_EVERY_MS = 1000;
const unsigned long FLASH_MS = 150;

const int STOPPED = 0;  // the timer's four states
const int RUNNING = 1;
const int PAUSED = 2;
const int RINGING = 3;
int state = STOPPED;

unsigned long timerSeconds = 60;  // the time it's set to
unsigned long pausedLeft = 0;     // ms left when paused
bool lit = false;

NHButton button(BUTTON_PIN);
NHCommands commands;
NHTimeout countdown;
NHTimer printTimer(PRINT_EVERY_MS);
NHTimer flashTimer(FLASH_MS);

void setLed(bool on) {
  lit = on;
  digitalWrite(LED_PIN, lit ? HIGH : LOW);
}

// Prints a time as minutes and seconds, such as 1:05.
void printTime(unsigned long ms) {
  unsigned long s = (ms + 999) / 1000;  // round up: "0:01" until it's really over
  Serial.print(s / 60);
  Serial.print(s % 60 < 10 ? ":0" : ":");
  Serial.println(s % 60);
}

void startTimer() {
  if (state == RUNNING) {
    Serial.println("Already running");
    return;
  }
  countdown.start(state == PAUSED ? pausedLeft : timerSeconds * 1000);
  printTimer.restart();
  state = RUNNING;
  setLed(false);
  Serial.print("Started: ");
  printTime(countdown.remaining());
}

void pauseTimer() {
  if (state != RUNNING) {
    Serial.println("Not running");
    return;
  }
  pausedLeft = countdown.remaining();
  countdown.stop();
  state = PAUSED;
  Serial.print("Paused at ");
  printTime(pausedLeft);
}

void resetTimer() {
  countdown.stop();
  state = STOPPED;
  setLed(false);
  Serial.print("Reset to ");
  printTime(timerSeconds * 1000);
}

void setTime() {
  long s = commands.argInt(0);
  if (commands.argCount() != 1 || s < 1 || s > 5999) {
    Serial.println("Give the seconds, from 1 to 5999, like: set 90");
    return;
  }
  timerSeconds = s;
  resetTimer();
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  button.begin();

  commands.add("set", setTime, F("set <seconds>"));
  commands.add("start", startTimer, F("start, or carry on after a pause"));
  commands.add("pause", pauseTimer);
  commands.add("reset", resetTimer);

  Serial.println("Kitchen timer. Type help for the commands.");
}

void loop() {
  button.update();
  commands.update();

  if (button.clicked()) {
    if (state == RUNNING) {
      pauseTimer();
    } else if (state == RINGING) {
      resetTimer();
    } else {
      startTimer();
    }
  }
  if (button.longPressed()) {
    resetTimer();
  }

  if (state == RUNNING) {
    if (countdown.done()) {
      state = RINGING;
      Serial.println("Time's up!");
    } else if (printTimer.ready()) {
      printTime(countdown.remaining());
    }
  }

  if (state == RINGING && flashTimer.ready()) {
    setLed(!lit);
  }
}
