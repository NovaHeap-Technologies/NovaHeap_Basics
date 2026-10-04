// NovaHeap_Basics: timers, a debounced button and a serial command menu, without delay().
// Copyright (c) 2026 NovaHeap Technologies
// Released under the NovaHeap Technologies License (MIT terms). See LICENSE.

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "NovaHeap_Basics.h"

static char lower(char c) { return (char)tolower((unsigned char)c); }

static bool sameWord(const char *a, const char *b) {
  while (*a && *b) {
    if (lower(*a++) != lower(*b++)) return false;
  }
  return *a == *b;
}

static bool isSpace(char c) { return c == ' ' || c == '\t'; }

// Definitions for the limits, in case one is passed by reference, as min() does.
const uint8_t NHCommands::MAX_COMMANDS;
const uint8_t NHCommands::MAX_LINE;
const uint8_t NHCommands::MAX_ARGS;

NHCommands::NHCommands(Stream &port)
    : _port(port), _helpInFlash(0), _count(0), _length(0), _tooLong(false), _argCount(0) {
  _line[0] = 0;
}

bool NHCommands::add(const char *name, void (*run)()) {
  return addCommand(name, run, nullptr, false);
}

bool NHCommands::add(const char *name, void (*run)(), const char *help) {
  return addCommand(name, run, help, false);
}

bool NHCommands::add(const char *name, void (*run)(), const __FlashStringHelper *help) {
  return addCommand(name, run, reinterpret_cast<const char *>(help), true);
}

bool NHCommands::addCommand(const char *name, void (*run)(), const char *help, bool inFlash) {
  if (!name || !*name || !run || _count >= MAX_COMMANDS) return false;
  for (const char *c = name; *c; c++) {
    if (isSpace(*c)) return false;  // one word, or it could never be typed
  }
  if (find(name)) return false;  // already taken
  if (inFlash) _helpInFlash |= (uint16_t)(1u << _count);
  _commands[_count++] = {name, run, help};
  return true;
}

const NHCommands::Command *NHCommands::find(const char *name) const {
  for (uint8_t i = 0; i < _count; i++) {
    if (sameWord(name, _commands[i].name)) return &_commands[i];
  }
  return nullptr;
}

void NHCommands::update() {
  while (_port.available() > 0) {
    int c = _port.read();
    if (c < 0) break;

    if (c == '\n' || c == '\r') {
      if (_tooLong) {
        _port.print(F("Too long: at most "));
        _port.print(MAX_LINE);
        _port.println(F(" characters"));
      } else if (_length > 0) {
        _line[_length] = 0;
        runLine();
      }
      _length = 0;  // "\r\n" ends one line, then finds this one empty
      _tooLong = false;
    } else if (c == '\b' || c == 127) {  // backspace, from terminal programs
      if (_length > 0) _length--;
    } else if (_length < MAX_LINE) {
      _line[_length++] = (char)c;
    } else {
      _tooLong = true;
    }
  }
}

void NHCommands::runLine() {
  const char *c = _line;
  while (isSpace(*c)) c++;
  if (!*c) return;  // a blank line

  // Show what was typed, so the reply reads next to it in the Serial Monitor.
  _port.print(F("> "));
  _port.println(_line);

  // Split into words, in place.
  char *words[1 + MAX_ARGS];
  uint8_t count = 0;
  bool tooMany = false;
  char *p = _line;
  while (*p) {
    while (isSpace(*p)) *p++ = 0;
    if (!*p) break;
    if (count == 1 + MAX_ARGS) {
      tooMany = true;
      break;
    }
    words[count++] = p;
    while (*p && !isSpace(*p)) p++;
  }

  const Command *command = find(words[0]);
  if (!command && sameWord(words[0], "?")) command = find("help");  // "?" means help
  if (!command) {
    if (sameWord(words[0], "help") || sameWord(words[0], "?")) {
      printHelp();
    } else {
      _port.print(F("Unknown command: "));
      _port.print(words[0]);
      _port.println(F(". Type help for the list."));
    }
    return;
  }
  if (tooMany) {
    _port.print(F("Too many arguments: at most "));
    _port.println(MAX_ARGS);
    return;
  }

  _argCount = count - 1;
  for (uint8_t i = 0; i < _argCount; i++) _args[i] = words[i + 1];
  command->run();
  _argCount = 0;
}

void NHCommands::printHelp() {
  bool ownHelp = false;
  size_t width = 4;  // "help"
  for (uint8_t i = 0; i < _count; i++) {
    size_t length = strlen(_commands[i].name);
    if (length > width) width = length;
    if (sameWord(_commands[i].name, "help")) ownHelp = true;
  }

  _port.println(F("Commands:"));
  for (uint8_t i = 0; i < _count; i++) {
    const Command &c = _commands[i];
    _port.print(F("  "));
    _port.print(c.name);
    if (c.help) {
      for (size_t pad = strlen(c.name); pad < width + 2; pad++) _port.print(' ');
      if (_helpInFlash & (1u << i)) {
        _port.print(reinterpret_cast<const __FlashStringHelper *>(c.help));
      } else {
        _port.print(c.help);
      }
    }
    _port.println();
  }
  if (!ownHelp) {
    _port.print(F("  help"));
    for (size_t pad = 4; pad < width + 2; pad++) _port.print(' ');
    _port.println(F("show this list"));
  }
}

const char *NHCommands::arg(uint8_t i) const { return i < _argCount ? _args[i] : ""; }

// A whole number or a decimal, with an optional sign: 12, -3, 2.5, .5
bool NHCommands::argIsNumber(uint8_t i) const {
  const char *s = arg(i);
  if (*s == '+' || *s == '-') s++;
  bool digits = false, point = false;
  for (; *s; s++) {
    if (isdigit((unsigned char)*s)) {
      digits = true;
    } else if (*s == '.' && !point) {
      point = true;
    } else {
      return false;
    }
  }
  return digits;
}

long NHCommands::argInt(uint8_t i) const { return argIsNumber(i) ? strtol(arg(i), nullptr, 10) : 0; }

float NHCommands::argFloat(uint8_t i) const { return argIsNumber(i) ? (float)atof(arg(i)) : 0; }
