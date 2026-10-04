// Unit tests for NovaHeap_Basics, run on a PC against a fake Arduino.
// Build and run: make -C extras/test

#include <stdio.h>
#include <string.h>

#include "NovaHeap_Basics.h"

// ---- Fake hardware ---------------------------------------------------------

static const int PINS = 32;
static unsigned long fakeNow;
static int level[PINS];                 // what digitalRead() returns...
static unsigned long lowUntil[PINS];    // ...except LOW before this time (a charging capacitor)
static uint8_t mode[PINS];
Stream Serial;

unsigned long millis() { return fakeNow; }
void delay(unsigned long ms) { fakeNow += ms; }
void pinMode(uint8_t pin, uint8_t m) { mode[pin] = m; }
int digitalRead(uint8_t pin) { return fakeNow < lowUntil[pin] ? LOW : level[pin]; }

static void reset(unsigned long now = 0) {
  fakeNow = now;
  for (int p = 0; p < PINS; p++) {
    level[p] = HIGH;  // buttons to GND read HIGH when up
    lowUntil[p] = 0;
    mode[p] = 0xFF;
  }
  Serial = Stream();
}

static void at(unsigned long now) { fakeNow = now; }

// ---- Test runner -----------------------------------------------------------

static int checks, failures;

#define CHECK(cond)                                                   \
  do {                                                                \
    checks++;                                                         \
    if (!(cond)) {                                                    \
      failures++;                                                     \
      printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);          \
    }                                                                 \
  } while (0)

static bool contains(const std::string &text, const char *part) {
  return text.find(part) != std::string::npos;
}

// ---- NHTimer ---------------------------------------------------------------

static void timerCountsFromFirstCall() {
  reset(1000);
  NHTimer t(500);
  CHECK(!t.ready());  // starts counting here
  at(1499);
  CHECK(!t.ready());
  at(1500);
  CHECK(t.ready());
  CHECK(!t.ready());
  at(2000);
  CHECK(t.ready());
}

static void timerDoesNotDrift() {
  reset();
  NHTimer t(500);
  t.ready();
  at(510);
  CHECK(t.ready());
  at(999);
  CHECK(!t.ready());  // due at 1000, not 1010
  at(1003);
  CHECK(t.ready());
  at(1500);
  CHECK(t.ready());
}

static void timerSkipsWhenFarBehind() {
  reset();
  NHTimer t(500);
  t.ready();
  at(2600);  // five intervals late
  CHECK(t.ready());
  CHECK(!t.ready());  // no burst of the missed ones
  at(3099);
  CHECK(!t.ready());
  at(3100);
  CHECK(t.ready());
}

static void timerRestartAndSetInterval() {
  reset();
  NHTimer t(500);
  t.ready();
  at(400);
  t.restart();
  at(800);
  CHECK(!t.ready());
  at(900);
  CHECK(t.ready());
  t.setInterval(100);
  CHECK(t.interval() == 100);
  at(1000);
  CHECK(t.ready());
}

static void timerZeroIntervalFiresEveryCall() {
  reset();
  NHTimer t(0);
  CHECK(!t.ready());
  CHECK(t.ready());
  CHECK(t.ready());
}

static void timerAcrossRollover() {
  reset(0xFFFFFF00UL);
  NHTimer t(0x200);
  t.ready();
  at(0xFF);  // 0x1FF later, after millis() wrapped
  CHECK(!t.ready());
  at(0x100);
  CHECK(t.ready());
}

// ---- NHTimeout -------------------------------------------------------------

static void timeoutIdleUntilStarted() {
  reset();
  NHTimeout t;
  CHECK(!t.done() && !t.isRunning() && t.remaining() == 0);
}

static void timeoutRunsOutOnce() {
  reset(100);
  NHTimeout t;
  t.start(3000);
  CHECK(t.isRunning());
  at(1100);
  CHECK(t.remaining() == 2000);
  CHECK(!t.done());
  at(3099);
  CHECK(!t.done() && t.isRunning());
  at(3100);
  CHECK(!t.isRunning());  // run out, though done() hasn't been checked yet
  CHECK(t.done());
  CHECK(!t.done());
  CHECK(t.remaining() == 0);
}

