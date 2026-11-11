#include <FastLED.h>
#include <ColorLogic.h>
#include "LedStrip.h"
#include "Config.h"
#include "Settings.h"
#include "Device.h"
#include "Mqtt.h"

static CRGB leds[NUM_LEDS];
static bool ledStripStarted          = false;
static int effectSelected            = EFFECT_SOLID;
static uint8_t randomEffectStep      = 0;
static unsigned long lastTimeRgbPlay = 0;

static uint8_t RandomByte(uint8_t n) {
  return (uint8_t) random(0, n);
}

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

static void RgbEffectPlay() {
  if (effectSelected != EFFECT_RANDOM) {
    return;
  }
  Rgb color = {Red.Val, Green.Val, Blue.Val};
  random_effect_tick(&randomEffectStep, &color, INCREMENT_STEP, RandomByte);
  Red.Val = color.r;
  Green.Val = color.g;
  Blue.Val = color.b;
  SetLedStrip(Red.Val, Green.Val, Blue.Val, Brightness.Val);
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
  Rgb rgb;
  if (!color_from_hex(color.c_str(), &rgb)) {
    return false;
  }
  Red.Val = rgb.r;
  Green.Val = rgb.g;
  Blue.Val = rgb.b;
  return true;
}

String LedStrip_ColorHex() {
  char color[8];
  color_to_hex({Red.Val, Green.Val, Blue.Val}, color);
  return String(color);
}

bool LedStrip_SelectEffect(const String& effect) {
  int e = effect_from_name(effect.c_str());
  if (e < 0) {
    return false;
  }
  effectSelected = e;
  return true;
}

int LedStrip_Effect() {
  return effectSelected;
}

String LedStrip_EffectName() {
  return effect_name(effectSelected);
}
