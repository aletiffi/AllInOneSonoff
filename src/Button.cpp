#include <Arduino.h>
#include <ReadInput.h>
#include "Button.h"
#include "Config.h"
#include "Settings.h"
#include "Device.h"
#include "Network.h"
#include "Ota.h"

static ReadInput Button(SONOFF_BUTTON);
static bool pushButton              = false;
static bool pushButtonPre           = false;
static int pushButtonCount          = 0;
static unsigned long pushButtonTime = 0;

static void PushButtonFunction(int func) {
  Serial.print("Function: " + String(func) + " -> ");
  switch (func) {
    case 1:
      Serial.println("Toggle");
      delay(T_250MS);
      Switch_Toggle();
      break;
    case 2:
      Serial.println("Connection check");
      delay(T_250MS);
      Connection_Manager();
      break;
    case 3:
      Serial.println("Check FW update");
      Ota_Check();
      break;
    case 4:
      Serial.println("Show ip address");
      ShowIpAddr();
      break;
    case 5:
      Serial.println("Wifi signal power");
      GetWifiPower(Ssid.Val);
      break;
    case 6:
      Serial.println("Load settings");
      LoadSettingsFromEeprom();
      break;
    case 7:
      Serial.println("Save settings");
      SaveSettingsInEeprom();
      break;
    case 8:
      Serial.println("EEPROM clean");
      CleanEEPROM();
      break;
    case 9:
      Serial.println("Read all EEPROM");
      mem.print_all();
      break;
    case 10:
      Serial.println("Sonoff restart");
      Restart();
      break;
    default:
      Serial.println("No function!");
      Led.Blink(T_200MS, 5, T_100MS);
      break;
  }
}

void Button_Begin() {
  pushButton = !Button.State();
  pushButtonPre = pushButton;
}

void Button_Loop() {
  pushButton = !Button.State();

  if (pushButton && !pushButtonPre) {
    Led.Off();
    delay(T_200MS);
    pushButtonTime = millis();
  }
  if (pushButtonPre && !pushButton) {
    PushButtonFunction(pushButtonCount);
    pushButtonCount = 0;
    if (deviceConnected) {
      Led.On();
    }
  }
  pushButtonPre = pushButton;

  if (pushButton) {
    if (millis() - pushButtonTime > (T_1S - T_100MS)) {
      pushButtonTime = millis();
      Led.Blink(T_200MS, 1, T_100MS);
      pushButtonCount ++;
    }
  }
}
