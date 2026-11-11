#include <unity.h>
#include "SettingsLogic.h"

void setUp() {}
void tearDown() {}

static void test_byte_number() {
  TEST_ASSERT_TRUE(settings_is_byte_number("0"));
  TEST_ASSERT_TRUE(settings_is_byte_number("1"));
  TEST_ASSERT_TRUE(settings_is_byte_number("255"));
  TEST_ASSERT_FALSE(settings_is_byte_number("256"));
  TEST_ASSERT_FALSE(settings_is_byte_number("1000"));
  TEST_ASSERT_FALSE(settings_is_byte_number(""));
  TEST_ASSERT_FALSE(settings_is_byte_number("-1"));
  TEST_ASSERT_FALSE(settings_is_byte_number("12a"));
  // Never written flash area
  TEST_ASSERT_FALSE(settings_is_byte_number("\xff\xff\xff"));
  TEST_ASSERT_FALSE(settings_is_byte_number(NULL));
}

static void test_parse_mode() {
  TEST_ASSERT_EQUAL_INT(LEDSTRIP_MODE, settings_parse_mode("LedStrip"));
  TEST_ASSERT_EQUAL_INT(SWITCH_MODE, settings_parse_mode("Switch"));
  // EEPROM written by an old firmware
  TEST_ASSERT_EQUAL_INT(SWITCH_MODE, settings_parse_mode(""));
  TEST_ASSERT_EQUAL_INT(SWITCH_MODE, settings_parse_mode("ledstrip"));
  TEST_ASSERT_EQUAL_INT(SWITCH_MODE, settings_parse_mode(NULL));
  TEST_ASSERT_EQUAL_STRING("LedStrip", settings_mode_name(LEDSTRIP_MODE));
  TEST_ASSERT_EQUAL_STRING("Switch", settings_mode_name(SWITCH_MODE));
  TEST_ASSERT_TRUE(settings_is_known_mode("Switch"));
  TEST_ASSERT_FALSE(settings_is_known_mode("Dimmer"));
  TEST_ASSERT_FALSE(settings_is_known_mode(""));
}

static void test_mode_change_restart() {
  TEST_ASSERT_TRUE(settings_mode_change_requires_restart("Switch", "LedStrip"));
  TEST_ASSERT_TRUE(settings_mode_change_requires_restart("LedStrip", "Switch"));
  TEST_ASSERT_TRUE(settings_mode_change_requires_restart("", "LedStrip"));
  TEST_ASSERT_FALSE(settings_mode_change_requires_restart("", "Switch"));
  TEST_ASSERT_FALSE(settings_mode_change_requires_restart("LedStrip", "LedStrip"));
}

static void test_sanitize() {
  char out[32];
  TEST_ASSERT_EQUAL_INT(SETTING_SET, settings_sanitize("  MyWifi \n", false, 20, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("MyWifi", out);
  // Spaces inside are kept (e.g. ssid with spaces)
  TEST_ASSERT_EQUAL_INT(SETTING_SET, settings_sanitize("My Wifi", false, 20, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("My Wifi", out);
  TEST_ASSERT_EQUAL_INT(SETTING_KEEP, settings_sanitize("", false, 20, out, sizeof(out)));
  TEST_ASSERT_EQUAL_INT(SETTING_KEEP, settings_sanitize("   ", false, 20, out, sizeof(out)));
  TEST_ASSERT_EQUAL_INT(SETTING_KEEP, settings_sanitize(NULL, false, 20, out, sizeof(out)));
}

static void test_sanitize_clear() {
  char out[32];
  TEST_ASSERT_EQUAL_INT(SETTING_CLEAR, settings_sanitize("-", true, 20, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("", out);
  TEST_ASSERT_EQUAL_INT(SETTING_CLEAR, settings_sanitize(" - ", true, 20, out, sizeof(out)));
  // "-" is a normal value when clearing is not allowed
  TEST_ASSERT_EQUAL_INT(SETTING_SET, settings_sanitize("-", false, 20, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("-", out);
}

static void test_sanitize_truncate() {
  char out[32];
  TEST_ASSERT_EQUAL_INT(SETTING_SET, settings_sanitize("0123456789ABCDEFGHIJKLMN", false, 20, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("0123456789ABCDEFGHIJ", out);
  char small[5];
  TEST_ASSERT_EQUAL_INT(SETTING_SET, settings_sanitize("abcdefgh", false, 20, small, sizeof(small)));
  TEST_ASSERT_EQUAL_STRING("abcd", small);
}

int runUnityTests() {
  UNITY_BEGIN();
  RUN_TEST(test_byte_number);
  RUN_TEST(test_parse_mode);
  RUN_TEST(test_mode_change_restart);
  RUN_TEST(test_sanitize);
  RUN_TEST(test_sanitize_clear);
  RUN_TEST(test_sanitize_truncate);
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
