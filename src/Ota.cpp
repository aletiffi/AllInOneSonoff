#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>
#include <memory>
#include "Ota.h"
#include "Config.h"
#include "Settings.h"
#include "Device.h"
#include "Network.h"
#include "Mqtt.h"

static bool otaRequested           = false;
static bool firstCheckDone         = false;
static unsigned long lastOtaCheck  = 0;
static String otaStatus            = "Never checked";

static bool IsValidUrl(const String& url) {
  return url.startsWith("http://") || url.startsWith("https://");
}

static std::unique_ptr<WiFiClient> MakeClient(const String& url) {
  if (url.startsWith("https://")) {
    std::unique_ptr<BearSSL::WiFiClientSecure> secure(new BearSSL::WiFiClientSecure());
    // The server certificate is not verified: the device has no root CAs nor a reliable clock
    secure->setInsecure();
    return std::unique_ptr<WiFiClient>(secure.release());
  }
  return std::unique_ptr<WiFiClient>(new WiFiClient());
}

static void SetStatus(const String& status) {
  otaStatus = status;
  Serial.println("OTA: " + status);
}

// Google Drive share link -> direct download link
//   https://drive.google.com/file/d/<ID>/view?usp=sharing
//   https://drive.google.com/open?id=<ID>
String Ota_NormalizeUrl(const String& url) {
  String id = "";
  const char* fileTag = "drive.google.com/file/d/";
  const char* openTag = "drive.google.com/open?id=";
  int p = url.indexOf(fileTag);

  if (p >= 0) {
    int start = p + strlen(fileTag);
    int end = url.indexOf('/', start);
    if (end < 0) {
      end = url.indexOf('?', start);
    }
    id = url.substring(start, end < 0 ? url.length() : end);
  } else if ((p = url.indexOf(openTag)) >= 0) {
    int start = p + strlen(openTag);
    int end = url.indexOf('&', start);
    id = url.substring(start, end < 0 ? url.length() : end);
  }

  if (id.length() > 0) {
    return "https://drive.google.com/uc?export=download&confirm=t&id=" + id;
  }
  return url;
}

// <0 if a < b, 0 if equal, >0 if a > b ("45.10.0.0" > "45.9.0.0")
int Ota_CompareVersions(const String& a, const String& b) {
  int ia = 0, ib = 0;
  while (ia < (int)a.length() || ib < (int)b.length()) {
    long na = 0, nb = 0;
    while (ia < (int)a.length() && a.charAt(ia) != '.') {
      na = na * 10 + (a.charAt(ia++) - '0');
    }
    while (ib < (int)b.length() && b.charAt(ib) != '.') {
      nb = nb * 10 + (b.charAt(ib++) - '0');
    }
    if (na != nb) {
      return na < nb ? -1 : 1;
    }
    ia++;
    ib++;
  }
  return 0;
}

static bool DownloadManifest(const String& url, String& version, String& binUrl) {
  std::unique_ptr<WiFiClient> client = MakeClient(url);
  HTTPClient http;
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setTimeout(T_10S);

  if (!http.begin(*client, url)) {
    SetStatus("Invalid manifest url");
    return false;
  }
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    SetStatus("Manifest download error: " + (code < 0 ? http.errorToString(code) : "HTTP " + String(code)));
    http.end();
    return false;
  }
  String body = http.getString();
  http.end();

  StaticJsonDocument <512> doc;
  if (deserializeJson(doc, body)) {
    SetStatus("Manifest is not a valid json");
    return false;
  }
  version = doc["version"].as<String>();
  binUrl = Ota_NormalizeUrl(doc["url"].as<String>());
  if (version.length() == 0 || version == "null" || !IsValidUrl(binUrl)) {
    SetStatus("Manifest without version or url");
    return false;
  }
  return true;
}

void Ota_Check() {
  otaRequested = false;
  firstCheckDone = true;
  lastOtaCheck = millis();

  if (!IsValidUrl(OtaUrl.Val)) {
    SetStatus("Update url not set");
    return;
  }
  if (!deviceConnected) {
    SetStatus("Device not connected");
    return;
  }

  String manifestUrl = Ota_NormalizeUrl(OtaUrl.Val);
  String version, binUrl;
  Serial.println("OTA: manifest " + manifestUrl + " (free heap " + String(ESP.getFreeHeap()) + ")");
  if (!DownloadManifest(manifestUrl, version, binUrl)) {
    return;
  }

  if (Ota_CompareVersions(version, FW_VERSION) <= 0) {
    SetStatus("Firmware up to date (" FW_VERSION ", available " + version + ")");
    return;
  }

  SetStatus("Updating " FW_VERSION " -> " + version);
  // Frees memory for the TLS connection
  Mqtt_Disconnect();
  Led.Off();

  std::unique_ptr<WiFiClient> client = MakeClient(binUrl);
  ESPhttpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  ESPhttpUpdate.setLedPin(SONOFF_LED, LOW);
  ESPhttpUpdate.rebootOnUpdate(true);
  t_httpUpdate_return ret = ESPhttpUpdate.update(*client, binUrl, FW_VERSION);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      SetStatus("Update failed (" + String(ESPhttpUpdate.getLastError()) + "): " + ESPhttpUpdate.getLastErrorString());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      SetStatus("No new update available");
      break;
    case HTTP_UPDATE_OK:
      // Normally not reached: the device restarts
      SetStatus("Update OK");
      break;
  }
  if (deviceConnected) {
    Led.On();
  }
}

void Ota_Request() {
  otaRequested = true;
  otaStatus = "Checking...";
}

String Ota_Status() {
  return otaStatus;
}

void Ota_Loop() {
  if (!deviceConnected) {
    return;
  }
  // On request, at the first connection and then every hour
  if (otaRequested || (!firstCheckDone && IsValidUrl(OtaUrl.Val)) || millis() - lastOtaCheck > T_1H) {
    Ota_Check();
  }
}
