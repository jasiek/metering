#include "debug.h"

void debug(const char *file, int line, const char *s) {
  char message[1024];
  snprintf(message, 1024, "%s:%d: %s", file, line, s);
  Serial.println(message);
}

void debug(const char *file, int line, String &s) {
  debug(file, line, s.c_str());
}

void debug(const char *file, int line, const char *f, const char *s) {
  char formatted[1024-8];
  snprintf(formatted, 1024-8, f, s);
  debug(file, line, formatted);
}

void debug(const char *file, int line, String &f, String &s) {
  debug(file, line, f.c_str(), s.c_str());
}
