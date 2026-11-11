#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <MqttLogic.h>
#include <HaDiscovery.h>
#include "Mqtt.h"
#include "Config.h"
#include "Settings.h"
#include "Device.h"
#include "LedStrip.h"
#include "Network.h"
#include "Ota.h"

// Discovery message (~1.7 KB) + topic + MQTT header
#define MQTT_BUFFER_SIZE          2304
#define MQTT_DEFAULT_PORT         1883
#define T_TELEMETRY               T_5MIN

static WiFiClient espClient;
static PubSubClient client(espClient);
static unsigned long lastMqttCheckConn = 0;
static unsigned long lastTelemetry     = 0;
static unsigned long republishAt       = 0;   // != 0: discovery to publish again (Home Assistant restarted)

static String mqttHost;
static uint16_t mqttPort = MQTT_DEFAULT_PORT;
static String uniqueId;          // sonoff_<chip id>
static String baseTopic;         // sonoff/<hostname>
static String commandTopic;
static String stateTopic;
static String availabilityTopic;
static String actionTopic;

static String Topic(const char* suffix) {
  return baseTopic + suffix;
}

static bool Publish(const String& topic, const char* payload, bool retained = false) {
  if (!client.publish(topic.c_str(), payload, retained)) {
    Serial.println("MQTT publish error on " + topic);
    return false;
  }
  return true;
}

static void PublishDiscovery() {
  static char payload[MQTT_BUFFER_SIZE];
  char topic[96];
  String configUrl = "http://" + WiFi.localIP().toString() + "/";
  HaDeviceInfo dev = {uniqueId.c_str(), Hostname.Val.c_str(), baseTopic.c_str(), commandTopic.c_str(), stateTopic.c_str(),
                      deviceMode == LEDSTRIP_MODE, FW_VERSION, configUrl.c_str()};

  ha_discovery_topic(uniqueId.c_str(), topic, sizeof(topic));
  if (ha_build_device_discovery(&dev, payload, sizeof(payload)) == 0) {
    Serial.println("Home Assistant discovery too long");
    return;
  }
  // Not retained: it is published again when Home Assistant restarts (birth message)
  if (Publish(topic, payload)) {
    Serial.println("Home Assistant discovery published on " + String(topic));
  }
}

static void PublishAll() {
  PublishDiscovery();
  // States after the discovery, so Home Assistant reads them
  PublishState();
  Mqtt_PublishUpdateState(false);
  Mqtt_PublishTelemetry();
}

static void HandleCommand(const byte* payload, unsigned int length) {
  LightCommand cmd;
  if (!mqtt_parse_command((const char*) payload, length, deviceMode == LEDSTRIP_MODE, &cmd)) {
    Serial.println("  - invalid command");
    return;
  }
  bool lightChanged = false;
  if (cmd.hasBrightness) {
    Brightness.Val = cmd.brightness;
    lightChanged = true;
    Serial.println("  - brightness = " + String(Brightness.Val));
  }
  if (cmd.hasColor) {
    Red.Val = cmd.color.r;
    Green.Val = cmd.color.g;
    Blue.Val = cmd.color.b;
    lightChanged = true;
    Serial.println("  - color = " + LedStrip_ColorHex());
  }
  if (cmd.hasEffect) {
    LedStrip_SelectEffect(effect_name(cmd.effect));
    lightChanged = true;
    Serial.println("  - effect = " + LedStrip_EffectName());
  }

  if (cmd.toggle) {
    Switch_Toggle();
  } else if (cmd.hasState && !cmd.on) {
    Switch_Off();
  } else if (cmd.hasState || (lightChanged && IsOn())) {
    // Also applies brightness, color or effect changes and publishes the state
    Switch_On();
  } else if (lightChanged) {
    // Strip off: the new values are used at the next power on
    SaveStateInEeprom();
    PublishState();
  }
}

static void HandleAction(const byte* payload, unsigned int length) {
  switch (ha_parse_action((const char*) payload, length)) {
    case HA_ACTION_RESTART:
      Serial.println("  - restart");
      Restart();
      break;
    case HA_ACTION_CHECK_UPDATE:
    case HA_ACTION_INSTALL:
      // Done in the main loop: the update can take a while
      Serial.println("  - firmware update");
      Ota_Request();
      break;
    default:
      Serial.println("  - unknown action");
      break;
  }
}

static void Callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("MQTT received on " + String(topic) + ": ");
  Serial.write(payload, length);
  Serial.println("");

  if (commandTopic == topic) {
    HandleCommand(payload, length);
  } else if (actionTopic == topic) {
    HandleAction(payload, length);
  } else if (strcmp(topic, HA_STATUS_TOPIC) == 0 && ha_is_birth_message((const char*) payload, length)) {
    // Home Assistant restarted: random delay to avoid all the devices publishing together
    republishAt = millis() + random(1000, 5000);
    if (republishAt == 0) {
      republishAt = 1;
    }
  }
}

