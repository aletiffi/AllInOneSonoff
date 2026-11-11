#include "MqttLogic.h"
#include <string.h>
#include <strings.h>
#include <ArduinoJson.h>

static uint8_t ClampByte(long val) {
  return val < 0 ? 0 : (val > 255 ? 255 : (uint8_t)val);
}

bool mqtt_parse_command(const char* payload, size_t length, bool ledstripMode, LightCommand* cmd) {
  memset(cmd, 0, sizeof(LightCommand));
  if (payload && (length == 2 || length == 3 || length == 6)) {
    if (length == 2 && strncasecmp(payload, "ON", 2) == 0) {
      cmd->hasState = cmd->on = true;
      return true;
    }
    if (length == 3 && strncasecmp(payload, "OFF", 3) == 0) {
      cmd->hasState = true;
      return true;
    }
    if (length == 6 && strncasecmp(payload, "TOGGLE", 6) == 0) {
      cmd->toggle = true;
      return true;
    }
  }
  StaticJsonDocument<512> doc;
  if (!payload || deserializeJson(doc, payload, length) || !doc.is<JsonObject>()) {
    return false;
  }

  const char* state = doc["state"] | "";
  if (strcmp(state, ON_PAYLOAD) == 0 || strcmp(state, OFF_PAYLOAD) == 0) {
    cmd->hasState = true;
    cmd->on = strcmp(state, ON_PAYLOAD) == 0;
  }
  if (!ledstripMode) {
    return true;
  }

  if (doc["brightness"].is<long>()) {
    cmd->hasBrightness = true;
    cmd->brightness = ClampByte(doc["brightness"].as<long>());
  }
  JsonObject color = doc["color"];
  if (!color.isNull() && color["r"].is<long>() && color["g"].is<long>() && color["b"].is<long>()) {
    cmd->hasColor = true;
    cmd->color.r = ClampByte(color["r"].as<long>());
    cmd->color.g = ClampByte(color["g"].as<long>());
    cmd->color.b = ClampByte(color["b"].as<long>());
  }
  int effect = effect_from_name(doc["effect"] | "");
  if (effect >= 0) {
    cmd->hasEffect = true;
    cmd->effect = effect;
  }
  return true;
}

size_t mqtt_build_state(const LightState* state, char* out, size_t outSize) {
  StaticJsonDocument<512> doc;
  doc["state"] = state->on ? ON_PAYLOAD : OFF_PAYLOAD;
  if (state->on && state->ledstrip) {
    doc["color_mode"] = "rgb";
    doc["brightness"] = state->brightness;
    doc["effect"] = effect_name(state->effect);
    JsonObject color = doc.createNestedObject("color");
    color["r"] = state->color.r;
    color["g"] = state->color.g;
    color["b"] = state->color.b;
  }
  if (measureJson(doc) + 1 > outSize) {
    return 0;
  }
  return serializeJson(doc, out, outSize);
}