static void timeoutStartAgainRestarts() {
  reset();
  NHTimeout t;
  t.start(1000);
  at(900);
  t.start(1000);
  at(1000);
  CHECK(!t.done());
  at(1900);
  CHECK(t.done());
}

static void timeoutStopCancels() {
  reset();
  NHTimeout t;
  t.start(1000);
  t.stop();
  at(2000);
  CHECK(!t.done() && !t.isRunning());
}

static void timeoutZeroIsDoneAtOnce() {
  reset();
  NHTimeout t;
  t.start(0);
  CHECK(t.done());
}

static void timeoutStaysOverWithoutDone() {
  // Checked only with isRunning(), never done(): it must not seem to start again when
  // millis() comes round, 2^32 ms later.
  reset();
  NHTimeout t;
  t.start(1000);
  at(2000);
  CHECK(!t.isRunning());
  at(500);  // millis() has wrapped: 2^32 + 500 ms after start
  CHECK(!t.isRunning() && t.remaining() == 0);
  CHECK(t.done());  // still reported, once
  CHECK(!t.done());
}

static void timeoutAcrossRollover() {
  reset(0xFFFFFF00UL);
  NHTimeout t;
  t.start(0x200);
  at(0xFF);
  CHECK(!t.done());
  at(0x100);
  CHECK(t.done());
}

// ---- NHButton --------------------------------------------------------------

struct Events {
  int pressed, released, clicked, longPressed;
};

// Calls update() every millisecond for `ms`, counting events.
static Events runFor(NHButton &b, unsigned long ms) {
  Events e = {0, 0, 0, 0};
  for (unsigned long i = 0; i < ms; i++) {
    fakeNow++;
    b.update();
    e.pressed += b.pressed();
    e.released += b.released();
    e.clicked += b.clicked();
    e.longPressed += b.longPressed();
  }
  return e;
}

// A press or release that bounces for 5 ms before settling at `settle`.
static void bounceTo(NHButton &b, int pin, int settle) {
  for (int i = 0; i < 5; i++) {
    level[pin] = (i % 2) ? settle : !settle;
    runFor(b, 1);
  }
  level[pin] = settle;
}

static void buttonSetsPullUpAndStartsUp() {
  reset();
  NHButton b(2);
  b.begin();
  CHECK(mode[2] == INPUT_PULLUP);
  CHECK(!b.isDown() && b.downTime() == 0);
  CHECK(fakeNow >= 20 && fakeNow <= 40);  // begin() waits for a steady reading
}

static void bouncyPressIsOnePress() {
  reset();
  NHButton b(2);
  b.begin();
  bounceTo(b, 2, LOW);
  Events e = runFor(b, 100);
  CHECK(e.pressed == 1 && e.released == 0 && e.clicked == 0);
  CHECK(b.isDown());

  bounceTo(b, 2, HIGH);
  e = runFor(b, 100);
  CHECK(e.pressed == 0 && e.released == 1 && e.clicked == 1 && e.longPressed == 0);
  CHECK(!b.isDown());
}

static void pressIsReportedAfterTheDebounceTime() {
  reset();
  NHButton b(2);
  b.begin();
  level[2] = LOW;  // first seen by the update() 1 ms later
  Events e = runFor(b, 20);
  CHECK(e.pressed == 0 && !b.isDown());
  e = runFor(b, 1);
  CHECK(e.pressed == 1 && b.isDown());
  CHECK(b.downTime() == 20);  // counted from when the pin was seen to go down
}

static void eventsLastOneUpdate() {
  reset();
  NHButton b(2);
  b.begin();
  level[2] = LOW;
  runFor(b, 21);
  CHECK(b.pressed());
  b.update();
  CHECK(!b.pressed() && b.isDown());
}

static void noiseShorterThanDebounceIsIgnored() {
  reset();
  NHButton b(2);
  b.begin();
  level[2] = LOW;
  runFor(b, 15);
  level[2] = HIGH;
  Events e = runFor(b, 100);
  CHECK(e.pressed == 0 && e.released == 0);
}

