#ifndef SettingsLogic_h
#define SettingsLogic_h

// Settings validation logic, independent from Arduino (unit tested on PC)

#include <stddef.h>

#define MODE_SWITCH               "Switch"
#define MODE_LEDSTRIP             "LedStrip"

enum DeviceModes { SWITCH_MODE, LEDSTRIP_MODE };

enum SettingAction {
  SETTING_KEEP,                   // Empty value: the saved value is kept
  SETTING_SET,                    // out contains the new value
  SETTING_CLEAR                   // "-" received: the saved value is deleted (only if allowed)
};

// "0".."255" only (value saved in the EEPROM state partition)
bool settings_is_byte_number(const char* val);

bool settings_is_known_mode(const char* mode);
// Empty or unknown mode (e.g. EEPROM written by an old firmware) -> Switch
DeviceModes settings_parse_mode(const char* mode);
const char* settings_mode_name(DeviceModes mode);
// GPIO14 changes function -> restart. Empty (old firmware) == Switch
bool settings_mode_change_requires_restart(const char* oldMode, const char* newMode);

// Value received from the web page: trimmed and truncated to maxLen
SettingAction settings_sanitize(const char* in, bool allowClear, size_t maxLen, char* out, size_t outSize);

#endif
