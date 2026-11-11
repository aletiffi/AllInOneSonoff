#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <SettingsLogic.h>
#include <MqttLogic.h>
#include "WebUI.h"
#include "Config.h"
#include "Settings.h"
#include "Device.h"
#include "LedStrip.h"
#include "Network.h"
#include "Ota.h"
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
  StaticJsonDocument <512> doc;
  String msg_out;

  doc["mode"] = DeviceModeName();
  doc["hostname"] = Hostname.Val;
  doc["version"] = FW_VERSION;
  doc["state"] = IsOn() ? ON_PAYLOAD : OFF_PAYLOAD;
  doc["ota_url"] = OtaUrl.Val;
  doc["ota_status"] = Ota_Status();
  doc["ota_signed"] = Ota_IsSigningEnabled();
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
        char val[MAX_LENGTH_URL + 1];
        // "-" deletes the update url, empty values keep the saved ones
        switch (settings_sanitize(kv.value().as<const char*>(), isUrl, isUrl ? MAX_LENGTH_URL : MAX_LENGTH_SETTING, val, sizeof(val))) {
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

  SaveSettingsInEeprom();
  server.send(200, "text/plain", "Settings saved");

  if (settings_mode_change_requires_restart(oldMode.c_str(), DeviceMode.Val.c_str())) {
    // GPIO14 changes function: a restart is required
    Serial.println("Device mode changed to " + DeviceMode.Val);
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
