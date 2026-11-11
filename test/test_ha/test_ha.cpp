#include <unity.h>
#include <string.h>
#include <ArduinoJson.h>
#include "HaDiscovery.h"

// Same size used by the firmware for the MQTT buffer
#define MQTT_BUFFER 2048

void setUp() {}
void tearDown() {}

static HaDeviceInfo Device(bool ledstrip) {
  HaDeviceInfo dev = {"sonoff_1a2b3c", "Cucina", "sonoff/cucina", "sonoff/cucina/set", "sonoff/cucina/state",
                      ledstrip, "45.4.0.0", "http://192.168.1.20/"};
  return dev;
}

static DynamicJsonDocument Discovery(bool ledstrip) {
  static char out[MQTT_BUFFER];
  HaDeviceInfo dev = Device(ledstrip);
  size_t len = ha_build_device_discovery(&dev, out, sizeof(out));
  TEST_ASSERT_TRUE_MESSAGE(len > 0, "discovery does not fit in the MQTT buffer");
  TEST_ASSERT_EQUAL_size_t(strlen(out), len);
  DynamicJsonDocument doc(8192);
  TEST_ASSERT_FALSE(deserializeJson(doc, out));
  return doc;
}

static void test_sanitize_id() {
  char out[32];
  ha_sanitize_id("Luce Cucina-1", out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("luce_cucina_1", out);
  ha_sanitize_id("SonoffAllInOne", out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("sonoffallinone", out);
  ha_sanitize_id("a/b+c#", out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("a_b_c_", out);
  char small[4];
  ha_sanitize_id("abcdef", small, sizeof(small));
  TEST_ASSERT_EQUAL_STRING("abc", small);
  ha_sanitize_id(NULL, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("", out);
}

static void test_topics() {
  char out[64];
  TEST_ASSERT_TRUE(ha_topic("sonoff/cucina", HA_AVAILABILITY_SUFFIX, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("sonoff/cucina/availability", out);
  TEST_ASSERT_TRUE(ha_discovery_topic("sonoff_1a2b3c", out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("homeassistant/device/sonoff_1a2b3c/config", out);
  char small[8];
  TEST_ASSERT_FALSE(ha_topic("sonoff/cucina", "/set", small, sizeof(small)));
}

static void test_discovery_device_and_origin() {
  DynamicJsonDocument doc = Discovery(false);
  // Required by device discovery: dev, o (with name), cmps
  TEST_ASSERT_EQUAL_STRING("sonoff_1a2b3c", doc["dev"]["ids"][0]);
  TEST_ASSERT_EQUAL_STRING("Cucina", doc["dev"]["name"]);
  TEST_ASSERT_EQUAL_STRING("45.4.0.0", doc["dev"]["sw"]);
  TEST_ASSERT_EQUAL_STRING("http://192.168.1.20/", doc["dev"]["cu"]);
  TEST_ASSERT_EQUAL_STRING("AllInOneSonoff", doc["o"]["name"]);
  TEST_ASSERT_EQUAL_STRING("sonoff/cucina/availability", doc["avty_t"]);
  TEST_ASSERT_TRUE(doc["cmps"].is<JsonObject>());
}

static void test_discovery_unique_ids() {
  DynamicJsonDocument doc = Discovery(true);
  JsonObject cmps = doc["cmps"];
  int entities = 0;
  for (JsonPair a : cmps) {
    TEST_ASSERT_TRUE(a.value()["p"].is<const char*>());
    const char* idA = a.value()["uniq_id"];
    if (!idA) {
      continue;   // Component being removed (only "p")
    }
    entities++;
    TEST_ASSERT_EQUAL(0, strncmp(idA, "sonoff_1a2b3c_", 14));
    for (JsonPair b : cmps) {
      const char* idB = b.value()["uniq_id"];
      if (idB && a.key() != b.key()) {
        TEST_ASSERT_TRUE_MESSAGE(strcmp(idA, idB) != 0, "duplicated unique_id");
      }
    }
  }
  TEST_ASSERT_EQUAL_INT(6, entities);
}

static void test_discovery_switch_mode() {
  DynamicJsonDocument doc = Discovery(false);
  JsonObject sw = doc["cmps"]["switch"];
  TEST_ASSERT_EQUAL_STRING("switch", sw["p"]);
  TEST_ASSERT_TRUE(sw["name"].isNull());
  TEST_ASSERT_TRUE(sw.containsKey("name"));
  TEST_ASSERT_EQUAL_STRING("sonoff/cucina/set", sw["cmd_t"]);
  TEST_ASSERT_EQUAL_STRING("sonoff/cucina/state", sw["stat_t"]);
  TEST_ASSERT_EQUAL_STRING("{{ value_json.state }}", sw["val_tpl"]);
  TEST_ASSERT_EQUAL_STRING("ON", sw["pl_on"]);
  TEST_ASSERT_EQUAL_STRING("OFF", sw["pl_off"]);
  // The light of the other mode is removed (only the platform)
  JsonObject light = doc["cmps"]["light"];
  TEST_ASSERT_EQUAL_STRING("light", light["p"]);
  TEST_ASSERT_EQUAL(1, light.size());
}

static void test_discovery_ledstrip_mode() {
  DynamicJsonDocument doc = Discovery(true);
  JsonObject light = doc["cmps"]["light"];
  TEST_ASSERT_EQUAL_STRING("light", light["p"]);
  TEST_ASSERT_EQUAL_STRING("json", light["schema"]);
  TEST_ASSERT_TRUE(light["name"].isNull());
  TEST_ASSERT_EQUAL_STRING("sonoff/cucina/set", light["cmd_t"]);
  TEST_ASSERT_EQUAL_STRING("sonoff/cucina/state", light["stat_t"]);
  // Current color modes api (no deprecated "rgb"/"color_mode" flags)
  TEST_ASSERT_EQUAL(1, light["sup_clrm"].size());
  TEST_ASSERT_EQUAL_STRING("rgb", light["sup_clrm"][0]);
  TEST_ASSERT_FALSE(light.containsKey("rgb"));
  TEST_ASSERT_FALSE(light.containsKey("color_mode"));
  TEST_ASSERT_TRUE(light["brightness"].as<bool>());
  TEST_ASSERT_TRUE(light["effect"].as<bool>());
  TEST_ASSERT_EQUAL_STRING("Solid", light["fx_list"][0]);
  TEST_ASSERT_EQUAL_STRING("Random", light["fx_list"][1]);
  TEST_ASSERT_FALSE(light["flash"].as<bool>());
  TEST_ASSERT_FALSE(light["transition"].as<bool>());
  TEST_ASSERT_EQUAL(1, doc["cmps"]["switch"].size());
  TEST_ASSERT_EQUAL_STRING("Basic R1 - LedStrip", doc["dev"]["mdl"]);
}

static void test_discovery_buttons_update_sensors() {
  DynamicJsonDocument doc = Discovery(false);
  JsonObject cmps = doc["cmps"];
  TEST_ASSERT_EQUAL_STRING("button", cmps["restart"]["p"]);
  TEST_ASSERT_EQUAL_STRING("sonoff/cucina/action", cmps["restart"]["cmd_t"]);
  TEST_ASSERT_EQUAL_STRING("restart", cmps["restart"]["pl_prs"]);
  TEST_ASSERT_EQUAL_STRING("restart", cmps["restart"]["dev_cla"]);
  TEST_ASSERT_EQUAL_STRING("check_update", cmps["check_update"]["pl_prs"]);
  TEST_ASSERT_EQUAL_STRING("update", cmps["firmware"]["p"]);
  TEST_ASSERT_EQUAL_STRING("sonoff/cucina/update", cmps["firmware"]["stat_t"]);
  TEST_ASSERT_EQUAL_STRING("install", cmps["firmware"]["pl_inst"]);
  TEST_ASSERT_EQUAL_STRING("firmware", cmps["firmware"]["dev_cla"]);
  TEST_ASSERT_EQUAL_STRING("sensor", cmps["rssi"]["p"]);
  TEST_ASSERT_EQUAL_STRING("signal_strength", cmps["rssi"]["dev_cla"]);
  TEST_ASSERT_EQUAL_STRING("dBm", cmps["rssi"]["unit_of_meas"]);
  TEST_ASSERT_EQUAL_STRING("sonoff/cucina/telemetry", cmps["ip"]["stat_t"]);
  // Actions parsed by the firmware match the payloads announced to Home Assistant
  const char* restart = cmps["restart"]["pl_prs"];
  const char* check = cmps["check_update"]["pl_prs"];
  const char* install = cmps["firmware"]["pl_inst"];
  TEST_ASSERT_EQUAL_INT(HA_ACTION_RESTART, ha_parse_action(restart, strlen(restart)));
  TEST_ASSERT_EQUAL_INT(HA_ACTION_CHECK_UPDATE, ha_parse_action(check, strlen(check)));
  TEST_ASSERT_EQUAL_INT(HA_ACTION_INSTALL, ha_parse_action(install, strlen(install)));
}

static void test_discovery_without_config_url() {
  char out[MQTT_BUFFER];
  HaDeviceInfo dev = Device(false);
  dev.configUrl = "";
  TEST_ASSERT_TRUE(ha_build_device_discovery(&dev, out, sizeof(out)) > 0);
  TEST_ASSERT_NULL(strstr(out, "\"cu\""));
}

static void test_discovery_buffer_too_small() {
  char out[256];
  HaDeviceInfo dev = Device(true);
  TEST_ASSERT_EQUAL_size_t(0, ha_build_device_discovery(&dev, out, sizeof(out)));
}

static void test_update_state() {
  char out[256];
  TEST_ASSERT_TRUE(ha_build_update_state("45.4.0.0", "45.5.0.0", false, "", out, sizeof(out)) > 0);
  TEST_ASSERT_EQUAL_STRING("{\"installed_version\":\"45.4.0.0\",\"latest_version\":\"45.5.0.0\",\"in_progress\":false}", out);
  // Never checked: latest = installed (Home Assistant shows "up to date")
  ha_build_update_state("45.4.0.0", "", true, "Updating", out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("{\"installed_version\":\"45.4.0.0\",\"latest_version\":\"45.4.0.0\",\"in_progress\":true,\"release_summary\":\"Updating\"}", out);
}

static void test_telemetry() {
  char out[128];
  TEST_ASSERT_TRUE(ha_build_telemetry(-61, "192.168.1.20", 3600, out, sizeof(out)) > 0);
  TEST_ASSERT_EQUAL_STRING("{\"rssi\":-61,\"ip\":\"192.168.1.20\",\"uptime\":3600}", out);
}

static void test_actions_and_birth() {
  TEST_ASSERT_EQUAL_INT(HA_ACTION_RESTART, ha_parse_action("restart", 7));
  TEST_ASSERT_EQUAL_INT(HA_ACTION_NONE, ha_parse_action("restartx", 8));
  TEST_ASSERT_EQUAL_INT(HA_ACTION_NONE, ha_parse_action("restart", 6));
  TEST_ASSERT_EQUAL_INT(HA_ACTION_NONE, ha_parse_action(NULL, 0));
  TEST_ASSERT_TRUE(ha_is_birth_message("online", 6));
  TEST_ASSERT_FALSE(ha_is_birth_message("offline", 7));
}

int runUnityTests() {
  UNITY_BEGIN();
  RUN_TEST(test_sanitize_id);
  RUN_TEST(test_topics);
  RUN_TEST(test_discovery_device_and_origin);
  RUN_TEST(test_discovery_unique_ids);
  RUN_TEST(test_discovery_switch_mode);
  RUN_TEST(test_discovery_ledstrip_mode);
  RUN_TEST(test_discovery_buttons_update_sensors);
  RUN_TEST(test_discovery_without_config_url);
  RUN_TEST(test_discovery_buffer_too_small);
  RUN_TEST(test_update_state);
  RUN_TEST(test_telemetry);
  RUN_TEST(test_actions_and_birth);
  return UNITY_END();
}

#ifdef ARDUINO
#include <Arduino.h>
void setup() {
  delay(2000);
  runUnityTests();
}
void loop() {}
#else
int main() {
  return runUnityTests();
}
#endif
