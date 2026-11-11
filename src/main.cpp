#include <Arduino.h>
#include "Config.h"
#include "Settings.h"
#include "Device.h"
#include "Button.h"
#include "WallSwitch.h"
#include "LedStrip.h"
#include "Network.h"
#include "Mqtt.h"
#include "WebUI.h"
#include "Ota.h"

void setup() {
  Serial.begin(115200);
  delay(T_250MS);

  Settings_Begin();
  Device_Begin();

  Serial.println("\n----------------------");
  Serial.println("Application ver. " FW_VERSION " running!");

  // The device mode is needed before configuring GPIO14
  LoadSettingsFromEeprom();
  Serial.println("Device mode: " + String(DeviceModeName()));

  if (deviceMode == LEDSTRIP_MODE) {
    LedStrip_Begin();
  } else {
    WallSwitch_Begin();
  }
  Button_Begin();

  LoadStateFromEeprom();
  if (SwitchState.Val == 1) {
    Rele.On();
    if (deviceMode == LEDSTRIP_MODE) {
      LedStrip_Show();
    }
  } else {
    Rele.Off();
  }
  Connection_Manager();
}

void loop() {
  Mqtt_Loop();
  WebUI_Loop();
  Network_Loop();
  Ota_Loop();
  Button_Loop();

  if (deviceMode == SWITCH_MODE) {
    WallSwitch_Loop();
  } else {
    LedStrip_Loop();
  }
}
