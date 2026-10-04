# Changelog

## 1.0.0

First release.

- `NHTimer`: repeating timer that doesn't drift, and skips rather than bursts when loop() falls behind
- `NHTimeout`: one-shot timer with `start()`, `stop()`, `done()`, `isRunning()` and `remaining()`
- `NHButton`: debounced button, to GND or to VCC, with `pressed()`, `released()`, `clicked()`, `longPressed()`, `isDown()` and `downTime()`
- `NHCommands`: Serial Monitor command menu with arguments, number checks and a built-in `help`
- All safe across the millis() rollover; no dynamic memory
- Five examples, and unit tests that run on a PC