static void longPressIsNotAClick() {
  reset();
  NHButton b(2);
  b.begin();
  level[2] = LOW;
  Events e = runFor(b, 1000);
  CHECK(e.pressed == 1 && e.longPressed == 0);
  e = runFor(b, 1);
  CHECK(e.longPressed == 1);  // 1000 ms after the pin was seen to go down
  e = runFor(b, 3000);
  CHECK(e.longPressed == 0);  // once per press
  level[2] = HIGH;
  e = runFor(b, 100);
  CHECK(e.released == 1 && e.clicked == 0);

  level[2] = LOW;  // the next short press is a click again
  runFor(b, 100);
  level[2] = HIGH;
  e = runFor(b, 100);
  CHECK(e.clicked == 1);
}

static void releaseJustBeforeLongPressIsAClick() {
  // Released at 990 ms: the long-press time passes during the release's debounce, but the
  // button is already up.
  reset();
  NHButton b(2);
  b.begin();
  level[2] = LOW;
  Events e = runFor(b, 991);
  CHECK(e.pressed == 1);
  level[2] = HIGH;
  e = runFor(b, 100);
  CHECK(e.longPressed == 0 && e.released == 1 && e.clicked == 1);
}

static void heldAtStartUpIsNotAPress() {
  reset();
  level[2] = LOW;
  NHButton b(2);
  b.begin();
  CHECK(b.isDown());  // so setup() can check it
  Events e = runFor(b, 2000);
  CHECK(e.pressed == 0 && e.longPressed == 0);
  level[2] = HIGH;
  e = runFor(b, 100);
  CHECK(e.released == 1 && e.clicked == 0);
}

static void chargingCapacitorAtStartUpIsNotAPress() {
  reset();
  lowUntil[2] = 4;  // reads LOW for 4 ms while the pull-up charges a capacitor
  NHButton b(2);
  b.begin();
  CHECK(!b.isDown());
  Events e = runFor(b, 100);
  CHECK(e.pressed == 0 && e.released == 0);
}

static void buttonToVcc() {
  reset();
  level[3] = LOW;  // up: the pull-down holds it LOW
  NHButton b(3, NH_BUTTON_TO_VCC);
  b.begin();
  CHECK(mode[3] == INPUT);
  CHECK(!b.isDown());
  level[3] = HIGH;
  Events e = runFor(b, 50);
  CHECK(e.pressed == 1 && b.isDown());
}

static void customTimes() {
  reset();
  NHButton b(2);
  b.setDebounceTime(0);
  b.setLongPressTime(300);
  b.begin();
  level[2] = LOW;
  Events e = runFor(b, 1);
  CHECK(e.pressed == 1);
  e = runFor(b, 300);
  CHECK(e.longPressed == 1);
}

static void worksWithoutBegin() {
  reset();
  NHButton b(2);
  b.update();
  CHECK(mode[2] == INPUT_PULLUP && !b.isDown());
  level[2] = LOW;
  Events e = runFor(b, 50);
  CHECK(e.pressed == 1);
}

static void buttonAcrossRollover() {
  reset(0xFFFFFF00UL);
  NHButton b(2);
  b.begin();
  level[2] = LOW;
  Events e = runFor(b, 0x200);  // through the wrap
  CHECK(e.pressed == 1 && e.longPressed == 0);
  CHECK(b.downTime() == 0x1FF);
  e = runFor(b, 1001 - 0x200);
  CHECK(e.longPressed == 1);
}

// ---- NHCommands ------------------------------------------------------------

static int ledOnRuns, blinkRuns;
static long blinkMs;
static float blinkDuty;
static bool blinkHadNumber;
static std::string firstArg, missingArg;
static int argsSeen;

static void ledOn() { ledOnRuns++; }

static NHCommands *current;  // the menu under test, for the handlers' argument calls

static void blink() {
  blinkRuns++;
  argsSeen = current->argCount();
  blinkHadNumber = current->argIsNumber(0);
  blinkMs = current->argInt(0);
  blinkDuty = current->argFloat(1);
  firstArg = current->arg(0);
  missingArg = current->arg(7);
}

