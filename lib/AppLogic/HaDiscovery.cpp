#include "HaDiscovery.h"
#include "ColorLogic.h"
#include <string.h>
#include <stdio.h>
#include <ArduinoJson.h>

#define DISCOVERY_DOC_SIZE        3072
#define ORIGIN_NAME               "AllInOneSonoff"
#define ORIGIN_URL                "https://github.com/aletiffi/AllInOneSonoff"

void ha_sanitize_id(const char* in, char* out, size_t outSize) {
  if (!out || outSize == 0) {
    return;
  }
  size_t j = 0;
  for (size_t i = 0; in && in[i] && j + 1 < outSize; i++) {
    char c = in[i];
    if (c >= 'A' && c <= 'Z') {
      c = c - 'A' + 'a';
    }
    out[j++] = ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) ? c : '_';
  }
  out[j] = '\0';
}

bool ha_topic(const char* base, const char* suffix, char* out, size_t outSize) {
  int n = snprintf(out, outSize, "%s%s", base ? base : "", suffix ? suffix : "");
  return n >= 0 && (size_t)n < outSize;
}

bool ha_discovery_topic(const char* uniqueId, char* out, size_t outSize) {
  int n = snprintf(out, outSize, HA_DISCOVERY_PREFIX "/device/%s/config", uniqueId ? uniqueId : "");
  return n >= 0 && (size_t)n < outSize;
}

static JsonObject AddComponent(JsonObject cmps, const char* key, const char* platform, const char* uniqueId) {
  JsonObject c = cmps.createNestedObject(key);
  c["p"] = platform;
  // Copied: the id is built in a local buffer
  char id[64];
  snprintf(id, sizeof(id), "%s_%s", uniqueId, key);
  c["uniq_id"] = (char*) id;
  return c;
}

size_t ha_build_device_discovery(const HaDeviceInfo* dev, char* out, size_t outSize) {
  DynamicJsonDocument doc(DISCOVERY_DOC_SIZE);
  char availability[96], action[96], update[96], telemetry[96];
  ha_topic(dev->baseTopic, HA_AVAILABILITY_SUFFIX, availability, sizeof(availability));
  ha_topic(dev->baseTopic, HA_ACTION_SUFFIX, action, sizeof(action));
  ha_topic(dev->baseTopic, HA_UPDATE_SUFFIX, update, sizeof(update));
  ha_topic(dev->baseTopic, HA_TELEMETRY_SUFFIX, telemetry, sizeof(telemetry));

  JsonObject device = doc.createNestedObject("dev");
  device["ids"].add(dev->uniqueId);
  device["name"] = dev->name;
  device["mf"] = "ITEAD Sonoff";
  device["mdl"] = dev->ledstrip ? "Basic R1 - LedStrip" : "Basic R1 - Switch";
  device["sw"] = dev->fwVersion;
  if (dev->configUrl && *dev->configUrl) {
    device["cu"] = dev->configUrl;
  }

  JsonObject origin = doc.createNestedObject("o");
  origin["name"] = ORIGIN_NAME;
  origin["sw"] = dev->fwVersion;
  origin["url"] = ORIGIN_URL;

  // Shared by all the components
  doc["avty_t"] = (char*) availability;
  doc["qos"] = 0;

  JsonObject cmps = doc.createNestedObject("cmps");

  if (dev->ledstrip) {
    JsonObject light = AddComponent(cmps, "light", "light", dev->uniqueId);
    light["name"] = nullptr;          // Main feature: takes the device name
    light["schema"] = "json";
    light["cmd_t"] = dev->commandTopic;
    light["stat_t"] = dev->stateTopic;
    light["sup_clrm"].add("rgb");
    light["brightness"] = true;
    light["effect"] = true;
    JsonArray effects = light.createNestedArray("fx_list");
    for (int i = 0; i < EFFECTS_NUM; i++) {
      effects.add(effect_name(i));
    }
    light["flash"] = false;
    light["transition"] = false;
    // Removes the component of the other mode (after a mode change)
    cmps.createNestedObject("switch")["p"] = "switch";
  } else {
    JsonObject sw = AddComponent(cmps, "switch", "switch", dev->uniqueId);
    sw["name"] = nullptr;
    sw["cmd_t"] = dev->commandTopic;
    sw["stat_t"] = dev->stateTopic;
    sw["val_tpl"] = "{{ value_json.state }}";
    sw["pl_on"] = "ON";
    sw["pl_off"] = "OFF";
    sw["dev_cla"] = "switch";
    cmps.createNestedObject("light")["p"] = "light";
  }

  JsonObject restart = AddComponent(cmps, "restart", "button", dev->uniqueId);
  restart["name"] = "Restart";
  restart["cmd_t"] = (char*) action;
  restart["pl_prs"] = HA_ACTION_RESTART_PAYLOAD;
  restart["dev_cla"] = "restart";
  restart["ent_cat"] = "config";

  JsonObject check = AddComponent(cmps, "check_update", "button", dev->uniqueId);
  check["name"] = "Check firmware update";
  check["cmd_t"] = (char*) action;
  check["pl_prs"] = HA_ACTION_CHECK_PAYLOAD;
  check["dev_cla"] = "update";
  check["ent_cat"] = "diagnostic";

  JsonObject firmware = AddComponent(cmps, "firmware", "update", dev->uniqueId);
  firmware["name"] = "Firmware";
  firmware["stat_t"] = (char*) update;
  firmware["cmd_t"] = (char*) action;
  firmware["pl_inst"] = HA_ACTION_INSTALL_PAYLOAD;
  firmware["dev_cla"] = "firmware";
  firmware["ent_cat"] = "config";
  firmware["title"] = ORIGIN_NAME;

  JsonObject rssi = AddComponent(cmps, "rssi", "sensor", dev->uniqueId);
  rssi["name"] = "WiFi signal";
  rssi["stat_t"] = (char*) telemetry;
  rssi["val_tpl"] = "{{ value_json.rssi }}";
  rssi["unit_of_meas"] = "dBm";
  rssi["dev_cla"] = "signal_strength";
  rssi["stat_cla"] = "measurement";
  rssi["ent_cat"] = "diagnostic";

  JsonObject ip = AddComponent(cmps, "ip", "sensor", dev->uniqueId);
  ip["name"] = "IP address";
  ip["stat_t"] = (char*) telemetry;
  ip["val_tpl"] = "{{ value_json.ip }}";
  ip["ic"] = "mdi:ip-network";
  ip["ent_cat"] = "diagnostic";

  if (doc.overflowed() || measureJson(doc) + 1 > outSize) {
    return 0;
  }
  return serializeJson(doc, out, outSize);
}

