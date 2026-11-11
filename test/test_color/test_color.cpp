#include <unity.h>
#include <stdlib.h>
#include "ColorLogic.h"

void setUp() {}
void tearDown() {}

static void test_color_from_hex() {
  Rgb c = {1, 2, 3};
  TEST_ASSERT_TRUE(color_from_hex("#ff8000", &c));
  TEST_ASSERT_EQUAL_UINT8(255, c.r);
  TEST_ASSERT_EQUAL_UINT8(128, c.g);
  TEST_ASSERT_EQUAL_UINT8(0, c.b);
  TEST_ASSERT_TRUE(color_from_hex("#3289A8", &c));
  TEST_ASSERT_EQUAL_UINT8(0x32, c.r);
  TEST_ASSERT_EQUAL_UINT8(0x89, c.g);
  TEST_ASSERT_EQUAL_UINT8(0xa8, c.b);
}

static void test_color_from_hex_invalid_keeps_color() {
  Rgb c = {1, 2, 3};
  TEST_ASSERT_FALSE(color_from_hex("ff8000", &c));
  TEST_ASSERT_FALSE(color_from_hex("#ff80", &c));
  TEST_ASSERT_FALSE(color_from_hex("#ff800000", &c));
  TEST_ASSERT_FALSE(color_from_hex("#gg8000", &c));
  TEST_ASSERT_FALSE(color_from_hex(NULL, &c));
  TEST_ASSERT_EQUAL_UINT8(1, c.r);
  TEST_ASSERT_EQUAL_UINT8(2, c.g);
  TEST_ASSERT_EQUAL_UINT8(3, c.b);
}

static void test_color_hex_round_trip() {
  char hex[8];
  for (int v = 0; v < 256; v += 17) {
    Rgb in = {(uint8_t)v, (uint8_t)(255 - v), (uint8_t)(v / 2)};
    Rgb out = {0, 0, 0};
    color_to_hex(in, hex);
    TEST_ASSERT_TRUE(color_from_hex(hex, &out));
    TEST_ASSERT_EQUAL_UINT8(in.r, out.r);
    TEST_ASSERT_EQUAL_UINT8(in.g, out.g);
    TEST_ASSERT_EQUAL_UINT8(in.b, out.b);
  }
  color_to_hex({0, 10, 255}, hex);
  TEST_ASSERT_EQUAL_STRING("#000aff", hex);
}

static void test_effect_names() {
  TEST_ASSERT_EQUAL_INT(EFFECT_SOLID, effect_from_name("Solid"));
  TEST_ASSERT_EQUAL_INT(EFFECT_RANDOM, effect_from_name("Random"));
  TEST_ASSERT_EQUAL_INT(-1, effect_from_name("random"));
  TEST_ASSERT_EQUAL_INT(-1, effect_from_name(""));
  TEST_ASSERT_EQUAL_INT(-1, effect_from_name(NULL));
  TEST_ASSERT_EQUAL_STRING("Random", effect_name(EFFECT_RANDOM));
  TEST_ASSERT_EQUAL_STRING("Solid", effect_name(99));
}

static void test_channel_step() {
  bool reached;
  TEST_ASSERT_EQUAL_UINT8(13, color_channel_step(10, 255, 3, &reached));
  TEST_ASSERT_FALSE(reached);
  TEST_ASSERT_EQUAL_UINT8(7, color_channel_step(10, 0, 3, &reached));
  // Never overshoots and no byte overflow
  TEST_ASSERT_EQUAL_UINT8(255, color_channel_step(254, 255, 3, &reached));
  TEST_ASSERT_EQUAL_UINT8(0, color_channel_step(2, 0, 3, &reached));
  TEST_ASSERT_EQUAL_UINT8(127, color_channel_step(129, 127, 3, &reached));
  TEST_ASSERT_FALSE(reached);
  TEST_ASSERT_EQUAL_UINT8(127, color_channel_step(127, 127, 3, &reached));
  TEST_ASSERT_TRUE(reached);
}

// Deterministic random sequences
static uint8_t alwaysMinRed(uint8_t n) { (void)n; return 3; }
static uint8_t pseudoRandom(uint8_t n) { return (uint8_t)(rand() % n); }

static void test_next_step_never_switches_off_strip() {
  // Two channels already off: "min" steps are never chosen
  Rgb onlyRed = {200, 0, 0};
  // The random function always proposes "red to 0": fallback to "green to max"
  TEST_ASSERT_EQUAL_UINT8(1, random_effect_next_step(onlyRed, alwaysMinRed));
  Rgb onlyBlue = {0, 0, 9};
  TEST_ASSERT_EQUAL_UINT8(0, random_effect_next_step(onlyBlue, alwaysMinRed));
  // A "min" step is allowed when at least two channels are on
  Rgb white = {255, 255, 255};
  TEST_ASSERT_EQUAL_UINT8(3, random_effect_next_step(white, alwaysMinRed));
}

static void test_random_effect_long_run() {
  srand(1234);
  Rgb c = {255, 0, 0};
  uint8_t step = 0;
  bool changed = false;
  for (int i = 0; i < 20000; i++) {
    Rgb before = c;
    random_effect_tick(&step, &c, 3, pseudoRandom);
    TEST_ASSERT_TRUE(step < RANDOM_EFFECT_STEPS);
    // The strip is never completely off
    TEST_ASSERT_FALSE(c.r == 0 && c.g == 0 && c.b == 0);
    if (before.r != c.r || before.g != c.g || before.b != c.b) {
      changed = true;
    }
  }
  TEST_ASSERT_TRUE(changed);
}

static void test_random_effect_invalid_step_recovers() {
  Rgb c = {10, 10, 10};
  uint8_t step = 200;
  random_effect_tick(&step, &c, 3, pseudoRandom);
  TEST_ASSERT_TRUE(step < RANDOM_EFFECT_STEPS);
}

int runUnityTests() {
  UNITY_BEGIN();
  RUN_TEST(test_color_from_hex);
  RUN_TEST(test_color_from_hex_invalid_keeps_color);
  RUN_TEST(test_color_hex_round_trip);
  RUN_TEST(test_effect_names);
  RUN_TEST(test_channel_step);
  RUN_TEST(test_next_step_never_switches_off_strip);
  RUN_TEST(test_random_effect_long_run);
  RUN_TEST(test_random_effect_invalid_step_recovers);
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
