#ifndef Settings_h
#define Settings_h

#include <Arduino.h>
#include <StoreStrings.h>
#include <SettingsLogic.h>

#define NUM_WIFI_SETTINGS         10
#define NUM_STATE_SETTINGS        5

struct WifiSetup {
  String Name;
  String Val;
};

struct Setting {
  String Name;
  byte Val;
};

// Settings saved in the first EEPROM partition (order = EEPROM layout)
extern WifiSetup Hostname;
extern WifiSetup Ssid;
extern WifiSetup Password;
extern WifiSetup MqttSubTopic;
extern WifiSetup MqttPubTopic;
extern WifiSetup MqttServer;
extern WifiSetup MqttUser;
extern WifiSetup MqttPassword;
extern WifiSetup OtaUrl;
extern WifiSetup DeviceMode;
extern WifiSetup* WifiSettings[NUM_WIFI_SETTINGS];

// Last state saved in the second EEPROM partition
extern Setting SwitchState;
extern Setting Red;
extern Setting Green;
extern Setting Blue;
extern Setting Brightness;
extern Setting* StateSettings[NUM_STATE_SETTINGS];

extern StoreStrings mem;
extern DeviceModes deviceMode;
extern bool wifiConfigured;
extern bool mqttConfigured;

void Settings_Begin();
void SaveSettingsInEeprom();
void LoadSettingsFromEeprom();
void SaveStateInEeprom();
void LoadStateFromEeprom();
const char* DeviceModeName();

#endif
