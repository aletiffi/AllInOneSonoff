#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ReadInput.h>
#include <DigiOut.h>
#include <StoreStrings.h>
#include <FastLED.h>
#include <ArduinoJson.h>
#include <ESP8266httpUpdate.h>
#include "GlobalVar.h"
#include "HomePage.h"

WiFiClient espClient;
PubSubClient client (espClient);
ESP8266WebServer server(80);
MDNSResponder mdns;
ReadInput Button(SONOFF_BUTTON);
ReadInput Switch(SONOFF_GPIO14);
DigiOut Led(SONOFF_LED, OFF_LED);
DigiOut Rele(SONOFF_RELAY, OFF_RELAY);
StoreStrings mem(SETTIGNS_EEPROM_SIZE, STATE_EEPROM_SIZE);
CRGB leds[NUM_LEDS];

void PushButtonFunction(int func);
void Restart();
void Switch_On();
void Switch_Off();
void Switch_Toggle();
void PublishState();
void Callback(char *topic, byte *payload, unsigned int length);
void ShowIpAddr();
bool GetWifiPower(String netName);
void Connection_Manager();
void handleNotFound();
void CleanEEPROM();
void ChangeSettings();
void SendInfo();
void SaveSettingsInEeprom();
void LoadSettingsFromEeprom();
void SaveStateInEeprom();
void LoadStateFromEeprom();
void BuildFakeUrl();
void OtaUpdate();
// LedStrip mode
void SetLedStrip(byte red, byte green, byte blue, byte brightness);
void RgbEffectPlay();
byte RgbChannelManager(byte channelVal, byte targetVal, byte incStep);
byte RandomEffectStepSelector();
void ChangeBrightness();
void ChangeEffect();
void ChangeColor();
bool SelectEffect(String effect);
byte HexString2Byte(String val);


void setup() {
  Serial.begin(115200);
  delay(T_250MS);

  if (!mem.isReady()) {
    mem.clear();
  }

  Rele.Begin();
  Led.Begin();
  Led.Blink(T_100MS * 5, 1, T_100MS * 5);

  Serial.println("\n----------------------");
  Serial.println("Application ver. " + String(FW_VERSION) + " running!");

  // The device mode is needed before configuring GPIO14
  LoadSettingsFromEeprom();
  Serial.println("Device mode: " + String(deviceMode == LEDSTRIP_MODE ? MODE_LEDSTRIP : MODE_SWITCH));

  if (deviceMode == LEDSTRIP_MODE) {
    pinMode(SONOFF_GPIO14, OUTPUT);
    FastLED.addLeds<WS2811, SONOFF_GPIO14, BRG>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.clear();
    FastLED.show();
  } else {
    switchState = Switch.State();
    switchStatePre = switchState;
  }
  pushButton = !Button.State();
  pushButtonPre = pushButton;

  LoadStateFromEeprom();
  if (SwitchState.Val == 1) {
    Rele.On();
    if (deviceMode == LEDSTRIP_MODE) {
      SetLedStrip(Red.Val, Green.Val, Blue.Val, Brightness.Val);
    }
  } else {
    Rele.Off();
  }
  BuildFakeUrl();
  Connection_Manager();
  randomSeed(millis());
}

void loop() {
  if (deviceConnected && mqttConfigured) {
    if (client.connected()) {
      client.loop();
    } else {
      if (millis() - lastMqttCheckConn > T_5S) {
        Serial.print("Connecting Mqtt client... ");
        lastMqttCheckConn = millis();
        // Try to reconnect
        if (client.connect(Hostname.Val.c_str(), MqttUser.Val.c_str(), MqttPassword.Val.c_str())) {
          lastMqttCheckConn = 0;
          client.subscribe(MqttSubTopic.Val.c_str());
          Serial.println("Ok");
          PublishState();
        } else {
          Serial.println("Failed");
        }
      }
    }
  }
  server.handleClient();
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
      OtaUpdate();
      PublishState();
      // Turns on the LED for network connection confirmation
      Led.On();
    }
  }

  // Multifunction button pressure control
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

  if (deviceMode == SWITCH_MODE) {
    // Wall switch: every change of position toggles the relay
    switchState = Switch.State();
    if (switchState != switchStatePre) {
      Switch_Toggle();
    }
    switchStatePre = switchState;

  } else {
    // Ledstrip effects
    if (RGB_Efect_Selected != Solid.Effect_Number && Rele.State() != OFF_RELAY) {
      if (millis() - lastTimeRgbPlay >= T_250MS) {
        lastTimeRgbPlay = millis();
        RgbEffectPlay();
      }
    }
  }
}

