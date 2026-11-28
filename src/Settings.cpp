#include "Settings.h"
#include "Config.h"

StoreStrings mem(SETTIGNS_EEPROM_SIZE, STATE_EEPROM_SIZE);

DeviceModes deviceMode            = SWITCH_MODE;
SonoffModels sonoffModel          = SONOFF_R1;
bool wifiConfigured               = false;
bool mqttConfigured               = false;

WifiSetup Hostname                = {"Hostname", ""};
WifiSetup Ssid                    = {"Ssid", ""};
WifiSetup Password                = {"Password", ""};
WifiSetup MqttSubTopic            = {"MQTT_Sub", ""};
WifiSetup MqttPubTopic            = {"MQTT_Pub", ""};
WifiSetup MqttServer              = {"MQTT_Server", ""};
WifiSetup MqttUser                = {"MQTT_User", ""};
WifiSetup MqttPassword            = {"MQTT_Password", ""};
// Same position of the old OTA Drive product key
WifiSetup OtaUrl                  = {"OTA_URL", ""};
// Appended last so that the EEPROM layout of previous firmwares stays valid
WifiSetup DeviceMode              = {"Device_Mode", ""};
// Appended last; in RAM the plain password, in the EEPROM with a marker (see SettingsLogic.h)
WifiSetup ApPassword              = {"AP_Password", ""};
// Appended last: with the EEPROM of an old firmware it reads leftovers, only "R2" is accepted
WifiSetup SonoffModel             = {"Sonoff_Model", ""};

WifiSetup* WifiSettings[NUM_WIFI_SETTINGS] = {
  &Hostname,
  &Ssid,
  &Password,
  &MqttSubTopic,
  &MqttPubTopic,
  &MqttServer,
  &MqttUser,
  &MqttPassword,
  &OtaUrl,
  &DeviceMode,
  &ApPassword,
  &SonoffModel};

Setting SwitchState               = {"SwitchState", 0};
Setting Red                       = {"Red", MAX_RGB_VAL};
Setting Green                     = {"Green", MAX_RGB_VAL};
Setting Blue                      = {"Blue", MAX_RGB_VAL};
Setting Brightness                = {"Brightness", MAX_RGB_VAL / 3};

Setting* StateSettings[NUM_STATE_SETTINGS] = {
  &SwitchState,
  &Red,
  &Green,
  &Blue,
  &Brightness};

void Settings_Begin() {
  if (!mem.isReady()) {
    mem.clear();
  }
}

const char* DeviceModeName() {
  return settings_mode_name(deviceMode);
}

const char* SonoffModelName() {
  return settings_model_name(sonoffModel);
}

void SaveSettingsInEeprom() {
  mem.resetWriteCounter();
  for (byte i = 0; i < NUM_WIFI_SETTINGS; i++) {
    if (WifiSettings[i] == &ApPassword) {
      char stored[MAX_LENGTH_SETTING + 2];
      settings_encode_ap_password(ApPassword.Val.c_str(), stored, sizeof(stored));
      mem.write(mem.getLastWrittenByte(), String(stored));
    } else {
      mem.write(mem.getLastWrittenByte(), WifiSettings[i]->Val);
    }
  }
}

void Settings_UpdateFlags() {
  // Only the server is required: topics have defaults, user and password are optional
  mqttConfigured = MqttServer.Val.length() > 0;
  wifiConfigured = Ssid.Val.length() > 0 && Password.Val.length() > 0;
}

void LoadSettingsFromEeprom() {
  Serial.println("Loading settings from EEPROM... ");
  mem.resetReadCounter();
  for (byte i = 0; i < NUM_WIFI_SETTINGS; i++) {
    WifiSettings[i]->Val = mem.read(mem.getLastReadedByte());
    if (WifiSettings[i] == &SonoffModel && !settings_is_known_model(SonoffModel.Val.c_str())) {
      // Never saved (EEPROM of an old firmware or cleared): R1
      SonoffModel.Val = "";
    }
    if (WifiSettings[i] == &ApPassword) {
      char pwd[MAX_LENGTH_SETTING + 1];
      settings_decode_ap_password(ApPassword.Val.c_str(), MAX_LENGTH_SETTING, pwd, sizeof(pwd));
      ApPassword.Val = pwd;
    }
    // Passwords are not printed
    bool secret = (WifiSettings[i] == &Password || WifiSettings[i] == &MqttPassword || WifiSettings[i] == &ApPassword) &&
                  WifiSettings[i]->Val.length() > 0;
    Serial.println(WifiSettings[i]->Name + ": " + (secret ? "****" : WifiSettings[i]->Val));
  }

  Settings_UpdateFlags();

  // Empty or unknown mode (e.g. EEPROM written by an old firmware) -> Switch
  sonoffModel = settings_parse_model(SonoffModel.Val.c_str());
  // LedStrip saved on an R2 -> Switch
  deviceMode = settings_effective_mode(DeviceMode.Val.c_str(), SonoffModel.Val.c_str());
}

void SaveStateInEeprom() {
  mem.resetWriteCounter2();
  for (byte i = 0; i < NUM_STATE_SETTINGS; i++) {
    mem.write_pt2(mem.getLastWrittenByte2(), String(StateSettings[i]->Val));
  }
}

void LoadStateFromEeprom() {
  Serial.println("Loading last state from EEPROM");
  mem.resetReadCounter2();
  for (byte i = 0; i < NUM_STATE_SETTINGS; i++) {
    String val = mem.read_pt2(mem.getLastReadedByte2());
    // Values never saved (or not valid) keep their default
    if (settings_is_byte_number(val.c_str())) {
      StateSettings[i]->Val = val.toInt();
    }
    Serial.print(StateSettings[i]->Name + ": " + StateSettings[i]->Val + " ");
  }
  Serial.println("");
}
