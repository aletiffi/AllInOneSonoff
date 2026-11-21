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

// --- Access point password ---
// WPA2: at least 8 characters; at most maxLen (setting size); printable ASCII only
#define AP_PASSWORD_MIN_LENGTH    8
bool settings_is_valid_ap_password(const char* pwd, size_t maxLen);
// Saved in the EEPROM with a marker byte: the settings are written one after the other and the bytes
// after the last one are not cleared, so a firmware that adds this setting could read the leftovers
// of older (longer) settings as a password and lock the access point. Leftovers are always printable
// text: without the marker (or not valid) the password is empty -> open access point.
#define AP_PASSWORD_MARKER        '\x02'
// "" -> "" (no marker); "secret12" -> "\x02secret12". False if out is too small
bool settings_encode_ap_password(const char* pwd, char* out, size_t outSize);
// Stored value -> password ("" if not set or not valid). False if out is too small
bool settings_decode_ap_password(const char* stored, size_t maxLen, char* out, size_t outSize);

#endif
