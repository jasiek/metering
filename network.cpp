#include "network.h"
#include "updater.h"
#include <FS.h>
#include <ArduinoJson.h>

ESP8266WiFiMulti WiFiMulti;
WiFiClient clientRegular;
WiFiClientSecure clientSecure;
MQTTClient mqtt;
network_config_t network_config;

void network::start() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  // Read network configuration, TODO: rename this.
  network::config();

  String nodeName = WiFi.macAddress();
  for (int i = nodeName.indexOf(':'); i > -1; i = nodeName.indexOf(':')) nodeName.remove(i, 1);
  nodeName.toLowerCase();
  strncpy(network_config.node_name, nodeName.c_str(), 12);
  DEBUG("Node name: %s", network_config.node_name);

  WiFiMulti.addAP(network_config.wifi_ssid, network_config.wifi_pass);
  mqtt.begin(network_config.mqtt_server,
    network_config.mqtt_port,
    network_config.mqtt_ssl ? clientSecure : clientRegular);
}

network_config_t *network::config() {
  if (network_config.ok) return &network_config;

  StaticJsonBuffer<200> buffer;
  char readFileBuffer[200];
  if (!SPIFFS.begin()) {
    DEBUG("SPIFFS could not be accessed");
    network_config.ok = false;
    return &network_config;
  }

  File f = SPIFFS.open("/config.json", "r");

  if (!f) {
    DEBUG("Opening config.json failed");
    network_config.ok = false;
    return &network_config;
  }

  f.readBytes(readFileBuffer, 200);
  DEBUG(readFileBuffer);
  JsonObject &root = buffer.parse(readFileBuffer);

  if (!root.success()) {
    DEBUG("Parsing config.json failed");
    network_config.ok = false;
    return &network_config;
  }

  strncpy(network_config.wifi_ssid, root["wifi_ssid"].asString(), 128);
  strncpy(network_config.wifi_pass, root["wifi_pass"].asString(), 128);
  strncpy(network_config.mqtt_server, root["mqtt_server"].asString(), 128);
  network_config.mqtt_port = root["mqtt_port"].as<int>();
  strncpy(network_config.mqtt_username, root["mqtt_username"].asString(), 128);
  strncpy(network_config.mqtt_password, root["mqtt_password"].asString(), 128);
  network_config.mqtt_ssl = root["mqtt_ssl"].as<bool>();
  network_config.ok = true;

  SPIFFS.end();

  return &network_config;
}

void network::hello() {
  StaticJsonBuffer<512> buffer;
  JsonObject& root = buffer.createObject();
  String stream;

  root["mac"] = network_config.node_name;
  root["sdk"] = ESP.getSdkVersion();
  root["boot_mode"] = ESP.getBootMode();
  root["boot_version"] = ESP.getBootVersion();
  root["chip_id"] = ESP.getChipId();
  root["core_version"] = ESP.getCoreVersion();
  root["cpu_freq"] = ESP.getCpuFreqMHz();
  root["md5"] =  ESP.getSketchMD5();
  root["git_rev"] = GIT_REVISION;
  root["uptime"] = millis() / 1000;

  root.printTo(stream);
  send("hello", stream.c_str(), false);
}

void network::report(float temp, float humidity, float pressure, float vcc) {
  StaticJsonBuffer<200> buffer;
  String stream;
  JsonObject& root = buffer.createObject();
  if (!isnan(temp)) {
    root["temperature"] = temp;
  }
  if (!isnan(humidity)) {
    root["humidity"] = humidity;
  }
  if (!isnan(pressure)) {
    root["pressure"] = pressure;
  }

  root["voltage"] = vcc;
  root["rssi"] = WiFi.RSSI();
  root["uptime"] = millis() / 1000;
  root.printTo(stream);

  send(mqtt_topic(), stream.c_str(), true);
}

void network::send(const char *topic, const char *payload, bool retained) {
  maybe_reconnect();

  DEBUG("topic: %s", topic);
  DEBUG("payload: %s", payload);

  MQTTMessage message;
  message.topic = (char*)topic;
  message.length = strlen(payload);
  message.payload = (char *)payload;
  message.retained = retained;

  int retry = 10;
  int _delay = 50;
  while (retry--) {
    _delay *= 2;
    delay(_delay);
    if (mqtt.connected()) {
      mqtt.loop();
      DEBUG("sending");
      if (mqtt.publish(&message)) {
        DEBUG("published");
        retry = 0;
      }
    }
  }
}

const char *network::mqtt_client_name() {
  static char client_name[64];
  sprintf(client_name, "ESP8266-Weather (%s)", network_config.node_name);
  return client_name;
}

const char *network::mqtt_topic() {
  static char topic[64];
  sprintf(topic, "/devices/%s", network_config.node_name);
  return topic;
}

void network::maybe_reconnect() {
  if (WiFiMulti.run() == WL_CONNECTED && mqtt.connected()) return;

  while (WiFiMulti.run() != WL_CONNECTED) {
    DEBUG("Reconnecting to WiFi");
    delay(1000);
  }

  DEBUG("Connected, got IP: %s", WiFi.localIP().toString().c_str());

  while (!mqtt.connected()) {
    DEBUG("(Re)connecting to MQTT");
    if (strlen(network_config.mqtt_username) == 0) {
      mqtt.connect(mqtt_client_name());
    } else {
      mqtt.connect(mqtt_client_name(), network_config.mqtt_username, network_config.mqtt_password);
    }
    delay(1000);
  }

  // Subscribe to two control topics, one for all sensors using
  // this software, and the other for one individual sensor
  char control_topic[9+12];
  sprintf(control_topic, "control/%s", network_config.node_name);
  if (mqtt.subscribe(MASS_CONTROL_TOPIC)) {
    DEBUG("Subscribed to %s", MASS_CONTROL_TOPIC);
  }
  if (mqtt.subscribe(control_topic)) {
    DEBUG("Subscribed to %s", control_topic);
  }
}

void network::mqtt_message_received_cb(String topic, String payload, char * bytes, unsigned int length) {
  if (topic.startsWith("control/")) {
    if (payload.startsWith("RESET")) ESP.restart();
    if (payload.startsWith("UPDATE")) {
      payload.remove(0, 7);
      payload.trim();
      updater::update(payload);
    }
    if (payload.startsWith("PING")) {
      hello();
    }
  }

  DEBUG("incoming: ");
  DEBUG(topic);
  DEBUG(" - ");
  DEBUG(payload);
}

void network::loop() {
  mqtt.loop();
}
