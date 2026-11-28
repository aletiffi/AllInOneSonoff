#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <SettingsLogic.h>
#include <MqttLogic.h>
#include <OtaLogic.h>
#include "WebUI.h"
#include "Config.h"
#include "Settings.h"
#include "Device.h"
#include "LedStrip.h"
#include "Network.h"
#include "Ota.h"
#include "Mqtt.h"
#include "HomePage.h"

static ESP8266WebServer server(80);
static bool webServerStarted = false;

static void RedirectHome() {
  server.sendHeader("Location", "/");
  server.send(303);
}

static void HandleNotFound() {
  String message = "Err 404\nPage Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

static void SendInfo() {
  // Used by the home page to show the controls of the current mode
  // On the heap: the ESP8266 stack is only 4 KB
  DynamicJsonDocument doc(1536);
  String msg_out;

  doc["mode"] = DeviceModeName();
  doc["hostname"] = Hostname.Val;
  doc["version"] = FW_VERSION;
  doc["model"] = SonoffModelName();
  doc["state"] = IsOn() ? ON_PAYLOAD : OFF_PAYLOAD;
  doc["ota_url"] = Ota_ManifestUrl();
  doc["ota_url_default"] = Ota_ManifestUrl() == OTA_DEFAULT_URL;
  doc["ota_status"] = Ota_Status();
  doc["ota_signed"] = Ota_IsSigningEnabled();
  doc["unique_id"] = Mqtt_UniqueId();
  doc["mqtt_connected"] = Mqtt_IsConnected();
  doc["mqtt_command"] = Mqtt_CommandTopic();
  doc["mqtt_state"] = Mqtt_StateTopic();
  doc["mqtt_base"] = Mqtt_BaseTopic();
  doc["ap_protected"] = ApPassword.Val.length() > 0;
  // Saved settings for the fields of the page; passwords never sent, only whether they are set
  JsonObject s = doc.createNestedObject("settings");
  s["Hostname"] = Hostname.Val;
  s["Sonoff_Model"] = SonoffModelName();
  s["Ssid"] = Ssid.Val;
  s["MQTT_Sub"] = MqttSubTopic.Val;
  s["MQTT_Pub"] = MqttPubTopic.Val;
  s["MQTT_Server"] = MqttServer.Val;
  s["MQTT_User"] = MqttUser.Val;
  // Only a custom url or "off" (not e.g. an old OTA Drive key, which means default)
  bool customOta = ota_is_valid_url(OtaUrl.Val.c_str()) || ota_is_disabled(OtaUrl.Val.c_str());
  s["OTA_URL"] = customOta ? OtaUrl.Val : String("");
  s["Password_set"] = Password.Val.length() > 0;
  s["MQTT_Password_set"] = MqttPassword.Val.length() > 0;
  s["AP_Password_set"] = ApPassword.Val.length() > 0;
  if (deviceMode == LEDSTRIP_MODE) {
    doc["color"] = LedStrip_ColorHex();
    doc["brightness"] = Brightness.Val;
    doc["effect"] = LedStrip_EffectName();
  }
  serializeJson(doc, msg_out);
  server.send(200, "application/json", msg_out);
}

static void ChangeBrightness() {
  StaticJsonDocument <JSON_MSG_LENGTH> doc;
  deserializeJson(doc, server.arg("plain"));
  if (deviceMode == LEDSTRIP_MODE && doc.containsKey("Brightness")) {
    Brightness.Val = constrain(doc["Brightness"].as<String>().toInt(), 0, MAX_RGB_VAL);
    Serial.println("Brightness recived: " + String(Brightness.Val));
    LedStrip_Show();
    SaveStateInEeprom();
  }
  server.send(200, "text/plain", "OK");
}

static void ChangeEffect() {
  StaticJsonDocument <JSON_MSG_LENGTH> doc;
  deserializeJson(doc, server.arg("plain"));
  if (deviceMode == LEDSTRIP_MODE && doc.containsKey("effect")) {
    String effect = doc["effect"].as<String>();
    if (LedStrip_SelectEffect(effect)) {
      Serial.println("Effect recived: " + effect);
    }
  }
  server.send(200, "text/plain", "OK");
}

static void ChangeColor() {
  StaticJsonDocument <JSON_MSG_LENGTH> doc;
  deserializeJson(doc, server.arg("plain"));
  if (deviceMode == LEDSTRIP_MODE && doc.containsKey("color")) {
    String color = doc["color"].as<String>();
    if (LedStrip_SetColorHex(color)) {
      Serial.println("Color recived: " + color);
      LedStrip_Show();
      SaveStateInEeprom();
    } else {
      Serial.println("Color recived: " + color + " -> unable to decode");
    }
  }
  server.send(200, "text/plain", "OK");
}

static void ChangeSettings() {
  StaticJsonDocument <768> doc;
  String data = server.arg("plain");
  String oldMode = DeviceMode.Val;
  String oldModel = SonoffModel.Val;
  String oldValues[NUM_WIFI_SETTINGS];
  for (byte i = 0; i < NUM_WIFI_SETTINGS; i++) {
    oldValues[i] = WifiSettings[i]->Val;
  }
  Serial.println("Parameters recived");
  if (deserializeJson(doc, data)) {
    server.send(400, "text/plain", "Invalid json");
    return;
  }
  JsonObject root = doc.as<JsonObject>();
  for (JsonPair kv : root) {
    for (byte i = 0; i < NUM_WIFI_SETTINGS; i++) {
      if (strcmp(kv.key().c_str(), WifiSettings[i]->Name.c_str()) == 0 && kv.value().is<const char*>()) {
        bool isUrl = WifiSettings[i] == &OtaUrl;
        // Passwords are never shown by the page: empty = keep, "-" = delete (not the WiFi one)
        // Empty = keep the saved value (an old or partially loaded page can never delete settings),
        // "-" = delete: sent by the page for the fields emptied by the user. Not for the WiFi password
        // (needed to connect) nor for the mode (validated below)
        bool allowClear = WifiSettings[i] != &Password && WifiSettings[i] != &DeviceMode && WifiSettings[i] != &SonoffModel;
        char val[MAX_LENGTH_URL + 1];
        switch (settings_sanitize(kv.value().as<const char*>(), allowClear, isUrl ? MAX_LENGTH_URL : MAX_LENGTH_SETTING, val, sizeof(val))) {
          case SETTING_SET:
            WifiSettings[i]->Val = val;
            break;
          case SETTING_CLEAR:
            WifiSettings[i]->Val = "";
            break;
          case SETTING_KEEP:
            break;
        }
      }
    }
  }
  // Only known modes are accepted
  if (!settings_is_known_mode(DeviceMode.Val.c_str())) {
    DeviceMode.Val = oldMode;
  }
  if (!settings_is_known_model(SonoffModel.Val.c_str())) {
    SonoffModel.Val = oldModel;
  }
  // No LedStrip mode on the R2
  if (settings_effective_mode(DeviceMode.Val.c_str(), SonoffModel.Val.c_str()) != settings_parse_mode(DeviceMode.Val.c_str())) {
    DeviceMode.Val = MODE_SWITCH;
  }
  // WPA2 needs at least 8 characters: an invalid password is not saved (nothing else is saved either)
  if (ApPassword.Val.length() > 0 && !settings_is_valid_ap_password(ApPassword.Val.c_str(), MAX_LENGTH_SETTING)) {
    LoadSettingsFromEeprom();
    server.send(400, "text/plain", "Access point password: 8-" + String(MAX_LENGTH_SETTING) + " characters (letters, numbers, symbols)");
    return;
  }

  SaveSettingsInEeprom();
  Settings_UpdateFlags();
  server.send(200, "text/plain", "Settings saved");
  // Names of the changed settings only (never the values: passwords)
  String changed = "";
  for (byte i = 0; i < NUM_WIFI_SETTINGS; i++) {
    if (WifiSettings[i]->Val != oldValues[i]) {
      changed += (changed.length() > 0 ? ", " : "") + WifiSettings[i]->Name + (WifiSettings[i]->Val.length() == 0 ? " (deleted)" : "");
    }
  }
  Serial.println("Settings changed: " + (changed.length() > 0 ? changed : String("none")));

  bool modelChanged = settings_model_change_requires_restart(oldModel.c_str(), SonoffModel.Val.c_str());
  if (settings_mode_change_requires_restart(oldMode.c_str(), DeviceMode.Val.c_str()) || modelChanged) {
    // The free pin changes function or position: a restart is required
    Serial.println("Device mode " + DeviceMode.Val + ", model " + SonoffModel.Val + ": restarting");
    Switch_Off();
    Restart();
  }
  Connection_Manager();
}

void WebUI_Begin() {
  // Routes are registered only once, Connection_Manager can be called many times
  if (webServerStarted) {
    return;
  }
  server.on("/", []() {
    // Always the page of the running firmware (an old page in the browser cache sends old requests)
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.send_P(200, "text/html", webPage);
  });
  server.on("/info", HTTP_GET, SendInfo);
  server.on("/on", []() {
    RedirectHome();
    Switch_On();
  });
  server.on("/off", []() {
    RedirectHome();
    Switch_Off();
  });
  server.on("/restart", []() {
    RedirectHome();
    Restart();
  });
  server.on("/update", []() {
    // The check is done in the main loop, the page polls /info for the result
    Ota_Request();
    server.send(200, "text/plain", "Update check started");
  });
  server.on("/brigthness", HTTP_POST, ChangeBrightness);
  server.on("/effect", HTTP_POST, ChangeEffect);
  server.on("/color", HTTP_POST, ChangeColor);
  server.on("/settings", HTTP_POST, ChangeSettings);
  server.on("/clean", HTTP_GET, CleanEEPROM);
  server.onNotFound(HandleNotFound);
  server.begin();
  webServerStarted = true;
}

void WebUI_Loop() {
  server.handleClient();
}
