// NovaHeap_Basics: timers, a debounced button and a serial command menu, without delay().
// Copyright (c) 2026 NovaHeap Technologies
// Released under the NovaHeap Technologies License (MIT terms). See LICENSE.

#include "NovaHeap_Basics.h"

// Time since `since`, correct across the millis() rollover every 49.7 days. millis() is
// 32-bit on Arduino boards, so the subtraction is done in 32 bits even where
// unsigned long is 64-bit (such as a Linux PC running the unit tests).
static inline uint32_t elapsed(unsigned long now, unsigned long since) {
  return (uint32_t)(now - since);
}

// ---- NHTimer -------------------------------------------------------------------------------

NHTimer::NHTimer(unsigned long intervalMs) : _interval(intervalMs), _last(0), _started(false) {}

bool NHTimer::ready() {
  if (!_started) {
    restart();
    return false;
  }
  unsigned long now = millis();
  if (elapsed(now, _last) < _interval) return false;

  _last += _interval;  // from when it was due, not from now, so the timing doesn't drift
  if (elapsed(now, _last) >= _interval) _last = now;  // over a whole interval behind: skip
  return true;
}

void NHTimer::restart() {
  _last = millis();
  _started = true;
}

// ---- NHTimeout -----------------------------------------------------------------------------

NHTimeout::NHTimeout() : _start(0), _ms(0), _running(false), _ranOut(false) {}

void NHTimeout::start(unsigned long ms) {
  _start = millis();
  _ms = ms;
  _running = true;
  _ranOut = false;
}

void NHTimeout::stop() { _running = false; }

bool NHTimeout::done() {
  if (_running && !isRunning()) {
    _running = false;
    return true;
  }
  return false;
}

bool NHTimeout::isRunning() const {
  if (!_running || _ranOut) return false;
  if (elapsed(millis(), _start) >= _ms) {
    _ranOut = true;
    return false;
  }
  return true;
}

unsigned long NHTimeout::remaining() const {
  if (!isRunning()) return 0;
  return _ms - elapsed(millis(), _start);
}

// ---- NHButton ------------------------------------------------------------------------------

NHButton::NHButton(uint8_t pin, NHButtonWiring wiring)
    : _pin(pin),
      _toVcc(wiring == NH_BUTTON_TO_VCC),
      _begun(false),
      _raw(false),
      _down(false),
      _longDone(false),
      _pressed(false),
      _released(false),
      _clicked(false),
      _longPressed(false),
      _rawSince(0),
      _downSince(0),
      _debounce(20),
      _longPress(1000) {}

bool NHButton::readPin() const {
  bool high = digitalRead(_pin) == HIGH;
  return _toVcc ? high : !high;
}

void NHButton::begin() {
  pinMode(_pin, _toVcc ? INPUT : INPUT_PULLUP);  // to VCC: the 10k resistor pulls it down

  // Wait, briefly, for a steady first reading. A capacitor on the pin, or long wires, can take
  // a few milliseconds to charge through the pull-up, and would otherwise look like a press.
  bool reading = readPin();
  unsigned long steady = 0;
  for (unsigned long waited = 0; steady < _debounce && waited < 2 * _debounce; waited++) {
    delay(1);
    bool next = readPin();
    steady = (next == reading) ? steady + 1 : 0;
    reading = next;
  }

  _begun = true;
  _raw = _down = reading;
  _rawSince = _downSince = millis();
  _longDone = _down;  // already down at start-up: no click or long press for this one
  _pressed = _released = _clicked = _longPressed = false;
}

void NHButton::update() {
  if (!_begun) begin();
  _pressed = _released = _clicked = _longPressed = false;

  unsigned long now = millis();
  bool raw = readPin();
  if (raw != _raw) {
    _raw = raw;
    _rawSince = now;
  }

  // Accept a change once the pin has held its new level for the debounce time: bounce, and
  // electrical noise, never last that long.
  if (_raw != _down && elapsed(now, _rawSince) >= _debounce) {
    _down = _raw;
    if (_down) {
      _pressed = true;
      _downSince = _rawSince;
      _longDone = false;
    } else {
      _released = true;
      _clicked = !_longDone;
    }
  }

  // Only while the pin is still down: not in the debounce time after it has come up.
  if (_down && _raw && !_longDone && elapsed(now, _downSince) >= _longPress) {
    _longDone = true;
    _longPressed = true;
  }
}

unsigned long NHButton::downTime() const { return _down ? elapsed(millis(), _downSince) : 0; }
