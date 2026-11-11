#ifndef HaDiscovery_h
#define HaDiscovery_h

// Home Assistant MQTT integration, independent from Arduino (unit tested on PC)
// https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
//
// One "device discovery" message (homeassistant/device/<unique_id>/config) creates:
//   - Switch mode:   switch   (relay)
//   - LedStrip mode: light    (json schema, rgb + brightness + effects)
//   - button "Restart", button "Check firmware update", update "Firmware"
//   - sensor "WiFi signal", sensor "IP address" (diagnostic)
//
// Topics (base = "sonoff/<hostname>"):
//   <base>/set           commands for the switch / light (or MQTT_Sub if set)
//   <base>/state         switch / light state           (or MQTT_Pub if set)
//   <base>/availability  "online" / "offline" (retained, last will)
//   <base>/action        "restart", "check_update", "install"
//   <base>/update        firmware update state (update entity)
//   <base>/telemetry     {"rssi": -60, "ip": "192.168.1.20", "uptime": 3600}

#include <stddef.h>

#define HA_DISCOVERY_PREFIX       "homeassistant"
#define HA_STATUS_TOPIC           "homeassistant/status"
#define HA_BASE_TOPIC_PREFIX      "sonoff/"
#define HA_AVAILABILITY_SUFFIX    "/availability"
#define HA_ACTION_SUFFIX          "/action"
#define HA_UPDATE_SUFFIX          "/update"
#define HA_TELEMETRY_SUFFIX       "/telemetry"
#define HA_SET_SUFFIX             "/set"
#define HA_STATE_SUFFIX           "/state"
#define HA_PAYLOAD_ONLINE         "online"
#define HA_PAYLOAD_OFFLINE        "offline"
#define HA_ACTION_RESTART_PAYLOAD "restart"
#define HA_ACTION_CHECK_PAYLOAD   "check_update"
#define HA_ACTION_INSTALL_PAYLOAD "install"

enum HaAction {
  HA_ACTION_NONE,
  HA_ACTION_RESTART,
  HA_ACTION_CHECK_UPDATE,
  HA_ACTION_INSTALL
};

struct HaDeviceInfo {
  const char* uniqueId;       // e.g. "sonoff_1a2b3c" (from the chip id, never changes)
  const char* name;           // device name shown in Home Assistant (hostname)
  const char* baseTopic;      // e.g. "sonoff/kitchen"
  const char* commandTopic;   // switch / light commands
  const char* stateTopic;     // switch / light state
  bool ledstrip;              // LedStrip mode -> light, Switch mode -> switch
  const char* fwVersion;
  const char* configUrl;      // device web page, "" if unknown
};

// Lowercase letters, digits and '_' only: "Luce Cucina-1" -> "luce_cucina_1"
void ha_sanitize_id(const char* in, char* out, size_t outSize);

// base + suffix, returns false if out is too small
bool ha_topic(const char* base, const char* suffix, char* out, size_t outSize);

// homeassistant/device/<uniqueId>/config
bool ha_discovery_topic(const char* uniqueId, char* out, size_t outSize);

// Device discovery payload, returns its length (0 if out is too small)
size_t ha_build_device_discovery(const HaDeviceInfo* dev, char* out, size_t outSize);

// {"installed_version":..,"latest_version":..,"in_progress":..,"release_summary":..}
size_t ha_build_update_state(const char* installed, const char* latest, bool inProgress, const char* summary, char* out, size_t outSize);

// {"rssi":..,"ip":..,"uptime":..}
size_t ha_build_telemetry(int rssi, const char* ip, unsigned long uptimeSeconds, char* out, size_t outSize);

HaAction ha_parse_action(const char* payload, size_t length);

// "online" received on homeassistant/status: discovery and states must be published again
bool ha_is_birth_message(const char* payload, size_t length);

#endif
