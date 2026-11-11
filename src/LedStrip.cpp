#include <FastLED.h>
#include "LedStrip.h"
#include "Config.h"
#include "Settings.h"
#include "Device.h"
#include "Mqtt.h"

#define RGB_COMBO_NUM             9
#define RGB_EFFECTS_NUM           2
#define EFFECT_SOLID              0
#define EFFECT_RANDOM             1

static const char* EffectNames[RGB_EFFECTS_NUM] = {"Solid", "Random"};

// Random effect: steps 0,3,6 -> red; 1,4,7 -> green; 2,5,8 -> blue
static const byte RandomTargets[RGB_COMBO_NUM] = {
  MAX_RGB_VAL, MAX_RGB_VAL, MAX_RGB_VAL,
  0, 0, 0,
  MAX_RGB_VAL / 2, MAX_RGB_VAL / 2, MAX_RGB_VAL / 2};

static CRGB leds[NUM_LEDS];
static bool ledStripStarted          = false;
static byte effectSelected           = EFFECT_SOLID;
static byte randomEffectStep         = 0;
static unsigned long lastTimeRgbPlay = 0;

static void SetLedStrip(byte red, byte green, byte blue, byte brightness) {
  if (!ledStripStarted) {
    return;
  }
  fill_solid(leds, NUM_LEDS, CRGB(red, green, blue));
  FastLED.setBrightness(brightness);
  FastLED.show();
  Serial.println("LedStrip updated -> R:" + String(red) + " G:" + String(green) + " B:" + String(blue) + " Brightness:" + String(brightness));
  if (effectSelected == EFFECT_SOLID) {
    PublishState();
  }
}

static byte RandomEffectStepSelector() {
  byte randSelection = random(0, RGB_COMBO_NUM);
  // Verifies that not all channels are disabled
  while ((RandomTargets[randSelection] == 0) && ((Red.Val == 0 && Green.Val == 0) || (Red.Val == 0 && Blue.Val == 0) || (Blue.Val == 0 && Green.Val == 0))) {
    randSelection = random(0, RGB_COMBO_NUM);
  }
  return randSelection;
}

static byte RgbChannelManager(byte channelVal, byte targetVal, byte incStep) {
  if (channelVal < targetVal) {
    return (targetVal - channelVal) < incStep ? targetVal : channelVal + incStep;
  } else if (channelVal > targetVal) {
    return (channelVal - targetVal) < incStep ? targetVal : channelVal - incStep;
  }
  // Target reached: choose the next step
  randomEffectStep = RandomEffectStepSelector();
  return channelVal;
}

static void RgbEffectPlay() {
  if (effectSelected != EFFECT_RANDOM || randomEffectStep >= RGB_COMBO_NUM) {
    return;
  }
  byte target = RandomTargets[randomEffectStep];
  switch (randomEffectStep % 3) {
    case 0:
      Red.Val = RgbChannelManager(Red.Val, target, INCREMENT_STEP);
      break;
    case 1:
      Green.Val = RgbChannelManager(Green.Val, target, INCREMENT_STEP);
      break;
    case 2:
      Blue.Val = RgbChannelManager(Blue.Val, target, INCREMENT_STEP);
      break;
  }
  SetLedStrip(Red.Val, Green.Val, Blue.Val, Brightness.Val);
}

static byte HexString2Byte(const String& val) {
  return (byte) strtol(val.c_str(), NULL, 16);
}

void LedStrip_Begin() {
  pinMode(SONOFF_GPIO14, OUTPUT);
  FastLED.addLeds<WS2811, SONOFF_GPIO14, BRG>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.clear();
  FastLED.show();
  ledStripStarted = true;
  randomSeed(micros());
}

void LedStrip_Loop() {
  if (ledStripStarted && effectSelected != EFFECT_SOLID && IsOn()) {
    if (millis() - lastTimeRgbPlay >= T_250MS) {
      lastTimeRgbPlay = millis();
      RgbEffectPlay();
    }
  }
}

void LedStrip_Show() {
  SetLedStrip(Red.Val, Green.Val, Blue.Val, Brightness.Val);
}

void LedStrip_Off() {
  if (ledStripStarted) {
    FastLED.clear();
    FastLED.show();
  }
  effectSelected = EFFECT_SOLID;
}

bool LedStrip_SetColorHex(const String& color) {
  if (color.length() != 7 || color.charAt(0) != '#') {
    return false;
  }
  Red.Val = HexString2Byte(color.substring(1, 3));
  Green.Val = HexString2Byte(color.substring(3, 5));
  Blue.Val = HexString2Byte(color.substring(5, 7));
  return true;
}

String LedStrip_ColorHex() {
  char color[8];
  snprintf(color, sizeof(color), "#%02x%02x%02x", Red.Val, Green.Val, Blue.Val);
  return String(color);
}

bool LedStrip_SelectEffect(const String& effect) {
  for (byte i = 0; i < RGB_EFFECTS_NUM; i++) {
    if (effect.equals(EffectNames[i])) {
      effectSelected = i;
      return true;
    }
  }
  return false;
}

String LedStrip_EffectName() {
  return EffectNames[effectSelected];
}
