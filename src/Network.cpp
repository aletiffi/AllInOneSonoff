#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "Network.h"
#include "Config.h"
#include "Settings.h"
#include "Device.h"
#include "Mqtt.h"
#include "WebUI.h"

bool deviceConnected                 = false;

static MDNSResponder mdns;
static unsigned long lastTimeCheckConn = 0;

void Connection_Manager() {
  Serial.println("Connection process started");
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  delay(T_200MS);
  LoadSettingsFromEeprom();

  WiFi.mode(WIFI_AP_STA);

  if (wifiConfigured) {
    if (GetWifiPower(Ssid.Val)) {
      WiFi.begin(Ssid.Val, Password.Val);
      Serial.println("Connecting to: " + String(Ssid.Val));
      byte numCehck = 0;
      while ((WiFi.status() != WL_CONNECTED)) {
        if (numCehck >= 150) {
          break;
        }
        numCehck++;
        delay(T_200MS);
      }
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected!");
    Serial.println("IP: " + WiFi.localIP().toString() + " Mac: " + String(WiFi.macAddress()));
    Mqtt_Setup();

    // Successful connection
    Led.On();
    deviceConnected = true;

  } else {
    Serial.println("Not connected!");
    // Connection failed
    Led.Blink(T_200MS, 10, T_100MS);
    // Keeps the led off
    Led.Off();
    deviceConnected = false;
  }

  if (Hostname.Val == "" || Hostname.Val.startsWith(" ") || Hostname.Val.length() >= MAX_LENGTH_SETTING) {
    Hostname.Val = DEFAULT_AP_NAME;
  }
  Serial.println("AP Name: " + Hostname.Val);

  WiFi.softAP(Hostname.Val.c_str());
  mdns.begin(Hostname.Val, WiFi.localIP());
  mdns.addService("http", "tcp", 80);
  WebUI_Begin();

  // Reset last connection time
  lastTimeCheckConn = millis();
}

void Network_Loop() {
  mdns.update();

  // Connection check
  if (wifiConfigured && millis() - lastTimeCheckConn > T_5MIN) {
    Serial.print("Connection check... ");
    Led.Off();
    delay(T_200MS);
    lastTimeCheckConn = millis();

    if (WiFi.status() != WL_CONNECTED) {
      Connection_Manager();
    } else {
      Serial.println("OK");
      PublishState();
      // Turns on the LED for network connection confirmation
      Led.On();
    }
  }
}

void ShowIpAddr() {
  if (deviceConnected) {
    IPAddress ip = WiFi.localIP();
    Serial.println(ip.toString());
    // Number of flashes equal to the last IP number
    Led.Blink(T_200MS, ip[3], T_200MS);
  } else {
    Serial.println("Not connected!");
    // Connection failed
    Led.Blink(T_200MS, 10, T_100MS);
  }
}

bool GetWifiPower(const String& netName) {
  int netsNumber = WiFi.scanNetworks();
  int minRawLevel = -75;
  int maxRawLevel = -45;
  int minLevel = 0;
  int maxLevel = 5;
  bool result = false;
  const char* levels[] = {" Unreacheable", " Very Low", " Low", " Good", " High", " Very High!"};

  for (int i = 0; i < netsNumber; ++i) {
    if (WiFi.SSID(i) == netName) {
      result = true;
      int rawPower = WiFi.RSSI(i);
      Serial.print(WiFi.SSID(i) + " (" + String(rawPower) + ")");
      if (minRawLevel <= rawPower && rawPower <= maxRawLevel) {
        int power = map(rawPower, minRawLevel, maxRawLevel, minLevel, maxLevel);
        Serial.println(levels[power]);
        Led.Blink(T_200MS, power, T_200MS);
      } else {
        Serial.println(" Signal level out of bounds");
      }
    }
  }
  return result;
}
