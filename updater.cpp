#include "updater.h"
#include "debug.h"
#include <ESP8266httpUpdate.h>

Ticker *_resetter;

void updater::begin(Ticker *t) {
  _resetter = t;
}

void updater::update(String &url) {
  _resetter->detach();
  ESP8266HTTPUpdate upd;
  upd.rebootOnUpdate(true);
  switch(upd.update(url, GIT_REVISION)) {
    case HTTP_UPDATE_FAILED:
      DEBUG("Update failed.");
      break;
    case HTTP_UPDATE_NO_UPDATES:
      DEBUG("No update required.");
      break;
    case HTTP_UPDATE_OK:
      DEBUG("Updated.");
  }
  DEBUG(upd.getLastErrorString().c_str());
  delay(5000);
  ESP.restart();
}
