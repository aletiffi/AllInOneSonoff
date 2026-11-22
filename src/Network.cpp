#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "Network.h"
#include "Config.h"
#include "Settings.h"
#include "Device.h"
#include "Mqtt.h"
#include "WebUI.h"

bool deviceConnected                 = false;

// The ESP8266 has one radio: the access point works on the channel of the station.
// While the SDK searches the network it scans all the channels and the access point is unusable,
// so the automatic reconnection (a scan every second) is disabled: short attempts every
// T_WIFI_RETRY, postponed while a device is connected to the access point (at most T_WIFI_MAX_POSTPONE:
// a phone that keeps the access point saved can stay connected and must not block the reconnection).
#define T_WIFI_RETRY              30000
#define T_WIFI_MAX_POSTPONE       T_5MIN
#define T_WIFI_ATTEMPT            15000
#define WIFI_OUTPUT_POWER_DBM     17.0f

static MDNSResponder mdns;
static unsigned long lastTimeCheckConn = 0;
static unsigned long lastWifiRetry     = 0;
static unsigned long wifiAttemptStart  = 0;
static bool wifiAttemptInProgress      = false;
static unsigned long postponedSince    = 0;      // 0 = no attempt postponed

static bool apPaused = false;

static void StartAccessPoint() {
  // Access point on the channel of the home network: one radio, both must use the same channel
  // (with the default channel 1 a device joining the access point could move the radio and drop the station)
  int apChannel = WiFi.status() == WL_CONNECTED ? WiFi.channel() : 1;
  bool apProtected = ApPassword.Val.length() > 0;
  bool apStarted = WiFi.softAP(Hostname.Val.c_str(), apProtected ? ApPassword.Val.c_str() : nullptr, apChannel);
  if (!apStarted) {
    Serial.println("ERROR: access point not started");
  } else if (apProtected) {
    Serial.println("Access point " + WiFi.softAPSSID() + " protected (WPA2), channel " + String(WiFi.channel()) + ", ip " + WiFi.softAPIP().toString());
  } else {
    Serial.println("Access point " + WiFi.softAPSSID() + " open, channel " + String(WiFi.channel()) + ", ip " + WiFi.softAPIP().toString());
    Serial.println("WARNING: access point without password, anyone nearby can change the settings");
  }
  mdns.begin(Hostname.Val, WiFi.localIP());
  mdns.addService("http", "tcp", 80);
}

void Network_PauseAccessPoint() {
  if (apPaused || WiFi.getMode() != WIFI_AP_STA) {
    return;
  }
  // mDNS first: it must leave the multicast group before the interface is removed (crash, exception 0)
  mdns.close();
  WiFi.softAPdisconnect(true);
  apPaused = true;
  Serial.println("Access point off during the update (free heap " + String(ESP.getFreeHeap()) + ")");
}

void Network_ResumeAccessPoint() {
  if (!apPaused) {
    return;
  }
  apPaused = false;
  WiFi.mode(WIFI_AP_STA);
  StartAccessPoint();
}

void Connection_Manager() {
  Serial.println("Connection process started");
  apPaused = false;
  // mDNS joins the multicast group on both interfaces: it must leave it before the access point
  // interface is removed, or the lwIP IGMP timer sends on the removed interface (crash, exception 0)
  mdns.close();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  // Settings already in RAM: loaded at startup, updated by the web page
  delay(T_200MS);

  // Without home WiFi configured: access point only (the station would only disturb it)
  WiFi.mode(wifiConfigured ? WIFI_AP_STA : WIFI_AP);
  // Credentials only in RAM (no flash write at each attempt), reconnection managed by Network_Loop
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  // Mains powered: no modem sleep (with some access points/extenders it causes "bcn_timout" disconnections)
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  // Below the maximum (20.5 dBm): lower current peaks on the small Sonoff power supply, still plenty for a home
  WiFi.setOutputPower(WIFI_OUTPUT_POWER_DBM);
  // 802.11g only: the SDK crashed (exception 29 in ieee80211_setup_ratetable) updating the 802.11n
  // parameters announced by the access point beacons; plenty of speed for a switch
  WiFi.setPhyMode(WIFI_PHY_MODE_11G);

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

  StartAccessPoint();
  WebUI_Begin();

  // Reset last connection time
  lastTimeCheckConn = millis();
  lastWifiRetry = millis();
  wifiAttemptInProgress = false;
}

// Station not connected: short connection attempts, the access point stays usable in between
static void WifiReconnect() {
  if (deviceConnected) {
    deviceConnected = false;
    Serial.println("WiFi connection lost (status " + String(WiFi.status()) + ")");
    Led.Off();
    // First attempt in a few seconds
    lastWifiRetry = millis() - T_WIFI_RETRY + T_5S;
  }
  if (wifiAttemptInProgress) {
    if (millis() - wifiAttemptStart > T_WIFI_ATTEMPT) {
      // Stops the search (credentials kept), the access point gets its channel back
      WiFi.disconnect(false, false);
      wifiAttemptInProgress = false;
      lastWifiRetry = millis();
      Serial.println("WiFi: " + Ssid.Val + " not reachable, next attempt in " + String(T_WIFI_RETRY / 1000) + " s");
    }
    return;
  }
  if (millis() - lastWifiRetry > T_WIFI_RETRY) {
    lastWifiRetry = millis();
    if (WiFi.softAPgetStationNum() > 0) {
      if (postponedSince == 0) {
        postponedSince = millis();
      }
      if (millis() - postponedSince < T_WIFI_MAX_POSTPONE) {
        // Someone may be using the settings page: the attempt could disconnect them
        Serial.println("WiFi: attempt postponed, a device is connected to the access point");
        return;
      }
      Serial.println("WiFi: attempts postponed for " + String(T_WIFI_MAX_POSTPONE / 60000) + " min, trying anyway");
    }
    postponedSince = 0;
    Serial.println("WiFi: connecting to " + Ssid.Val);
    WiFi.begin(Ssid.Val, Password.Val);
    wifiAttemptInProgress = true;
    wifiAttemptStart = millis();
  }
}

void Network_Loop() {
  mdns.update();

  if (!wifiConfigured) {
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    WifiReconnect();
    return;
  }
  if (!deviceConnected) {
    // Connected again (also after a Connection_Manager timeout): MQTT host and topics from the current
    // settings, Mqtt_Loop then connects by itself
    wifiAttemptInProgress = false;
    deviceConnected = true;
    Serial.println("WiFi connected again, IP: " + WiFi.localIP().toString());
    Mqtt_Setup();
    Led.On();
  }

  // Periodic connection check
  if (millis() - lastTimeCheckConn > T_5MIN) {
    Serial.print("Connection check... ");
    Led.Off();
    delay(T_200MS);
    lastTimeCheckConn = millis();
    Serial.println("OK");
    PublishState();
    // Turns on the LED for network connection confirmation
    Led.On();
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
      // Values outside the range are clamped: below -> Unreacheable, above -> Very High
      int clampedPower = constrain(rawPower, minRawLevel, maxRawLevel);
      int power = map(clampedPower, minRawLevel, maxRawLevel, minLevel, maxLevel);
      Serial.println(levels[power]);
      Led.Blink(T_200MS, power, T_200MS);
    }
  }
  return result;
}
