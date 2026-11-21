#include "SettingsLogic.h"
#include <string.h>
#include <stdlib.h>

bool settings_is_byte_number(const char* val) {
  size_t len = val ? strlen(val) : 0;
  if (len == 0 || len > 3) {
    return false;
  }
  for (size_t i = 0; i < len; i++) {
    if (val[i] < '0' || val[i] > '9') {
      return false;
    }
  }
  return atoi(val) <= 255;
}

bool settings_is_known_mode(const char* mode) {
  return mode && (strcmp(mode, MODE_SWITCH) == 0 || strcmp(mode, MODE_LEDSTRIP) == 0);
}

DeviceModes settings_parse_mode(const char* mode) {
  return (mode && strcmp(mode, MODE_LEDSTRIP) == 0) ? LEDSTRIP_MODE : SWITCH_MODE;
}

const char* settings_mode_name(DeviceModes mode) {
  return mode == LEDSTRIP_MODE ? MODE_LEDSTRIP : MODE_SWITCH;
}

bool settings_mode_change_requires_restart(const char* oldMode, const char* newMode) {
  return settings_parse_mode(oldMode) != settings_parse_mode(newMode);
}

static bool IsSpace(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

SettingAction settings_sanitize(const char* in, bool allowClear, size_t maxLen, char* out, size_t outSize) {
  if (!in || !out || outSize == 0) {
    return SETTING_KEEP;
  }
  while (IsSpace(*in)) {
    in++;
  }
  size_t len = strlen(in);
  while (len > 0 && IsSpace(in[len - 1])) {
    len--;
  }
  if (len == 0) {
    return SETTING_KEEP;
  }
  if (allowClear && len == 1 && in[0] == '-') {
    out[0] = '\0';
    return SETTING_CLEAR;
  }
  if (len > maxLen) {
    len = maxLen;
  }
  if (len > outSize - 1) {
    len = outSize - 1;
  }
  memcpy(out, in, len);
  out[len] = '\0';
  return SETTING_SET;
}

bool settings_is_valid_ap_password(const char* pwd, size_t maxLen) {
  if (!pwd) {
    return false;
  }
  size_t len = strlen(pwd);
  if (len < AP_PASSWORD_MIN_LENGTH || len > maxLen) {
    return false;
  }
  for (size_t i = 0; i < len; i++) {
    if (pwd[i] < 0x20 || pwd[i] > 0x7e) {
      return false;
    }
  }
  return true;
}

bool settings_encode_ap_password(const char* pwd, char* out, size_t outSize) {
  if (!out || outSize == 0) {
    return false;
  }
  size_t len = pwd ? strlen(pwd) : 0;
  if (len == 0) {
    out[0] = '\0';
    return true;
  }
  if (len + 2 > outSize) {
    return false;
  }
  out[0] = AP_PASSWORD_MARKER;
  memcpy(out + 1, pwd, len + 1);
  return true;
}

bool settings_decode_ap_password(const char* stored, size_t maxLen, char* out, size_t outSize) {
  if (!out || outSize == 0) {
    return false;
  }
  out[0] = '\0';
  if (!stored || stored[0] != AP_PASSWORD_MARKER || !settings_is_valid_ap_password(stored + 1, maxLen)) {
    return true;
  }
  size_t len = strlen(stored + 1);
  if (len + 1 > outSize) {
    return false;
  }
  memcpy(out, stored + 1, len + 1);
  return true;
}
