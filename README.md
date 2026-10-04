# NovaHeap_Basics

Timers, a debounced button and a Serial Monitor command menu for Arduino, all without `delay()`, by NovaHeap Technologies.

[![CI](https://github.com/NovaHeap-Technologies/NovaHeap_Basics/actions/workflows/ci.yml/badge.svg)](https://github.com/NovaHeap-Technologies/NovaHeap_Basics/actions/workflows/ci.yml)

Once a sketch has to do two things at once, `delay()` stops working: while it waits, nothing else happens. The fix is `millis()`, a few variables for each timer, and debounce code for each button. It works, but it's easy to get subtly wrong. NovaHeap_Basics is that code, written once and tested.

- **`NHTimer`**: do something every so often. It doesn't drift, and if `loop()` falls behind it skips the missed beats rather than firing them all at once.
- **`NHTimeout`**: do something once, after a while. Start it, stop it, see how long is left.
- **`NHButton`**: a push button that reports each press once, however much the switch bounces. It tells a click from a long press.
- **`NHCommands`**: type `blink 250` in the Serial Monitor to run your own `blink()` function, with arguments and a built-in `help` list.
- **Safe across the `millis()` rollover** every 49.7 days. No dynamic memory. Runs on any board.

## Install

**Arduino IDE:** Tools → Manage Libraries, search for **NovaHeap Basics**, click Install.

**Manually:** download this repository as a ZIP, then Sketch → Include Library → Add .ZIP Library.

## Quick start

```cpp
#include <NovaHeap_Basics.h>

NHTimer blinkTimer(500);   // every 500 ms
NHButton button(2);        // button between pin 2 and GND
NHTimeout light;
bool blinkOn = false;

void setup() {
  pinMode(8, OUTPUT);        // LEDs, each with a 220 ohm resistor, on pins 8 and 9
  pinMode(9, OUTPUT);
  button.begin();
}

void loop() {
  button.update();

  if (blinkTimer.ready()) {         // pin 9 blinks all along...
    blinkOn = !blinkOn;
    digitalWrite(9, blinkOn ? HIGH : LOW);
  }
  if (button.pressed()) {           // ...while a press lights pin 8 for 3 seconds
    light.start(3000);
  }
  digitalWrite(8, light.isRunning() ? HIGH : LOW);
}
```

## Examples

| Example | Shows |
|---|---|
| `01_TwoBlinkRates` | Two LEDs blinking at their own speeds with `NHTimer` |
| `02_StairLight` | A press lights an LED for 10 s, a long press turns it off: `NHButton` and `NHTimeout` |
| `03_ButtonEvents` | Every button event, printed |
| `04_SerialCommands` | `on`, `off`, `blink 250` and `status` typed in the Serial Monitor |
| `05_KitchenTimer` | A countdown timer using all four |

On ESP32 and ESP8266 the examples use GPIO 4 and 5, which are free on every ESP32 (including C3, S2 and S3) and ESP8266 board.

## Reference

### NHTimer

| Call | Does |
|---|---|
| `NHTimer t(ms)` | A timer that repeats every `ms` |
| `t.ready()` | `true` once each time the interval passes. The first interval counts from the first call |
| `t.restart()` | Count the interval from now |
| `t.setInterval(ms)` / `t.interval()` | Change or read the interval |

### NHTimeout

| Call | Does |
|---|---|
| `NHTimeout t` | A one-shot timer, not running |
| `t.start(ms)` | Run out `ms` from now. Starting again restarts the time |
| `t.stop()` | Cancel it. `done()` won't fire |
| `t.done()` | `true` once, the first time it's checked after running out |
| `t.isRunning()` | `true` while counting down |
| `t.remaining()` | Milliseconds left, or 0 |

### NHButton

| Call | Does |
|---|---|
| `NHButton b(pin)` | A button between `pin` and GND. Uses the internal pull-up, so no resistor |
| `NHButton b(pin, NH_BUTTON_TO_VCC)` | A button between `pin` and 5V/3.3V. Needs 10 kΩ from the pin to GND |
| `b.begin()` | Set up the pin and read its starting state. Call in `setup()`; it takes about 20 ms |
| `b.update()` | Call every `loop()` |
| `b.pressed()` / `b.released()` | `true` for one `update()` when the button goes down or comes up |
| `b.clicked()` | `true` for one `update()` when it comes up before the long-press time |
| `b.longPressed()` | `true` for one `update()` once it has been down for the long-press time |
| `b.isDown()` | `true` while it's down |
| `b.downTime()` | How long it has been down, in ms; 0 while it's up |
| `b.setDebounceTime(ms)` | Default 20 ms |
| `b.setLongPressTime(ms)` | Default 1000 ms |

How it behaves:

- A change counts once the pin has stayed at its new level for the debounce time, so neither bounce nor electrical noise can fake a press. The press is reported 20 ms after it happens.
- `NH_BUTTON_TO_VCC` always needs the 10 kΩ pull-down resistor, even on boards that have internal pull-downs, so a sketch behaves the same on every board.
- A long press is not also a click. Each press gives `pressed()`, then either `longPressed()` or, on release, `clicked()`; `released()` comes either way.
- A button already held down when `begin()` runs reads as down (`isDown()`), so `setup()` can check it. It doesn't count as a press, a click or a long press.

### NHCommands

| Call | Does |
|---|---|
| `NHCommands c` | A command menu on `Serial`. `NHCommands c(Serial1)` uses another port |
| `c.add("name", function, "help text")` | Typing `name` runs `function()`. The help text is optional; `F("...")` keeps it out of RAM. Both are kept as pointers, not copied, so pass text in quotes. Returns `false` if the list is full (16 commands), the name is taken, empty or more than one word, or the function is missing |
| `c.update()` | Call every `loop()` |
| `c.argCount()` | How many words followed the command (up to 4) |
| `c.arg(i)` | Argument `i` (from 0) as text, or `""` |
| `c.argIsNumber(i)` | `true` if it's a number like `12`, `-3` or `2.5` |
| `c.argInt(i)` / `c.argFloat(i)` | Argument `i` as a number, or 0 if it isn't one |
| `c.printHelp()` | Print the list of commands |

How it behaves:

- Set the Serial Monitor's line ending to **Newline** (or **Both NL & CR**). A command runs when its line ends.
- Commands match whatever the case: `ON`, `On` and `on` are the same.
- Each line is shown back as `> blink 250`, so replies read next to it.
- `help` (or `?`) lists the commands. If you add your own `help`, both run yours instead. Unknown words get a short message.
- Lines are up to 64 characters. Backspace works in terminal programs such as PuTTY.

## Tests

Every class has unit tests that run on a PC against a fake Arduino:

```
make -C extras/test
```

GitHub Actions runs them on every push, lints the library, and compiles the examples for AVR, Uno R4, SAMD, ESP32, ESP8266 and RP2040 boards.

## License

NovaHeap Technologies License (MIT terms): free for personal and commercial use, as long as the copyright notice stays in. See [LICENSE](LICENSE).

---

Made by **NovaHeap Technologies**. Pairs with [NovaHeap_Relay](https://github.com/NovaHeap-Technologies/NovaHeap_Relay), [NovaHeap_BatteryMonitor](https://github.com/NovaHeap-Technologies/NovaHeap_BatteryMonitor) and [NovaHeap_NTC](https://github.com/NovaHeap-Technologies/NovaHeap_NTC).
