#ifndef Ota_h
#define Ota_h

#include <Arduino.h>

// Firmware update from a json manifest reachable at OtaUrl (http or https, Google Drive links supported):
//   {"version": "45.2.0.0", "url": "https://.../firmware.bin"}
// The firmware is downloaded only if the manifest version is newer than FW_VERSION.
void Ota_Loop();
void Ota_Check();
void Ota_Request();
String Ota_Status();
String Ota_NormalizeUrl(const String& url);
int Ota_CompareVersions(const String& a, const String& b);

#endif
