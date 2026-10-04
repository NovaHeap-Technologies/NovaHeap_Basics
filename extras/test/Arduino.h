// Minimal Arduino API for running the unit tests on a PC.
#pragma once
#include <stddef.h>
#include <stdint.h>

#include <string>

#define LOW 0x0
#define HIGH 0x1
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

unsigned long millis();
void delay(unsigned long ms);
void pinMode(uint8_t pin, uint8_t mode);
int digitalRead(uint8_t pin);

class __FlashStringHelper;
#define F(s) (reinterpret_cast<const __FlashStringHelper *>(s))

// Print and Stream, writing to and reading from strings.
class Print {
 public:
  std::string out;
  void print(const char *s) { out += s; }
  void print(const __FlashStringHelper *s) { out += reinterpret_cast<const char *>(s); }
  void print(char c) { out += c; }
  void print(int n) { out += std::to_string(n); }
  void println() { out += "\r\n"; }
  template <typename T>
  void println(T value) {
    print(value);
    println();
  }
};

class Stream : public Print {
 public:
  std::string in;
  size_t next = 0;
  int available() { return (int)(in.size() - next); }
  int read() { return next < in.size() ? (unsigned char)in[next++] : -1; }
  void type(const std::string &text) { in += text; }
};

extern Stream Serial;
