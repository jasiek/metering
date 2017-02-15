#include "updater.h"
#include "debug.h"
#include <ESP8266httpUpdate.h>

Ticker *_resetter;

void updater::begin() {
  _resetter = NULL;
}

void updater::begin(Ticker *t) {
  _resetter = t;
}

void updater::update(String &url) {
  M_DEBUG("Attempting to update from %s");
  if (_resetter != NULL) _resetter->detach();
  ESP8266HTTPUpdate upd;
  upd.rebootOnUpdate(false);
  switch(upd.update(url, GIT_REVISION)) {
    case HTTP_UPDATE_FAILED:
      M_DEBUG("Update failed.");
      break;
    case HTTP_UPDATE_NO_UPDATES:
      M_DEBUG("No update required.");
      break;
    case HTTP_UPDATE_OK:
      M_DEBUG("Updated.");
  }
  M_DEBUG(upd.getLastErrorString().c_str());
  delay(5000);
  ESP.restart();
}
