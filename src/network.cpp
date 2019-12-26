#include "network.h"
#include "updater.h"
#include <FS.h>
#include <ArduinoJson.h>
#include <time.h>

ESP8266WiFiMulti WiFiMulti;
WiFiClient clientRegular;
WiFiClientSecure clientSecure;
MQTTClient mqtt(MQTT_BUFFER_SIZE);
network_config_t network_config;
wifi_config_t wifi_config;
mqtt_config_t mqtt_config;

static const char isrg_root_x1[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

static const char trustid_x3_root[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/
MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT
DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow
PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD
Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB
AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O
rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq
OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b
xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw
7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD
aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV
HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG
SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69
ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr
AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz
R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5
JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo
Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ
-----END CERTIFICATE-----
)EOF";

BearSSL::X509List caList;

const char* const wl_status_strings[] = {
    "IDLE",
    "NO SSID AVAILABLE",
    "SCAN COMPLETED",
    "CONNECTED",
    "CONNECT FAILED",
    "CONNECTION LOST",
    "DISCONNECTED",
    0
};

void network::start(const char *project_name) {
  network::start(project_name, false);
}

void network::start(const char *project_name, bool mqtt_username_as_device_id) {
  // Append these two roots of trust to the CA list for cert validation.
  caList.append(isrg_root_x1);
  caList.append(trustid_x3_root);
  clientSecure.setTrustAnchors(&caList);
  
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  // Read WiFi and MQTT configuration
  read_config();

  // Set some convenience variables
  strncpy(network_config.project_name, project_name, PROJECT_NAME_LEN);

  // Optionally, set custom device identifier
  if (mqtt_username_as_device_id) {
    snprintf(network_config.mqtt_device_topic, MQTT_FIELD_LEN, "devices/%s", mqtt_config.username);
  }
  set_node_name();
  configTime(0, 0, "0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org");

  WiFiMulti.addAP(wifi_config.ssid, wifi_config.password);

#ifdef METERING_SSL_INSECURE
  // Use sparingly.
  clientSecure.setInsecure();
#endif

  mqtt.begin(mqtt_config.server,
    mqtt_config.port,
    mqtt_config.ssl ? clientSecure : clientRegular);
  mqtt.onMessage(network::mqtt_message_received_cb);
}

void network::hello() {
  subscribe();
  DynamicJsonBuffer json;
  JsonObject &root = json.createObject();

  root["project"] = network_config.project_name;
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

  M_DEBUG("Waiting 10 seconds for timestamp");
  int timeout = 10;
  while (timeout-- && time(NULL) == 0) {
    delay(1000);
    loop();
  }
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

  DynamicJsonBuffer json;
  JsonObject &root = json.parse(f);

  if (!root.success()) {
    M_DEBUG("Couldn't parse JSON");
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

void network::report(String &stream) {
  send(network_config.mqtt_device_topic, stream.c_str(), false);
}

void network::send(const char *topic, const char *payload, bool retained) {
  maybe_reconnect();

  M_DEBUG("topic: %s", topic);
  M_DEBUG("payload: %s", payload);

  int retry = 10;
  int _delay = 50;
  while (retry--) {
    _delay *= 2;
    delay(_delay);
    if (mqtt.connected()) {
      M_DEBUG("sending");
      if (mqtt.publish(topic, payload, retained, 1)) {
        M_DEBUG("published");
        retry = 0;
      }
    }
  }
}

void network::maybe_reconnect() {
  if (WiFiMulti.run() == WL_CONNECTED && mqtt.connected()) return;

  while (WiFiMulti.run() != WL_CONNECTED) {
    M_DEBUG("Reconnecting to WiFi (status = %s)", wl_status_strings[WiFiMulti.run()]);
    delay(WIFI_RECONNECT_DELAY);
  }

  M_DEBUG("Connected, got IP: %s", WiFi.localIP().toString().c_str());

  while (!mqtt.connected()) {
    M_DEBUG("(Re)connecting to MQTT");
    if (strlen(mqtt_config.username) == 0) {
      mqtt.connect(network_config.mqtt_client_name);
    } else {
      mqtt.connect(network_config.mqtt_client_name, mqtt_config.username, mqtt_config.password);
    }
    delay(MQTT_RECONNECT_DELAY);
  }

  subscribe();
}

void network::mqtt_message_received_cb(String &topic, String &payload) {
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
      send("pong", network_config.node_name, false);
    }
  }
  // if (topic == "time/epoch") {
  //   long t = atol(payload.c_str());
  //   if (t > 0) {
  //     updateTime(t);
  //     M_DEBUG("Time set to %d", t);
  //   }
  // }
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
  if (strlen(network_config.mqtt_device_topic) == 0)
    snprintf(network_config.mqtt_device_topic, MQTT_FIELD_LEN, "devices/%s", network_config.node_name);
}

void network::subscribe() {
  // Subscribe to two control topics, one for all sensors using
  // this software, and the other for one individual sensor
  maybe_reconnect();

  String topic = String(network_config.mqtt_device_topic);
  topic.replace("devices/", "control/");
  if (mqtt.subscribe(topic.c_str())) {
    M_DEBUG("Subscribed to %s", topic.c_str());
  }

  char control_topic[35];
  memset(control_topic, 0, 35);
  snprintf(control_topic, 9 + PROJECT_NAME_LEN, "control/%s", network_config.project_name);
  if (mqtt.subscribe(control_topic)) {
    M_DEBUG("Subscribed to %s", control_topic);
  }

  // Subscribe to an MQTT channel with the current time
  if (mqtt.subscribe("time/epoch")) {
    M_DEBUG("Subscribed to time/epoch");
  }
}
