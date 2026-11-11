#ifndef Mqtt_h
#define Mqtt_h

// MQTT client: json commands on MqttSubTopic, json state on MqttPubTopic
void Mqtt_Setup();
void Mqtt_Loop();
void Mqtt_Disconnect();
void PublishState();

#endif