static void myHelp() { Serial.print("my own help"); }

static void resetHandlers() {
  ledOnRuns = blinkRuns = argsSeen = 0;
  blinkMs = 0;
  blinkDuty = 0;
  blinkHadNumber = false;
  firstArg = missingArg = "x";
}

static void commandsRunAndEcho() {
  reset();
  resetHandlers();
  NHCommands c;
  current = &c;
  CHECK(c.add("on", ledOn, "switch the LED on"));
  Serial.type("on\n");
  c.update();
  CHECK(ledOnRuns == 1);
  CHECK(contains(Serial.out, "> on"));
  CHECK(c.argCount() == 0);
}

static void commandsIgnoreCaseAndLineEndings() {
  reset();
  resetHandlers();
  NHCommands c;
  c.add("on", ledOn);
  Serial.type("ON\r\n");  // Both NL & CR: one command, not one plus an empty line
  c.update();
  Serial.type("On\r");
  c.update();
  Serial.type("\n\n   \n");  // empty lines do nothing
  c.update();
  CHECK(ledOnRuns == 2);
  CHECK(Serial.out == "> ON\r\n> On\r\n");  // nothing for the empty lines
}

static void commandsGetArguments() {
  reset();
  resetHandlers();
  NHCommands c;
  current = &c;
  c.add("blink", blink);
  Serial.type("  blink \t 250   0.25 \n");
  c.update();
  CHECK(blinkRuns == 1 && argsSeen == 2);
  CHECK(blinkHadNumber && blinkMs == 250);
  CHECK(blinkDuty > 0.2499f && blinkDuty < 0.2501f);
  CHECK(firstArg == "250" && missingArg == "");

  Serial.type("blink fast\n");
  c.update();
  CHECK(blinkRuns == 2 && !blinkHadNumber && blinkMs == 0 && firstArg == "fast");
}

static void numbersAreChecked() {
  struct {
    const char *text;
    bool number;
    long value;
  } cases[] = {{"12", true, 12},  {"-3", true, -3},     {"+2", true, 2},   {".5", true, 0},
               {"2.9", true, 2},  {"1.2.3", false, 0},  {"-", false, 0},   {"12a", false, 0},
               {"0x10", false, 0}, {"nan", false, 0}};
  for (auto t : cases) {
    reset();
    resetHandlers();
    NHCommands c;
    current = &c;
    c.add("blink", blink);
    Serial.type(std::string("blink ") + t.text + "\n");
    c.update();
    CHECK(blinkHadNumber == t.number);
    CHECK(blinkMs == t.value);
  }
}

static void tooManyArgumentsDoesNotRun() {
  reset();
  resetHandlers();
  NHCommands c;
  current = &c;
  c.add("blink", blink);
  Serial.type("blink 1 2 3 4\n");
  c.update();
  CHECK(blinkRuns == 1 && argsSeen == 4);
  Serial.type("blink 1 2 3 4 5\n");
  c.update();
  CHECK(blinkRuns == 1);
  CHECK(contains(Serial.out, "Too many arguments: at most 4"));
}

static void unknownCommand() {
  reset();
  NHCommands c;
  c.add("on", ledOn);
  Serial.type("dance\n");
  c.update();
  CHECK(contains(Serial.out, "Unknown command: dance. Type help for the list."));
}

static void helpListsCommandsInColumns() {
  reset();
  NHCommands c;
  c.add("on", ledOn, "switch the LED on");
  c.add("blink", blink, F("blink <ms>"));
  c.add("quiet", ledOn);  // no help text
  Serial.type("help\n");
  c.update();
  CHECK(contains(Serial.out, "Commands:\r\n"));
  CHECK(contains(Serial.out, "  on     switch the LED on\r\n"));
  CHECK(contains(Serial.out, "  blink  blink <ms>\r\n"));
  CHECK(contains(Serial.out, "  quiet\r\n"));
  CHECK(contains(Serial.out, "  help   show this list\r\n"));

  Serial.out.clear();
  Serial.type("?\n");
  c.update();
  CHECK(contains(Serial.out, "Commands:"));
}

