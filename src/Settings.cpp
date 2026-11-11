#include "Settings.h"
#include "Config.h"

StoreStrings mem(SETTIGNS_EEPROM_SIZE, STATE_EEPROM_SIZE);

DeviceModes deviceMode            = SWITCH_MODE;
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
  &DeviceMode};

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

static bool IsByteNumber(const String& val) {
  if (val.length() == 0 || val.length() > 3) {
    return false;
  }
  for (unsigned int i = 0; i < val.length(); i++) {
    if (!isDigit(val.charAt(i))) {
      return false;
    }
  }
  return val.toInt() <= 255;
}

void Settings_Begin() {
  if (!mem.isReady()) {
    mem.clear();
  }
}

const char* DeviceModeName() {
  return deviceMode == LEDSTRIP_MODE ? MODE_LEDSTRIP : MODE_SWITCH;
}

void SaveSettingsInEeprom() {
  mem.resetWriteCounter();
  for (byte i = 0; i < NUM_WIFI_SETTINGS; i++) {
    mem.write(mem.getLastWrittenByte(), WifiSettings[i]->Val);
  }
}

void LoadSettingsFromEeprom() {
  Serial.println("Loading settings from EEPROM... ");
  mem.resetReadCounter();
  for (byte i = 0; i < NUM_WIFI_SETTINGS; i++) {
    WifiSettings[i]->Val = mem.read(mem.getLastReadedByte());
    // Passwords are not printed
    bool secret = (WifiSettings[i] == &Password || WifiSettings[i] == &MqttPassword) && WifiSettings[i]->Val.length() > 0;
    Serial.println(WifiSettings[i]->Name + ": " + (secret ? "****" : WifiSettings[i]->Val));
  }

  mqttConfigured = MqttSubTopic.Val.length() > 0 && MqttPubTopic.Val.length() > 0 && MqttServer.Val.length() > 0 &&
                   MqttUser.Val.length() > 0 && MqttPassword.Val.length() > 0;
  wifiConfigured = Ssid.Val.length() > 0 && Password.Val.length() > 0;

  // Empty or unknown mode (e.g. EEPROM written by an old firmware) -> Switch
  deviceMode = DeviceMode.Val == MODE_LEDSTRIP ? LEDSTRIP_MODE : SWITCH_MODE;
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
    if (IsByteNumber(val)) {
      StateSettings[i]->Val = val.toInt();
    }
    Serial.print(StateSettings[i]->Name + ": " + StateSettings[i]->Val + " ");
  }
  Serial.println("");
}
