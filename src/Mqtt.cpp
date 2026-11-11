#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Mqtt.h"
#include "Config.h"
#include "Settings.h"
#include "Device.h"
#include "LedStrip.h"
#include "Network.h"

static WiFiClient espClient;
static PubSubClient client(espClient);
static unsigned long lastMqttCheckConn = 0;

static void Callback(char *topic, byte *payload, unsigned int length) {
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
        Serial.println("  - color = R:" + String(Red.Val) + " G:" + String(Green.Val) + " B:" + String(Blue.Val));

      } else if (deviceMode == LEDSTRIP_MODE && key.equals("effect")) {
        String effect = doc["effect"].as<String>();
        if (LedStrip_SelectEffect(effect)) {
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

void Mqtt_Setup() {
  client.setServer(MqttServer.Val.c_str(), 1883);
  client.setCallback(Callback);
}

void Mqtt_Loop() {
  if (!deviceConnected || !mqttConfigured) {
    return;
  }
  if (client.connected()) {
    client.loop();
  } else if (millis() - lastMqttCheckConn > T_5S) {
    Serial.print("Connecting Mqtt client... ");
    lastMqttCheckConn = millis();
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

void Mqtt_Disconnect() {
  client.disconnect();
}

void PublishState() {
  if (!deviceConnected || !client.connected()) {
    return;
  }
  StaticJsonDocument <JSON_MSG_LENGTH> doc;
  char msg_out[JSON_MSG_LENGTH];

  if (!IsOn()) {
    doc["state"] = OFF_PAYLOAD;
  } else {
    doc["state"] = ON_PAYLOAD;
    if (deviceMode == LEDSTRIP_MODE) {
      doc["brightness"] = Brightness.Val;
      doc["effect"] = LedStrip_EffectName();
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
