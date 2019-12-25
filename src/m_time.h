#ifndef _M_TIME_H_
#define _M_TIME_H_

#include <Arduino.h>

// Poor man's RTC.

namespace m_time {
  void set(uint32 t);
  uint32 get_current();
}

#endif
