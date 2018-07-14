#include "debug.h"

void metering::debug(const char *file, int line, const char *s) {
  Serial.printf("%s:%d: %s\r\n", file, line, s);
}

void metering::debug(const char *file, int line, String &s) {
  debug(file, line, s.c_str());
}

void metering::debug(const char *file, int line, const char *f, const char *s) {
  char formatted[1024-8];
  snprintf(formatted, 1024-8, f, s);
  debug(file, line, formatted);
}

void metering::debug(const char *file, int line, const char *f, uint32 u) {
  char formatted[1024-8];
  snprintf(formatted, 1024-8, f, u);
  debug(file, line, formatted);
}

void metering::debug(const char *file, int line, String &f, String &s) {
  debug(file, line, f.c_str(), s.c_str());
}
