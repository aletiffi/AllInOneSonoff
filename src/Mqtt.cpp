#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <MqttLogic.h>
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
  Serial.print("Payload recived: ");
  Serial.write(payload, length);
  Serial.println("");

  if (MqttSubTopic.Val != topic) {
    return;
  }
  LightCommand cmd;
  if (!mqtt_parse_command((const char*) payload, length, deviceMode == LEDSTRIP_MODE, &cmd)) {
    Serial.println("  - invalid json");
    return;
  }
  if (cmd.hasBrightness) {
    Brightness.Val = cmd.brightness;
    Serial.println("  - brightness = " + String(Brightness.Val));
  }
  if (cmd.hasColor) {
    Red.Val = cmd.color.r;
    Green.Val = cmd.color.g;
    Blue.Val = cmd.color.b;
    Serial.println("  - color = " + LedStrip_ColorHex());
  }
  if (cmd.hasEffect) {
    LedStrip_SelectEffect(effect_name(cmd.effect));
    Serial.println("  - effect = " + LedStrip_EffectName());
  }
  if (cmd.hasState) {
    Serial.println(String("  - state = ") + (cmd.on ? ON_PAYLOAD : OFF_PAYLOAD));
    if (cmd.on) {
      Switch_On();
    } else {
      Switch_Off();
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
  char msg_out[JSON_MSG_LENGTH];
  LightState state = {IsOn(), deviceMode == LEDSTRIP_MODE, Brightness.Val, {Red.Val, Green.Val, Blue.Val}, LedStrip_Effect()};

  if (mqtt_build_state(&state, msg_out, sizeof(msg_out)) == 0) {
    Serial.println("Message too long");
    return;
  }
  if (client.publish(MqttPubTopic.Val.c_str(), msg_out)) {
    Serial.print("Message published: ");
    Serial.println(msg_out);
  } else {
    Serial.println("Message publishing error");
  }
}
