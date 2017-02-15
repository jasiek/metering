#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <MQTTClient.h>
#include "debug.h"

#define WIFI_SSID_LEN 32
#define WIFI_PASS_LEN 64

struct wifi_config_t {
  bool ok;
  char ssid[WIFI_SSID_LEN + 1];
  char password[WIFI_PASS_LEN + 1];
};

#define MQTT_FIELD_LEN 64

struct mqtt_config_t {
  bool ok;
  char server[MQTT_FIELD_LEN + 1];
  int port;
  bool ssl;
  char username[MQTT_FIELD_LEN + 1];
  char password[MQTT_FIELD_LEN + 1];
};

#define MAC_LEN 12
#define PROJECT_NAME_LEN 24

struct network_config_t {
  char mqtt_client_name[MQTT_FIELD_LEN + 1];
  char mqtt_device_topic[MQTT_FIELD_LEN + 1];
  char mqtt_incoming_topic[MQTT_FIELD_LEN + 1];
  char node_name[MAC_LEN + 1];
  char project_name[PROJECT_NAME_LEN + 1];
};

namespace network {
  void start(const char *);
  void hello();
  void report(float temp, float humidity, float pressure, float vcc);
  void maybe_reconnect();
  void mqtt_message_received_cb(String topic, String payload, char * bytes, unsigned int length);
  void loop();

  // "private" functions
  bool read_config();
  void set_node_name();
  void send(const char *topic, const char *payload, bool retained);
  void subscribe();
}

#endif
