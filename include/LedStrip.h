#ifndef LedStrip_h
#define LedStrip_h

#include <Arduino.h>

// LedStrip mode: ws2812b strip on GPIO14, color and brightness are in Settings (Red, Green, Blue, Brightness)
void LedStrip_Begin();
void LedStrip_Loop();
void LedStrip_Show();
void LedStrip_Off();
bool LedStrip_SetColorHex(const String& color);
String LedStrip_ColorHex();
bool LedStrip_SelectEffect(const String& effect);
String LedStrip_EffectName();

#endif
