#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <MQTTClient.h>
#include "debug.h"

#define MASS_CONTROL_TOPIC "control/ESP8266-Weather"

struct network_config_t {
  bool ok;
  char wifi_ssid[128];
  char wifi_pass[128];
  char mqtt_server[128];
  int mqtt_port;
  bool mqtt_ssl;
  char mqtt_username[128];
  char mqtt_password[128];
  char mqtt_incoming_topic[128];
  char node_name[12];
};

namespace network {
  void start();
  network_config_t *config();
  void hello();
  void report(float temp, float humidity, float pressure, float vcc);
  void send(const char *topic, const char *payload, bool retained);
  const char* mqtt_client_name();
  const char* mqtt_topic();
  void maybe_reconnect();
  void mqtt_message_received_cb(String topic, String payload, char * bytes, unsigned int length);
  void loop();
}

#endif
