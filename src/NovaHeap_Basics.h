// NovaHeap_Basics: timers, a debounced button and a serial command menu, without delay().
// Copyright (c) 2026 NovaHeap Technologies
// Released under the NovaHeap Technologies License (MIT terms). See LICENSE.

#ifndef NOVAHEAP_BASICS_H
#define NOVAHEAP_BASICS_H

#include <Arduino.h>

// ---- NHTimer: something every so often ----------------------------------------------------

// A repeating timer. ready() is true once every interval:
//   NHTimer blink(500);
//   if (blink.ready()) { ... }   // every 500 ms
class NHTimer {
 public:
  explicit NHTimer(unsigned long intervalMs);

  // True once each time the interval passes. The first interval counts from the first call.
  // Intervals stay evenly spaced, but if loop() falls more than a whole interval behind,
  // the missed ones are skipped rather than all fired at once.
  bool ready();

  void restart();  // count the interval from now
  void setInterval(unsigned long ms) { _interval = ms; }
  unsigned long interval() const { return _interval; }

 private:
  unsigned long _interval;
  unsigned long _last;
  bool _started;
};

// ---- NHTimeout: something once, after a while ---------------------------------------------

// A one-shot timer:
//   NHTimeout light;
//   light.start(3000);
//   if (light.done()) { ... }   // once, 3 s later
class NHTimeout {
 public:
  NHTimeout();

  void start(unsigned long ms);  // run out ms from now; starting again restarts the time
  void stop();                   // cancel: done() won't fire

  bool done();                   // true once, the first time it's checked after running out
  bool isRunning() const;        // started, and not yet run out
  unsigned long remaining() const;  // milliseconds left, or 0

 private:
  unsigned long _start;
  unsigned long _ms;
  bool _running;
  // Seen to have run out. Without this, a timeout checked only with isRunning() would seem to
  // start again 49.7 days later, when millis() comes round.
  mutable bool _ranOut;
};

// ---- NHButton: a push button, debounced ---------------------------------------------------

enum NHButtonWiring : uint8_t {
  NH_BUTTON_TO_GND,  // button between the pin and GND; uses the internal pull-up (the default)
  NH_BUTTON_TO_VCC   // button between the pin and 5V/3.3V; needs a 10k resistor from pin to GND
};

class NHButton {
 public:
  explicit NHButton(uint8_t pin, NHButtonWiring wiring = NH_BUTTON_TO_GND);

  void begin();   // call once in setup()
  void update();  // call every loop()

  // Events. Each is true for one update() only: check them in the same loop().
  bool pressed() const { return _pressed; }    // went down
  bool released() const { return _released; }  // came up
  bool clicked() const { return _clicked; }    // came up before the long-press time
  bool longPressed() const { return _longPressed; }  // has been down for the long-press time

  bool isDown() const { return _down; }
  unsigned long downTime() const;  // how long it has been down, in ms; 0 while up

  // A change counts once the pin has stayed at its new level this long. Default 20 ms.
  void setDebounceTime(unsigned long ms) { _debounce = ms; }
  // How long a press must last to be a long press. Default 1000 ms.
  void setLongPressTime(unsigned long ms) { _longPress = ms; }

 private:
  bool readPin() const;

  uint8_t _pin;
  bool _toVcc;
  bool _begun;
  bool _raw;       // the pin's last reading: true = down
  bool _down;      // the debounced state
  bool _longDone;  // this press has been a long press (or began before begin())
  bool _pressed, _released, _clicked, _longPressed;
  unsigned long _rawSince;  // when the pin last changed
  unsigned long _downSince;
  unsigned long _debounce;
  unsigned long _longPress;
};

// ---- NHCommands: a menu of commands typed into the Serial Monitor -------------------------

// Commands are words, optionally followed by arguments: "blink 250".
//   NHCommands commands;
//   void ledOn() { digitalWrite(13, HIGH); }
//   setup:  commands.add("on", ledOn, "switch the LED on");
//   loop:   commands.update();
// "help" lists them. Set the Serial Monitor's line ending to "Newline" (or "Both NL & CR").
class NHCommands {
 public:
  static const uint8_t MAX_COMMANDS = 16;
  static const uint8_t MAX_LINE = 64;  // characters in one line, including arguments
  static const uint8_t MAX_ARGS = 4;

  explicit NHCommands(Stream &port = Serial);

  // Add a command. name is one word, matched without regard to case. help is shown by "help":
  // a plain string, or F("...") to keep it out of RAM on boards that have little. Both are
  // kept as pointers, not copied, so pass text in quotes (or a global or static array).
  // Returns false if the list is full, the name is taken, or name or run is missing.
  bool add(const char *name, void (*run)());
  bool add(const char *name, void (*run)(), const char *help);
  bool add(const char *name, void (*run)(), const __FlashStringHelper *help);

  // Call every loop(). Reads what has arrived and runs a command at the end of each line.
  void update();

  void printHelp();

  // A command's arguments, while it runs.
  uint8_t argCount() const { return _argCount; }
  const char *arg(uint8_t i) const;  // "" if there is no such argument
  bool argIsNumber(uint8_t i) const;
  long argInt(uint8_t i) const;      // 0 if it isn't a number
  float argFloat(uint8_t i) const;   // 0 if it isn't a number

 private:
  struct Command {
    const char *name;
    void (*run)();
    const char *help;
  };

  bool addCommand(const char *name, void (*run)(), const char *help, bool inFlash);
  const Command *find(const char *name) const;
  void runLine();

  Stream &_port;
  Command _commands[MAX_COMMANDS];
  uint16_t _helpInFlash;  // one bit per command
  uint8_t _count;
  char _line[MAX_LINE + 1];
  uint8_t _length;
  bool _tooLong;
  const char *_args[MAX_ARGS];
  uint8_t _argCount;
};

#endif