static void Connect() {
  Serial.print("Connecting Mqtt client to " + mqttHost + ":" + String(mqttPort) + "... ");
  const char* user = MqttUser.Val.length() > 0 ? MqttUser.Val.c_str() : nullptr;
  const char* password = MqttPassword.Val.length() > 0 ? MqttPassword.Val.c_str() : nullptr;

  // Last will: the broker publishes "offline" if the device disappears
  if (client.connect(uniqueId.c_str(), user, password, availabilityTopic.c_str(), 0, true, HA_PAYLOAD_OFFLINE)) {
    Serial.println("Ok");
    lastMqttCheckConn = 0;
    Publish(availabilityTopic, HA_PAYLOAD_ONLINE, true);
    client.subscribe(commandTopic.c_str());
    client.subscribe(actionTopic.c_str());
    client.subscribe(HA_STATUS_TOPIC);
    PublishAll();
  } else {
    Serial.println("Failed (" + String(client.state()) + ")");
  }
}

void Mqtt_Setup() {
  char id[48];

  // "host" or "host:port"
  mqttHost = MqttServer.Val;
  mqttPort = MQTT_DEFAULT_PORT;
  int colon = mqttHost.indexOf(':');
  if (colon > 0) {
    mqttPort = mqttHost.substring(colon + 1).toInt();
    mqttHost = mqttHost.substring(0, colon);
  }

  snprintf(id, sizeof(id), "sonoff_%06x", ESP.getChipId());
  uniqueId = id;
  ha_sanitize_id(Hostname.Val.c_str(), id, sizeof(id));
  baseTopic = String(HA_BASE_TOPIC_PREFIX) + id;
  // Topics of the settings page, if set, for compatibility with old configurations
  commandTopic = MqttSubTopic.Val.length() > 0 ? MqttSubTopic.Val : Topic(HA_SET_SUFFIX);
  stateTopic = MqttPubTopic.Val.length() > 0 ? MqttPubTopic.Val : Topic(HA_STATE_SUFFIX);
  availabilityTopic = Topic(HA_AVAILABILITY_SUFFIX);
  actionTopic = Topic(HA_ACTION_SUFFIX);

  if (client.connected()) {
    client.disconnect();
  }
  client.setBufferSize(MQTT_BUFFER_SIZE);
  client.setServer(mqttHost.c_str(), mqttPort);
  client.setCallback(Callback);
}

void Mqtt_Loop() {
  if (!deviceConnected || !mqttConfigured) {
    return;
  }
  if (!client.connected()) {
    if (millis() - lastMqttCheckConn > T_5S) {
      lastMqttCheckConn = millis();
      Connect();
    }
    return;
  }
  client.loop();

  if (republishAt != 0 && (long)(millis() - republishAt) >= 0) {
    republishAt = 0;
    PublishAll();
  }
  if (millis() - lastTelemetry > T_TELEMETRY) {
    Mqtt_PublishTelemetry();
  }
}

void Mqtt_Disconnect() {
  if (client.connected()) {
    Publish(availabilityTopic, HA_PAYLOAD_OFFLINE, true);
    client.disconnect();
  }
}

void PublishState() {
  if (!deviceConnected || !client.connected()) {
    return;
  }
  char msg_out[JSON_MSG_LENGTH];
  LightState state = {IsOn(), deviceMode == LEDSTRIP_MODE, Brightness.Val, {Red.Val, Green.Val, Blue.Val}, LedStrip_Effect()};

  if (mqtt_build_state(&state, msg_out, sizeof(msg_out)) == 0) {
    Serial.println("Message too long");
    return;
  }
  if (Publish(stateTopic, msg_out)) {
    Serial.println("State published: " + String(msg_out));
  }
}

void Mqtt_PublishUpdateState(bool inProgress) {
  if (!deviceConnected || !client.connected()) {
    return;
  }
  char msg_out[512];
  String summary = Ota_Status();
  if (ha_build_update_state(FW_VERSION, Ota_LatestVersion().c_str(), inProgress, summary.substring(0, 250).c_str(), msg_out, sizeof(msg_out)) > 0) {
    Publish(Topic(HA_UPDATE_SUFFIX), msg_out);
  }
}

void Mqtt_PublishTelemetry() {
  lastTelemetry = millis();
  if (!deviceConnected || !client.connected()) {
    return;
  }
  char msg_out[128];
  if (ha_build_telemetry(WiFi.RSSI(), WiFi.localIP().toString().c_str(), millis() / 1000, msg_out, sizeof(msg_out)) > 0) {
    Publish(Topic(HA_TELEMETRY_SUFFIX), msg_out);
  }
}

String Mqtt_BaseTopic() {
  return baseTopic;
}

String Mqtt_CommandTopic() {
  return commandTopic;
}

String Mqtt_StateTopic() {
  return stateTopic;
}

String Mqtt_UniqueId() {
  return uniqueId;
}

bool Mqtt_IsConnected() {
  return client.connected();
}
