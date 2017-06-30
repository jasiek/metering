#ifndef _METERING_DEBUG_H
#define _METERING_DEBUG_H

#include <Arduino.h>

#define M_DEBUG(...) metering::debug(__FILE__, __LINE__, __VA_ARGS__)

namespace metering {
  void debug(const char *, int, const char *);
  void debug(const char *, int, const char *, const char *);
  void debug(const char *, int, String &);
  void debug(const char *, int, String &, String &);
}

#endif
