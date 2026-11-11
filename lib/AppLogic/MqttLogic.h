#ifndef MqttLogic_h
#define MqttLogic_h

// MQTT json messages (Home Assistant "json" schema), independent from Arduino (unit tested on PC)

#include <stddef.h>
#include <stdint.h>
#include "ColorLogic.h"

#define ON_PAYLOAD                "ON"
#define OFF_PAYLOAD               "OFF"

struct LightCommand {
  bool hasState;
  bool on;
  bool hasBrightness;
  uint8_t brightness;
  bool hasColor;
  Rgb color;
  bool hasEffect;
  int effect;
};

struct LightState {
  bool on;
  bool ledstrip;
  uint8_t brightness;
  Rgb color;
  int effect;
};

// {"state":"ON","brightness":120,"color":{"r":255,"g":0,"b":80},"effect":"Random"}
// In Switch mode only "state" is used. Returns false if the json is not valid.
bool mqtt_parse_command(const char* payload, size_t length, bool ledstripMode, LightCommand* cmd);

// State to publish; returns the message length (0 if out is too small)
size_t mqtt_build_state(const LightState* state, char* out, size_t outSize);

#endif
