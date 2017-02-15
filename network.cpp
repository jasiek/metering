#include "network.h"
#include "updater.h"
#include <FS.h>
#include <ArduinoJson.h>

ESP8266WiFiMulti WiFiMulti;
WiFiClient clientRegular;
WiFiClientSecure clientSecure;
MQTTClient mqtt;
network_config_t network_config;
wifi_config_t wifi_config;
mqtt_config_t mqtt_config;

#define BUFFER_SIZE 1024

void network::start(const char *project_name) {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  // Read WiFi and MQTT configuration
  read_config();

  // Set some convenience variables
  strncpy(network_config.project_name, project_name, PROJECT_NAME_LEN);
  set_node_name();

  WiFiMulti.addAP(wifi_config.ssid, wifi_config.password);
  mqtt.begin(mqtt_config.server,
    mqtt_config.port,
    mqtt_config.ssl ? clientSecure : clientRegular);
}

void network::hello() {
  StaticJsonBuffer<BUFFER_SIZE> json;
  JsonObject &root = json.createObject();

  root["mac"] = network_config.node_name;
  root["chip_id"] = ESP.getChipId();
  root["flash_chip_id"] = ESP.getFlashChipId();
  root["flash_chip_size"] = ESP.getFlashChipSize();
  root["flash_chip_real_size"] = ESP.getFlashChipRealSize();
  root["flash_chip_speed"] = ESP.getFlashChipSpeed();
  root["core_version"] = ESP.getCoreVersion();
  root["sdk_version"] = ESP.getSdkVersion();
  root["boot_version"] = ESP.getBootVersion();
  root["boot_mode"] = ESP.getBootMode();
  root["cpu_mhz"] = ESP.getCpuFreqMHz();
  root["reset_reason"] = ESP.getResetReason();
  root["reset_info"] = ESP.getResetInfo();
  root["sketch_size"] = ESP.getSketchSize();
  root["sketch_md5"] = ESP.getSketchMD5();
  root["sketch_git_rev"] = GIT_REVISION;

  String stream;
  root.printTo(stream);
  send("hello", stream.c_str(), false);
}

bool network::read_config() {
  memset(&network_config, 0, sizeof(network_config_t));
  memset(&mqtt_config, 0, sizeof(mqtt_config_t));
  memset(&wifi_config, 0, sizeof(wifi_config_t));

  if (!SPIFFS.begin()) {
    M_DEBUG("SPIFFS could not be accessed");
    return false;
  }

  File f = SPIFFS.open("/config.json", "r");
  if (!f) {
    M_DEBUG("Opening /config.json failed");
    return false;
  }

  StaticJsonBuffer<BUFFER_SIZE> json;
  char readFileBuffer[BUFFER_SIZE];

  f.readBytes(readFileBuffer, BUFFER_SIZE);
  JsonObject &root = json.parse(readFileBuffer);

  if (!root.success()) {
    M_DEBUG("Couldn't parse JSON");
    M_DEBUG(readFileBuffer);
    SPIFFS.end();
    return false;
  }

  strncpy(wifi_config.ssid, root["wifi_ssid"].as<char*>(), WIFI_SSID_LEN);
  strncpy(wifi_config.password, root["wifi_pass"].as<char*>(), WIFI_PASS_LEN);
  wifi_config.ok = true;

  strncpy(mqtt_config.server, root["mqtt_server"].as<char*>(), MQTT_FIELD_LEN);
  mqtt_config.port = root["mqtt_port"].as<int>();
  if (root["mqtt_username"].success()) {
    strncpy(mqtt_config.username, root["mqtt_username"].as<char*>(), MQTT_FIELD_LEN);
  }
  if (root["mqtt_password"].success()) {
    strncpy(mqtt_config.password, root["mqtt_password"].as<char*>(), MQTT_FIELD_LEN);
  }
  mqtt_config.ssl = root["mqtt_ssl"].as<bool>();
  mqtt_config.ok = true;

  SPIFFS.end();
  return true;
}

void network::report(float temp, float humidity, float pressure, float vcc) {
  StaticJsonBuffer<BUFFER_SIZE> buffer;
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

  send(network_config.mqtt_device_topic, stream.c_str(), true);
}

void network::send(const char *topic, const char *payload, bool retained) {
  maybe_reconnect();

  M_DEBUG("topic: %s", topic);
  M_DEBUG("payload: %s", payload);

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
      M_DEBUG("sending");
      if (mqtt.publish(&message)) {
        M_DEBUG("published");
        retry = 0;
      }
    }
  }
}

void network::maybe_reconnect() {
  if (WiFiMulti.run() == WL_CONNECTED && mqtt.connected()) return;

  while (WiFiMulti.run() != WL_CONNECTED) {
    M_DEBUG("Reconnecting to WiFi");
    delay(1000);
  }

  M_DEBUG("Connected, got IP: %s", WiFi.localIP().toString().c_str());

  while (!mqtt.connected()) {
    M_DEBUG("(Re)connecting to MQTT");
    if (strlen(mqtt_config.username) == 0) {
      mqtt.connect(network_config.mqtt_client_name);
    } else {
      mqtt.connect(network_config.mqtt_client_name, mqtt_config.username, mqtt_config.password);
    }
    delay(1000);
  }

  subscribe();
}

void network::mqtt_message_received_cb(String topic, String payload, char * bytes, unsigned int length) {
  M_DEBUG("Incoming message from %s.", topic.c_str());
  M_DEBUG("Payload: %s", payload.c_str());

  if (topic.startsWith("control/")) {
    if (payload.startsWith("RESET")) ESP.restart();
    if (payload.startsWith("UPDATE")) {
      payload.remove(0, 7);
      payload.trim();
      updater::update(payload);
    }
    if (payload.startsWith("PING")) {
      send("hello", "PONG", false);
    }
  }
}

void network::loop() {
  mqtt.loop();
}

void network::set_node_name() {
  String nodeName = WiFi.macAddress();
  for (int i = nodeName.indexOf(':'); i > -1; i = nodeName.indexOf(':')) nodeName.remove(i, 1);
  nodeName.toLowerCase();
  strncpy(network_config.node_name, nodeName.c_str(), MAC_LEN);
  M_DEBUG("Node name: %s", network_config.node_name);

  // Pull this out some day, maybe?
  snprintf(network_config.mqtt_client_name, MQTT_FIELD_LEN, "%s (%s)", network_config.project_name, network_config.node_name);
  snprintf(network_config.mqtt_device_topic, MQTT_FIELD_LEN, "devices/%s", network_config.node_name);
}

void network::subscribe() {
  // Subscribe to two control topics, one for all sensors using
  // this software, and the other for one individual sensor
  char control_topic[35];
  memset(control_topic, 0, 35);
  snprintf(control_topic, 9 + MAC_LEN, "control/%s", network_config.node_name);
  if (mqtt.subscribe(control_topic)) {
    M_DEBUG("Subscribed to %s", control_topic);
  }

  memset(control_topic, 0, 35);
  snprintf(control_topic, 9 + PROJECT_NAME_LEN, "control/%s", network_config.project_name);
  if (mqtt.subscribe(control_topic)) {
    M_DEBUG("Subscribed to %s", control_topic);
  }
}