size_t ha_build_update_state(const char* installed, const char* latest, bool inProgress, const char* summary, char* out, size_t outSize) {
  StaticJsonDocument<512> doc;
  doc["installed_version"] = installed;
  // Never checked: the installed version is the latest known one
  doc["latest_version"] = (latest && *latest) ? latest : installed;
  doc["in_progress"] = inProgress;
  if (summary && *summary) {
    doc["release_summary"] = summary;
  }
  if (measureJson(doc) + 1 > outSize) {
    return 0;
  }
  return serializeJson(doc, out, outSize);
}

size_t ha_build_telemetry(int rssi, const char* ip, unsigned long uptimeSeconds, char* out, size_t outSize) {
  StaticJsonDocument<256> doc;
  doc["rssi"] = rssi;
  doc["ip"] = ip;
  doc["uptime"] = uptimeSeconds;
  if (measureJson(doc) + 1 > outSize) {
    return 0;
  }
  return serializeJson(doc, out, outSize);
}

static bool PayloadEquals(const char* payload, size_t length, const char* value) {
  return payload && length == strlen(value) && strncmp(payload, value, length) == 0;
}

HaAction ha_parse_action(const char* payload, size_t length) {
  if (PayloadEquals(payload, length, HA_ACTION_RESTART_PAYLOAD)) return HA_ACTION_RESTART;
  if (PayloadEquals(payload, length, HA_ACTION_CHECK_PAYLOAD)) return HA_ACTION_CHECK_UPDATE;
  if (PayloadEquals(payload, length, HA_ACTION_INSTALL_PAYLOAD)) return HA_ACTION_INSTALL;
  return HA_ACTION_NONE;
}

bool ha_is_birth_message(const char* payload, size_t length) {
  return PayloadEquals(payload, length, HA_PAYLOAD_ONLINE);
}
