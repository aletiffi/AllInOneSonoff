#ifndef Network_h
#define Network_h

#include <Arduino.h>

// Wifi station + access point, mDNS and periodic connection check.
// When the network is lost: short reconnection attempts every 30 s, the access point stays usable
extern bool deviceConnected;

void Connection_Manager();
void Network_Loop();
void ShowIpAddr();
bool GetWifiPower(const String& netName);
// Access point off during the firmware download: the TLS connection leaves a few KB of memory and a
// device joining the access point made the SDK crash (failed allocation in ieee80211_setup_ratetable)
void Network_PauseAccessPoint();
void Network_ResumeAccessPoint();

#endif
