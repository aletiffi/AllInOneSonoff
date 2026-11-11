#include <unity.h>
#include <string.h>
#include "MqttLogic.h"

void setUp() {}
void tearDown() {}

static bool Parse(const char* json, bool ledstrip, LightCommand* cmd) {
  return mqtt_parse_command(json, strlen(json), ledstrip, cmd);
}

static void test_switch_commands() {
  LightCommand cmd;
  TEST_ASSERT_TRUE(Parse("{\"state\":\"ON\"}", false, &cmd));
  TEST_ASSERT_TRUE(cmd.hasState);
  TEST_ASSERT_TRUE(cmd.on);
  TEST_ASSERT_TRUE(Parse("{\"state\":\"OFF\"}", false, &cmd));
  TEST_ASSERT_TRUE(cmd.hasState);
  TEST_ASSERT_FALSE(cmd.on);
  // Unknown state is ignored
  TEST_ASSERT_TRUE(Parse("{\"state\":\"on\"}", false, &cmd));
  TEST_ASSERT_FALSE(cmd.hasState);
}

static void test_switch_mode_ignores_light_fields() {
  LightCommand cmd;
  TEST_ASSERT_TRUE(Parse("{\"state\":\"ON\",\"brightness\":10,\"color\":{\"r\":1,\"g\":2,\"b\":3},\"effect\":\"Random\"}", false, &cmd));
  TEST_ASSERT_TRUE(cmd.hasState);
  TEST_ASSERT_FALSE(cmd.hasBrightness);
  TEST_ASSERT_FALSE(cmd.hasColor);
  TEST_ASSERT_FALSE(cmd.hasEffect);
}

static void test_ledstrip_full_command() {
  LightCommand cmd;
  TEST_ASSERT_TRUE(Parse("{\"state\":\"ON\",\"brightness\":120,\"color\":{\"r\":255,\"g\":0,\"b\":80},\"effect\":\"Random\"}", true, &cmd));
  TEST_ASSERT_TRUE(cmd.hasState && cmd.on);
  TEST_ASSERT_TRUE(cmd.hasBrightness);
  TEST_ASSERT_EQUAL_UINT8(120, cmd.brightness);
  TEST_ASSERT_TRUE(cmd.hasColor);
  TEST_ASSERT_EQUAL_UINT8(255, cmd.color.r);
  TEST_ASSERT_EQUAL_UINT8(0, cmd.color.g);
  TEST_ASSERT_EQUAL_UINT8(80, cmd.color.b);
  TEST_ASSERT_TRUE(cmd.hasEffect);
  TEST_ASSERT_EQUAL_INT(EFFECT_RANDOM, cmd.effect);
}

static void test_ledstrip_partial_and_invalid_fields() {
  LightCommand cmd;
  // Only brightness: out of range values are clamped
  TEST_ASSERT_TRUE(Parse("{\"brightness\":300}", true, &cmd));
  TEST_ASSERT_FALSE(cmd.hasState);
  TEST_ASSERT_TRUE(cmd.hasBrightness);
  TEST_ASSERT_EQUAL_UINT8(255, cmd.brightness);
  TEST_ASSERT_TRUE(Parse("{\"brightness\":-5}", true, &cmd));
  TEST_ASSERT_EQUAL_UINT8(0, cmd.brightness);
  // Incomplete color and unknown effect are ignored
  TEST_ASSERT_TRUE(Parse("{\"color\":{\"r\":1,\"g\":2},\"effect\":\"Rainbow\"}", true, &cmd));
  TEST_ASSERT_FALSE(cmd.hasColor);
  TEST_ASSERT_FALSE(cmd.hasEffect);
  TEST_ASSERT_TRUE(Parse("{\"brightness\":\"high\"}", true, &cmd));
  TEST_ASSERT_FALSE(cmd.hasBrightness);
}

static void test_plain_text_commands() {
  LightCommand cmd;
  TEST_ASSERT_TRUE(Parse("ON", false, &cmd));
  TEST_ASSERT_TRUE(cmd.hasState && cmd.on);
  TEST_ASSERT_TRUE(Parse("off", true, &cmd));
  TEST_ASSERT_TRUE(cmd.hasState);
  TEST_ASSERT_FALSE(cmd.on);
  TEST_ASSERT_TRUE(Parse("TOGGLE", false, &cmd));
  TEST_ASSERT_TRUE(cmd.toggle);
  TEST_ASSERT_FALSE(cmd.hasState);
  TEST_ASSERT_FALSE(Parse("ONN", false, &cmd));
  TEST_ASSERT_FALSE(Parse("1", false, &cmd));
}