static void ownHelpReplacesTheBuiltIn() {
  reset();
  NHCommands c;
  c.add("HELP", myHelp);
  Serial.type("help\n");
  c.update();
  CHECK(contains(Serial.out, "my own help"));
  CHECK(!contains(Serial.out, "Commands:"));
  Serial.out.clear();
  Serial.type("?\n");  // "?" means help, whoever's help it is
  c.update();
  CHECK(contains(Serial.out, "my own help"));
  CHECK(!contains(Serial.out, "Commands:"));
}

static void longLinesAreRefused() {
  reset();
  resetHandlers();
  NHCommands c;
  c.add("on", ledOn);
  Serial.type("on " + std::string(NHCommands::MAX_LINE, 'x') + "\n");
  c.update();
  CHECK(ledOnRuns == 0);
  CHECK(contains(Serial.out, "Too long: at most 64 characters"));
  Serial.type("on\n");  // the next line is fine
  c.update();
  CHECK(ledOnRuns == 1);

  Serial.type(std::string(NHCommands::MAX_LINE - 2, ' ') + "on\n");  // exactly 64 fits
  c.update();
  CHECK(ledOnRuns == 2);
}

static void backspaceAndSplitArrivals() {
  reset();
  resetHandlers();
  NHCommands c;
  current = &c;
  c.add("on", ledOn);
  c.add("blink", blink);
  Serial.type("onx\b\n");
  c.update();
  CHECK(ledOnRuns == 1);
  Serial.type("bl");
  c.update();
  CHECK(blinkRuns == 0);
  Serial.type("ink 5\n");
  c.update();
  CHECK(blinkRuns == 1 && blinkMs == 5);
}

static void badAddsAreRefused() {
  reset();
  NHCommands c;
  CHECK(!c.add(nullptr, ledOn));
  CHECK(!c.add("", ledOn));
  CHECK(!c.add("two words", ledOn));
  CHECK(!c.add("on", nullptr));
  CHECK(c.add("on", ledOn));
  CHECK(!c.add("ON", ledOn));  // taken, whatever the case
  static char names[NHCommands::MAX_COMMANDS][4];
  int added = 1;
  for (int i = 0; i < NHCommands::MAX_COMMANDS; i++) {
    snprintf(names[i], sizeof(names[i]), "c%d", i);
    added += c.add(names[i], ledOn);
  }
  CHECK(added == NHCommands::MAX_COMMANDS);  // full
}

int main() {
  timerCountsFromFirstCall();
  timerDoesNotDrift();
  timerSkipsWhenFarBehind();
  timerRestartAndSetInterval();
  timerZeroIntervalFiresEveryCall();
  timerAcrossRollover();

  timeoutIdleUntilStarted();
  timeoutRunsOutOnce();
  timeoutStartAgainRestarts();
  timeoutStopCancels();
  timeoutZeroIsDoneAtOnce();
  timeoutStaysOverWithoutDone();
  timeoutAcrossRollover();

  buttonSetsPullUpAndStartsUp();
  bouncyPressIsOnePress();
  pressIsReportedAfterTheDebounceTime();
  eventsLastOneUpdate();
  noiseShorterThanDebounceIsIgnored();
  longPressIsNotAClick();
  releaseJustBeforeLongPressIsAClick();
  heldAtStartUpIsNotAPress();
  chargingCapacitorAtStartUpIsNotAPress();
  buttonToVcc();
  customTimes();
  worksWithoutBegin();
  buttonAcrossRollover();

  commandsRunAndEcho();
  commandsIgnoreCaseAndLineEndings();
  commandsGetArguments();
  numbersAreChecked();
  tooManyArgumentsDoesNotRun();
  unknownCommand();
  helpListsCommandsInColumns();
  ownHelpReplacesTheBuiltIn();
  longLinesAreRefused();
  backspaceAndSplitArrivals();
  badAddsAreRefused();

  printf("%d checks, %d failed\n", checks, failures);
  return failures ? 1 : 0;
}
