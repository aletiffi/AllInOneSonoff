#include "Device.h"
#include "Config.h"
#include "Settings.h"
#include "LedStrip.h"
#include "Mqtt.h"

DigiOut Led(SONOFF_LED, OFF_LED);
DigiOut Rele(SONOFF_RELAY, OFF_RELAY);

void Device_Begin() {
  Rele.Begin();
  Led.Begin();
  Led.Blink(T_100MS * 5, 1, T_100MS * 5);
}

bool IsOn() {
  return Rele.State() != OFF_RELAY;
}

void Switch_On() {
  if (deviceMode == LEDSTRIP_MODE) {
    if (!IsOn()) {
      Rele.On();
      SwitchState.Val = 1;
      // Power supply delay
      delay(T_1S);
    }
    // Also publishes the state
    LedStrip_Show();
  } else {
    Rele.On();
    SwitchState.Val = 1;
    PublishState();
  }
  SaveStateInEeprom();
}

void Switch_Off() {
  if (deviceMode == LEDSTRIP_MODE) {
    LedStrip_Off();
  }
  Rele.Off();
  SwitchState.Val = 0;
  PublishState();
  SaveStateInEeprom();
}

void Switch_Toggle() {
  if (IsOn()) {
    Switch_Off();
  } else {
    Switch_On();
  }
}

void Restart() {
  Serial.println("Restart");
  delay(T_250MS);
  Led.Blink(T_200MS, 5, T_100MS);
  ESP.restart();
}

void CleanEEPROM() {
  Led.Blink(T_200MS, 3, T_200MS);
  mem.clear();
  mem.print_all();
  Led.Blink(T_200MS, 3, T_200MS);
  Restart();
}