static void test_home_assistant_commands() {
  // Messages sent by the Home Assistant json light
  LightCommand cmd;
  TEST_ASSERT_TRUE(Parse("{\"state\":\"ON\",\"brightness\":128}", true, &cmd));
  TEST_ASSERT_TRUE(cmd.on && cmd.hasBrightness && !cmd.hasColor);
  TEST_ASSERT_TRUE(Parse("{\"state\":\"ON\",\"color\":{\"r\":10,\"g\":20,\"b\":30}}", true, &cmd));
  TEST_ASSERT_TRUE(cmd.on && cmd.hasColor && !cmd.hasBrightness);
  TEST_ASSERT_TRUE(Parse("{\"state\":\"ON\",\"effect\":\"Random\"}", true, &cmd));
  TEST_ASSERT_TRUE(cmd.on && cmd.hasEffect);
  TEST_ASSERT_TRUE(Parse("{\"state\":\"OFF\"}", true, &cmd));
  TEST_ASSERT_TRUE(cmd.hasState && !cmd.on);
}

static void test_invalid_json() {
  LightCommand cmd;
  TEST_ASSERT_FALSE(Parse("ONE", false, &cmd));
  TEST_ASSERT_FALSE(Parse("{\"state\":", true, &cmd));
  TEST_ASSERT_FALSE(Parse("", true, &cmd));
  TEST_ASSERT_FALSE(cmd.hasState);
  // Payload not null terminated (as received by PubSubClient)
  const char raw[] = "{\"state\":\"ON\"}garbage";
  TEST_ASSERT_TRUE(mqtt_parse_command(raw, 14, false, &cmd));
  TEST_ASSERT_TRUE(cmd.hasState && cmd.on);
}

static void test_build_state() {
  char out[256];
  LightState st = {false, false, 0, {0, 0, 0}, EFFECT_SOLID};
  TEST_ASSERT_TRUE(mqtt_build_state(&st, out, sizeof(out)) > 0);
  TEST_ASSERT_EQUAL_STRING("{\"state\":\"OFF\"}", out);

  st.on = true;
  mqtt_build_state(&st, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("{\"state\":\"ON\"}", out);

  LightState led = {true, true, 85, {255, 0, 80}, EFFECT_RANDOM};
  mqtt_build_state(&led, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("{\"state\":\"ON\",\"color_mode\":\"rgb\",\"brightness\":85,\"effect\":\"Random\",\"color\":{\"r\":255,\"g\":0,\"b\":80}}", out);

  // Ledstrip off: only the state
  led.on = false;
  mqtt_build_state(&led, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("{\"state\":\"OFF\"}", out);
}

static void test_build_state_round_trip() {
  char out[256];
  LightCommand cmd;
  LightState led = {true, true, 200, {12, 34, 56}, EFFECT_SOLID};
  mqtt_build_state(&led, out, sizeof(out));
  TEST_ASSERT_TRUE(Parse(out, true, &cmd));
  TEST_ASSERT_TRUE(cmd.on);
  TEST_ASSERT_EQUAL_UINT8(200, cmd.brightness);
  TEST_ASSERT_EQUAL_UINT8(34, cmd.color.g);
  TEST_ASSERT_EQUAL_INT(EFFECT_SOLID, cmd.effect);
}

static void test_build_state_buffer_too_small() {
  char out[10];
  LightState led = {true, true, 85, {255, 0, 80}, EFFECT_RANDOM};
  TEST_ASSERT_EQUAL_UINT(0, mqtt_build_state(&led, out, sizeof(out)));
}

int runUnityTests() {
  UNITY_BEGIN();
  RUN_TEST(test_switch_commands);
  RUN_TEST(test_switch_mode_ignores_light_fields);
  RUN_TEST(test_ledstrip_full_command);
  RUN_TEST(test_ledstrip_partial_and_invalid_fields);
  RUN_TEST(test_plain_text_commands);
  RUN_TEST(test_home_assistant_commands);
  RUN_TEST(test_invalid_json);
  RUN_TEST(test_build_state);
  RUN_TEST(test_build_state_round_trip);
  RUN_TEST(test_build_state_buffer_too_small);
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
