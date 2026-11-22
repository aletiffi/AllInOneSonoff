#ifndef Mqtt_h
#define Mqtt_h

#include <Arduino.h>

// MQTT client with Home Assistant integration (discovery, availability, birth message).
// Topics and payloads are described in lib/AppLogic/HaDiscovery.h
void Mqtt_Setup();
void Mqtt_Loop();
// Publishes "offline" and disconnects (before a restart)
void Mqtt_Disconnect();
// Disconnects without "offline" and frees the buffer, to have memory for the OTA TLS connections:
// Mqtt_Loop reconnects as soon as it runs again
void Mqtt_Suspend();
void PublishState();
void Mqtt_PublishUpdateState(bool inProgress);
void Mqtt_PublishTelemetry();
String Mqtt_BaseTopic();
String Mqtt_CommandTopic();
String Mqtt_StateTopic();
String Mqtt_UniqueId();
bool Mqtt_IsConnected();

#endif
