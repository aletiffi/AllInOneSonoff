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

static void test_ap_password_valid() {
  TEST_ASSERT_TRUE(settings_is_valid_ap_password("12345678", 20));
  TEST_ASSERT_TRUE(settings_is_valid_ap_password("a b c d e f g h i j", 20));
  TEST_ASSERT_TRUE(settings_is_valid_ap_password("12345678901234567890", 20));
  // WPA2 minimum 8 characters
  TEST_ASSERT_FALSE(settings_is_valid_ap_password("1234567", 20));
  TEST_ASSERT_FALSE(settings_is_valid_ap_password("", 20));
  TEST_ASSERT_FALSE(settings_is_valid_ap_password(NULL, 20));
  // Longer than the setting
  TEST_ASSERT_FALSE(settings_is_valid_ap_password("123456789012345678901", 20));
  // Not printable
  TEST_ASSERT_FALSE(settings_is_valid_ap_password("1234\x02" "5678", 20));
  TEST_ASSERT_FALSE(settings_is_valid_ap_password("1234\xe8" "5678", 20));
}

static void test_ap_password_encode_decode() {
  char stored[24], pwd[24];
  TEST_ASSERT_TRUE(settings_encode_ap_password("secret12", stored, sizeof(stored)));
  TEST_ASSERT_EQUAL_CHAR(AP_PASSWORD_MARKER, stored[0]);
  TEST_ASSERT_EQUAL_STRING("secret12", stored + 1);
  TEST_ASSERT_TRUE(settings_decode_ap_password(stored, 20, pwd, sizeof(pwd)));
  TEST_ASSERT_EQUAL_STRING("secret12", pwd);

  // No password: nothing saved (open access point)
  TEST_ASSERT_TRUE(settings_encode_ap_password("", stored, sizeof(stored)));
  TEST_ASSERT_EQUAL_STRING("", stored);
  TEST_ASSERT_TRUE(settings_decode_ap_password(stored, 20, pwd, sizeof(pwd)));
  TEST_ASSERT_EQUAL_STRING("", pwd);

  // Buffers too small
  TEST_ASSERT_FALSE(settings_encode_ap_password("secret12", stored, 9));
  TEST_ASSERT_FALSE(settings_decode_ap_password("\x02secret12", 20, pwd, 8));
}

static void test_ap_password_leftovers() {
  char pwd[24];
  // Leftovers of older settings (no marker): ignored
  TEST_ASSERT_TRUE(settings_decode_ap_password("ive.google.com/uc", 20, pwd, sizeof(pwd)));
  TEST_ASSERT_EQUAL_STRING("", pwd);
  TEST_ASSERT_TRUE(settings_decode_ap_password(NULL, 20, pwd, sizeof(pwd)));
  TEST_ASSERT_EQUAL_STRING("", pwd);
  // Marker but not valid (too short or too long): ignored
  TEST_ASSERT_TRUE(settings_decode_ap_password("\x02short", 20, pwd, sizeof(pwd)));
  TEST_ASSERT_EQUAL_STRING("", pwd);
  TEST_ASSERT_TRUE(settings_decode_ap_password("\x02" "123456789012345678901", 20, pwd, sizeof(pwd)));
  TEST_ASSERT_EQUAL_STRING("", pwd);
}

int runUnityTests() {
  UNITY_BEGIN();
  RUN_TEST(test_byte_number);
  RUN_TEST(test_parse_mode);
  RUN_TEST(test_mode_change_restart);
  RUN_TEST(test_sanitize);
  RUN_TEST(test_sanitize_clear);
  RUN_TEST(test_sanitize_truncate);
  RUN_TEST(test_ap_password_valid);
  RUN_TEST(test_ap_password_encode_decode);
  RUN_TEST(test_ap_password_leftovers);
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
