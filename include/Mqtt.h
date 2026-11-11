#ifndef Mqtt_h
#define Mqtt_h

#include <Arduino.h>

// MQTT client with Home Assistant integration (discovery, availability, birth message).
// Topics and payloads are described in lib/AppLogic/HaDiscovery.h
void Mqtt_Setup();
void Mqtt_Loop();
void Mqtt_Disconnect();
void PublishState();
void Mqtt_PublishUpdateState(bool inProgress);
void Mqtt_PublishTelemetry();
String Mqtt_BaseTopic();
String Mqtt_CommandTopic();
String Mqtt_StateTopic();
String Mqtt_UniqueId();
bool Mqtt_IsConnected();

#endif
