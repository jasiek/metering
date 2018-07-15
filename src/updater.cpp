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

void updater::restart() {
  ESP.restart();
}

const String updater::update(String &url) {
  if (_resetter != NULL) _resetter->detach();
  ESP8266HTTPUpdate upd;
  upd.rebootOnUpdate(false);
  switch(upd.update(url, GIT_REVISION)) {
  case HTTP_UPDATE_FAILED:
    return String("Update failed.");
    break;
  case HTTP_UPDATE_NO_UPDATES:
    return String("No update required.");
    break;
  case HTTP_UPDATE_OK:
    _resetter->once(5, updater::restart);
    return String("Updated.");
  }
  return String("This should never happen.");
}
