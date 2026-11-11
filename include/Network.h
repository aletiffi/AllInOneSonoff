#ifndef Network_h
#define Network_h

#include <Arduino.h>

// Wifi station + access point, mDNS and periodic connection check
extern bool deviceConnected;

void Connection_Manager();
void Network_Loop();
void ShowIpAddr();
bool GetWifiPower(const String& netName);

#endif
