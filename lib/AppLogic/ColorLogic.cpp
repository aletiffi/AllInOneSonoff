#include "ColorLogic.h"
#include <string.h>
#include <stdio.h>

static const char* const EffectNames[EFFECTS_NUM] = {"Solid", "Random"};

// Steps 0,3,6 -> red; 1,4,7 -> green; 2,5,8 -> blue
static const uint8_t RandomTargets[RANDOM_EFFECT_STEPS] = {255, 255, 255, 0, 0, 0, 127, 127, 127};

static int HexDigit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

bool color_from_hex(const char* hex, Rgb* rgb) {
  if (!hex || !rgb || strlen(hex) != 7 || hex[0] != '#') {
    return false;
  }
  uint8_t v[3];
  for (int i = 0; i < 3; i++) {
    int hi = HexDigit(hex[1 + i * 2]);
    int lo = HexDigit(hex[2 + i * 2]);
    if (hi < 0 || lo < 0) {
      return false;
    }
    v[i] = (uint8_t)(hi * 16 + lo);
  }
  rgb->r = v[0];
  rgb->g = v[1];
  rgb->b = v[2];
  return true;
}

void color_to_hex(Rgb rgb, char* out) {
  snprintf(out, 8, "#%02x%02x%02x", rgb.r, rgb.g, rgb.b);
}

int effect_from_name(const char* name) {
  for (int i = 0; name && i < EFFECTS_NUM; i++) {
    if (strcmp(name, EffectNames[i]) == 0) {
      return i;
    }
  }
  return -1;
}

const char* effect_name(int effect) {
  return (effect >= 0 && effect < EFFECTS_NUM) ? EffectNames[effect] : EffectNames[EFFECT_SOLID];
}

uint8_t color_channel_step(uint8_t value, uint8_t target, uint8_t step, bool* reached) {
  *reached = false;
  if (value < target) {
    return (target - value) < step ? target : value + step;
  }
  if (value > target) {
    return (value - target) < step ? target : value - step;
  }
  *reached = true;
  return value;
}

uint8_t random_effect_next_step(Rgb c, RandomFn rnd) {
  bool twoChannelsOff = (c.r == 0 && c.g == 0) || (c.r == 0 && c.b == 0) || (c.g == 0 && c.b == 0);
  uint8_t sel = rnd(RANDOM_EFFECT_STEPS) % RANDOM_EFFECT_STEPS;
  // A bounded number of attempts, then the first "max" step of a channel that is off
  for (int attempts = 0; RandomTargets[sel] == 0 && twoChannelsOff && attempts < 50; attempts++) {
    sel = rnd(RANDOM_EFFECT_STEPS) % RANDOM_EFFECT_STEPS;
  }
  if (RandomTargets[sel] == 0 && twoChannelsOff) {
    sel = c.r == 0 ? 0 : (c.g == 0 ? 1 : 2);
  }
  return sel;
}

void random_effect_tick(uint8_t* step, Rgb* color, uint8_t incStep, RandomFn rnd) {
  if (*step >= RANDOM_EFFECT_STEPS) {
    *step = 0;
  }
  uint8_t target = RandomTargets[*step];
  uint8_t* channel = (*step % 3 == 0) ? &color->r : ((*step % 3 == 1) ? &color->g : &color->b);
  bool reached;
  *channel = color_channel_step(*channel, target, incStep, &reached);
  if (reached) {
    *step = random_effect_next_step(*color, rnd);
  }
}