void PushButtonFunction(int func) {
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
      OtaUpdate();
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

void Restart() {
  Serial.println("Restart");
  delay(T_250MS);
  Led.Blink(T_200MS, 5, T_100MS);
  ESP.restart();
}

void Switch_On() {
  if (deviceMode == LEDSTRIP_MODE) {
    if (Rele.State() == OFF_RELAY) {
      Rele.On();
      SwitchState.Val = 1;
      // Power supply delay
      delay(T_1S);
    }
    // Also publishes the state
    SetLedStrip(Red.Val, Green.Val, Blue.Val, Brightness.Val);
  } else {
    Rele.On();
    SwitchState.Val = 1;
    if (deviceConnected) {
      PublishState();
    }
  }
  SaveStateInEeprom();
}

void Switch_Off() {
  if (deviceMode == LEDSTRIP_MODE) {
    FastLED.clear();
    FastLED.show();
    RGB_Efect_Selected = Solid.Effect_Number;
  }
  Rele.Off();
  SwitchState.Val = 0;

  if (deviceConnected) {
    PublishState();
  }
  SaveStateInEeprom();
}

void Switch_Toggle() {
  if (Rele.State() != OFF_RELAY) {
    Switch_Off();
  } else {
    Switch_On();
  }
}

void PublishState() {
  StaticJsonDocument <JSON_MSG_LENGTH> doc;
  char msg_out[JSON_MSG_LENGTH];

  if (Rele.State() == OFF_RELAY) {
    doc["state"] = OFF_PAYLOAD;
  } else {
    doc["state"] = ON_PAYLOAD;
    if (deviceMode == LEDSTRIP_MODE) {
      doc["brightness"] = Brightness.Val;
      doc["effect"] = RbgEffects[RGB_Efect_Selected]->Effect_Name;
      JsonObject color = doc.createNestedObject("color");
      color["r"] = Red.Val;
      color["g"] = Green.Val;
      color["b"] = Blue.Val;
    }
  }
  serializeJson(doc, msg_out);
  if (client.publish(MqttPubTopic.Val.c_str(), msg_out)) {
    Serial.print("Message published: ");
    Serial.println(msg_out);
  } else {
    Serial.println("Message publishing error");
  }
}

void Callback(char *topic, byte *payload, unsigned int length) {
  StaticJsonDocument <JSON_MSG_LENGTH> doc;
  Serial.print("Payload recived: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print(char(payload[i]));
  }
  Serial.println("");

  if ((String(topic) == MqttSubTopic.Val) && (length > 5)) {
    String state;
    Serial.println("Command/s on payload: ");
    if (deserializeJson(doc, payload, length)) {
      Serial.println("  - invalid json");
      return;
    }
    JsonObject root = doc.as<JsonObject>();

    for (JsonPair kv : root) {
      String key = kv.key().c_str();

      if (key.equals("state")) {
        state = doc["state"].as<String>();
        Serial.println("  - state = " + state);

      } else if (deviceMode == LEDSTRIP_MODE && key.equals("brightness")) {
        Brightness.Val = doc["brightness"];
        Serial.println("  - brightness = " + String(Brightness.Val));

      } else if (deviceMode == LEDSTRIP_MODE && key.equals("color")) {
        JsonObject color = doc["color"];
        Red.Val = color["r"];
        Green.Val = color["g"];
        Blue.Val = color["b"];
        Serial.println("  - color = R:" + String(Red.Val) + " G:" + String(Green.Val) + " B:"  + String(Blue.Val));

      } else if (deviceMode == LEDSTRIP_MODE && key.equals("effect")) {
        String effect = doc["effect"].as<String>();
        if (SelectEffect(effect)) {
          Serial.println("  - effect = " + effect);
        }

      } else {
        Serial.println("  - key ignored = " + key);
      }
    }

    if (state.equals(OFF_PAYLOAD)) {
      Switch_Off();
    }
    if (state.equals(ON_PAYLOAD)) {
      Switch_On();
    }
  }
}

void ShowIpAddr() {
  if (deviceConnected) {
    String ip = WiFi.localIP().toString();
    byte numIterations = 0;
    String ipNumStr = "0";
    byte ipNum = 0;

    Serial.println(ip);
    for (uint16_t i = 0; i < ip.length(); i++) {
      if (ip.charAt(i) == '.') {
        numIterations++;
      }
      if (numIterations == 3) {
        ipNumStr = ip.substring(i + 1, ip.length());
        ipNum = ipNumStr.toInt();
        break;
      }
    }

    Led.Blink(T_200MS, ipNum, T_200MS);

  } else {

    Serial.println("Not connected!");
    // Connection failed
    Led.Blink(T_200MS, 10, T_100MS);
  }
}

bool GetWifiPower(String netName) {
  int netsNumber = WiFi.scanNetworks();
  int minRawLevel = -75;
  int maxRawLevel = -45;
  int minLevel = 0;
  int maxLevel = 5;
  bool result = false;

  if (netsNumber > 0) {
    for (int i = 0; i < netsNumber; ++i) {
      if (String(WiFi.SSID(i)) == netName) {
        result = true;
        int rawPower = WiFi.RSSI(i);
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(rawPower);
        Serial.print(")");
        if (minRawLevel <= rawPower && rawPower <= maxRawLevel) {
          int power = map(rawPower, minRawLevel, maxRawLevel, minLevel, maxLevel);
          switch (power) {
            case 0:
              Serial.println(" Unreacheable");
              break;
            case 1:
              Serial.println(" Very Low");
              break;
            case 2:
              Serial.println(" Low");
              break;
            case 3:
              Serial.println(" Good");
              break;
            case 4:
              Serial.println(" High");
              break;
            case 5:
              Serial.println(" Very High!");
              break;
          }
          Led.Blink(T_200MS, power, T_200MS);
        } else {
          Serial.println(" Signal level out of bounds");
        }
      }
    }
  }
  return result;
}

void Connection_Manager() {
  Serial.println("Connection process started");
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  delay(T_200MS);
  LoadSettingsFromEeprom();

  WiFi.mode(WIFI_AP_STA);

  if (Ssid.Val != NULL_CHAR && Password.Val != NULL_CHAR) {
    if (GetWifiPower(Ssid.Val)) {
      WiFi.begin(Ssid.Val, Password.Val);
      Serial.println("Connecting to: " + String(Ssid.Val));
      byte numCehck = 0;
      while ((WiFi.status() != WL_CONNECTED)) {
        if (numCehck >= 150) {
          lastTimeCheckConn = millis();
          break;
        }
        numCehck++;
        delay(T_200MS);
      }
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected!");
    if (mac.equals("")) {
      String macTemp = String(WiFi.macAddress());
      for (byte i = 0; i < macTemp.length(); i++) {
        if (macTemp.charAt(i) != ':') {
          mac += macTemp.charAt(i);
        }
      }
      OtaUpdate();
    }
    Serial.println("IP: " + WiFi.localIP().toString() + " Mac: " + String(WiFi.macAddress()));
    client.setServer(MqttServer.Val.c_str(), 1883);
    client.setCallback(Callback);

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

  // WebServer configuration
  if (Hostname.Val == "" || Hostname.Val.startsWith(" ") || Hostname.Val.length() >= MAX_LENGTH_SETTING) {
    Hostname.Val = DEFAULT_AP_NAME;
  }
  Serial.println("AP Name: " + Hostname.Val);

  WiFi.softAP(Hostname.Val.c_str());
  mdns.begin(Hostname.Val, WiFi.localIP());
  mdns.addService("http", "tcp", 80);

  // Routes are registered only once, Connection_Manager can be called many times
  if (!webServerStarted) {
    server.on("/", []() {
      server.send_P(200, "text/html", webPage);
    });
    server.on("/info", HTTP_GET, SendInfo);
    server.on("/on", []() {
      server.sendHeader("Location", "/");
      server.send(303);
      Switch_On();
    });
    server.on("/off", []() {
      server.sendHeader("Location", "/");
      server.send(303);
      Switch_Off();
    });
    server.on("/restart", []() {
      server.sendHeader("Location", "/");
      server.send(303);
      Restart();
    });
    server.on("/brigthness", HTTP_POST, ChangeBrightness);
    server.on("/effect", HTTP_POST, ChangeEffect);
    server.on("/color", HTTP_POST, ChangeColor);
    server.on("/settings", HTTP_POST, ChangeSettings);
    server.on("/clean", HTTP_GET, CleanEEPROM);
    server.onNotFound(handleNotFound);
    server.begin();
    webServerStarted = true;
  }

  // Reset last connection time
  lastTimeCheckConn = millis();
}

void handleNotFound() {
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

void SendInfo() {
  // Used by the home page to show the controls of the current mode
  StaticJsonDocument <JSON_MSG_LENGTH> doc;
  String msg_out;
  char color[8];

  doc["mode"] = deviceMode == LEDSTRIP_MODE ? MODE_LEDSTRIP : MODE_SWITCH;
  doc["hostname"] = Hostname.Val;
  doc["version"] = FW_VERSION;
  doc["state"] = Rele.State() == OFF_RELAY ? OFF_PAYLOAD : ON_PAYLOAD;
  if (deviceMode == LEDSTRIP_MODE) {
    snprintf(color, sizeof(color), "#%02x%02x%02x", Red.Val, Green.Val, Blue.Val);
    doc["color"] = color;
    doc["brightness"] = Brightness.Val;
    doc["effect"] = RbgEffects[RGB_Efect_Selected]->Effect_Name;
  }
  serializeJson(doc, msg_out);
  server.send(200, "application/json", msg_out);
}

void CleanEEPROM() {
  Led.Blink(T_200MS, 3, T_200MS);
  mem.clear();
  mem.print_all();
  Led.Blink(T_200MS, 3, T_200MS);
  Restart();
}

void ChangeSettings() {
  StaticJsonDocument <512> doc;
  String data = server.arg("plain");
  String oldMode = DeviceMode.Val;
  Serial.println("Parameters recived:");
  Serial.println(data);
  deserializeJson(doc, data);
  JsonObject root = doc.as<JsonObject>();
  for (JsonPair kv : root) {
    for (byte i = 0; i < NUM_WIFI_SETTINGS; i++) {
      if (strcmp(kv.key().c_str(), WifiSettings[i]->Name.c_str()) == 0) {
        String val = kv.value().as<String>();
        if (val != " " && val != "" && !val.startsWith(" ")) {
          WifiSettings[i]->Val = val;
        }
      }
    }
  }
  // Only known modes are accepted
  if (DeviceMode.Val != MODE_SWITCH && DeviceMode.Val != MODE_LEDSTRIP) {
    DeviceMode.Val = oldMode;
  }

  SaveSettingsInEeprom();
  server.send(200, "text/plain", "Settings saved");

  if (DeviceMode.Val != oldMode && !(oldMode == "" && DeviceMode.Val == MODE_SWITCH)) {
    // GPIO14 changes function: a restart is required
    Serial.println("Device mode changed to " + DeviceMode.Val);
    Switch_Off();
    Restart();
  }
  Connection_Manager();
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
    Serial.println( WifiSettings[i]->Name + ": " +  WifiSettings[i]->Val);
  }

  if ( MqttSubTopic.Val != NULL_CHAR && MqttPubTopic.Val != NULL_CHAR && MqttServer.Val != NULL_CHAR &&
       MqttUser.Val != NULL_CHAR && MqttPassword.Val != NULL_CHAR) {
    mqttConfigured = true;
  } else {
    mqttConfigured = false;
  }

  if (Ssid.Val != NULL_CHAR && Password.Val != NULL_CHAR) {
    wifiConfigured = true;
  } else {
    wifiConfigured = false;
  }

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
    // Values never saved keep their default
    if (val.length() > 0 && val != NULL_CHAR) {
      StateSettings[i]->Val = val.toInt();
    }
    Serial.print( StateSettings[i]->Name + ": " +  StateSettings[i]->Val + " ");
  }
  Serial.println("");
}

void BuildFakeUrl() {
  // To be able to change product key
  String fake_url = "http://otadrive.com/DeviceApi/GetEsp8266Update?";
  fake_url += "&s=" + mac;
  fake_url += FW_URL_MKR(FAKE_API_KEY, FW_VERSION);
}

void OtaUpdate() {
  String url = "http://otadrive.com/DeviceApi/GetEsp8266Update?";
  url += "&s=" + mac + "&_FirmwareInfo&k=" + OtaDriveProductKey.Val + "&v=" + FW_VERSION + "&FirmwareInfo_&";

  Serial.println("Get firmware from url:");
  Serial.println(url);

  t_httpUpdate_return ret = ESPhttpUpdate.update(espClient, url, FW_VERSION);
  switch (ret)
  {
    case HTTP_UPDATE_FAILED:
      Serial.printf("Update Faild, Error: (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("No new update available");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("Update OK");
      break;
    default:
      Serial.println(ret);
      break;
  }
}

//----------------------------------------------------------------------------
// LedStrip mode
//----------------------------------------------------------------------------
void SetLedStrip(byte red, byte green, byte blue, byte brightness) {
  if (deviceMode != LEDSTRIP_MODE) {
    return;
  }
  fill_solid(leds, NUM_LEDS, CRGB(red, green, blue));
  FastLED.setBrightness(brightness);
  FastLED.show();
  Serial.println("LedStrip updated -> R:" + String(red) + " G:" + String(green) + " B:" + String(blue) + " Brightness:" + String(brightness));
  if (deviceConnected && RGB_Efect_Selected == Solid.Effect_Number) {
    PublishState();
  }
}

void RgbEffectPlay() {
  if (RGB_Efect_Selected == Random.Effect_Number) {
    if (RandomEffectStep >= RGB_COMBO_NUM) {
      RGB_Efect_Selected = Solid.Effect_Number;
      return;
    }
    byte target = Random_Effects[RandomEffectStep]->TargetVal;
    // Steps 0,3,6 -> red; 1,4,7 -> green; 2,5,8 -> blue
    switch (RandomEffectStep % 3) {
      case 0:
        Red.Val = RgbChannelManager(Red.Val, target, INCREMENT_STEP);
        break;
      case 1:
        Green.Val = RgbChannelManager(Green.Val, target, INCREMENT_STEP);
        break;
      case 2:
        Blue.Val = RgbChannelManager(Blue.Val, target, INCREMENT_STEP);
        break;
    }
    SetLedStrip(Red.Val, Green.Val, Blue.Val, Brightness.Val);
  }
}

byte RgbChannelManager(byte channelVal, byte targetVal, byte incStep) {
  if (channelVal < targetVal) {
    // Increase
    if ((targetVal - channelVal) < incStep) {
      return targetVal;
    } else {
      return channelVal + incStep;
    }
  } else if (channelVal > targetVal) {
    // Decrease
    if ((channelVal - targetVal) < incStep) {
      return targetVal;
    } else {
      return channelVal - incStep;
    }
  } else {
    // Target reached: choose the next step
    RandomEffectStep = RandomEffectStepSelector();
    return channelVal;
  }
}

byte RandomEffectStepSelector() {
  byte randSelection = random(0, RGB_COMBO_NUM);
  // Verifies that not all channels are disabled
  while ((Random_Effects[randSelection]->TargetVal == 0 ) && ((Red.Val == 0 && Green.Val == 0) || (Red.Val == 0 && Blue.Val == 0) || (Blue.Val == 0 && Green.Val == 0))) {
    randSelection = random(0, RGB_COMBO_NUM);
  }
  return randSelection;
}

bool SelectEffect(String effect) {
  for (byte i = 0; i < RGB_EFFECTS_NUM; i++) {
    if (effect.equals(RbgEffects[i]->Effect_Name)) {
      RGB_Efect_Selected = RbgEffects[i]->Effect_Number;
      return true;
    }
  }
  return false;
}

void ChangeBrightness() {
  StaticJsonDocument <JSON_MSG_LENGTH> doc;
  deserializeJson(doc, server.arg("plain"));
  if (deviceMode == LEDSTRIP_MODE && doc.containsKey("Brightness")) {
    Brightness.Val = constrain(doc["Brightness"].as<String>().toInt(), 0, MAX_RGB_VAL);
    Serial.println("Brightness recived: " + String(Brightness.Val));
    SetLedStrip(Red.Val, Green.Val, Blue.Val, Brightness.Val);
    SaveStateInEeprom();
  }
  server.send(200, "text/plain", "OK");
}

void ChangeEffect() {
  StaticJsonDocument <JSON_MSG_LENGTH> doc;
  deserializeJson(doc, server.arg("plain"));
  if (deviceMode == LEDSTRIP_MODE && doc.containsKey("effect")) {
    String effect = doc["effect"].as<String>();
    if (SelectEffect(effect)) {
      Serial.println("Effect recived: " + effect);
    }
  }
  server.send(200, "text/plain", "OK");
}

void ChangeColor() {
  StaticJsonDocument <JSON_MSG_LENGTH> doc;
  deserializeJson(doc, server.arg("plain"));
  if (deviceMode == LEDSTRIP_MODE && doc.containsKey("color")) {
    String color = doc["color"].as<String>();
    if (color.length() == 7 && color.charAt(0) == '#') {
      Red.Val = HexString2Byte(color.substring(1, 3));
      Green.Val = HexString2Byte(color.substring(3, 5));
      Blue.Val = HexString2Byte(color.substring(5, 7));
      Serial.println("Color recived: " + color + " -> R: " + String(Red.Val) + " G: " + String(Green.Val) + " B: " + String(Blue.Val));
      SetLedStrip(Red.Val, Green.Val, Blue.Val, Brightness.Val);
      SaveStateInEeprom();
    } else {
      Serial.println("Color recived: " + color + " -> unable to decode");
    }
  }
  server.send(200, "text/plain", "OK");
}

byte HexString2Byte(String val) {
  return (byte) strtol(val.c_str(), NULL, 16);
}
