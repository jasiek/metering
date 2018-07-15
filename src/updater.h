#ifndef _UPDATER_H
#define _UPDATER_H

#include <Ticker.h>
#include <Arduino.h>

namespace updater {
  void begin();
  void begin(Ticker *t);
  void restart();
  String update(String &url);
}

#endif
