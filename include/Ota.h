#ifndef Ota_h
#define Ota_h

#include <Arduino.h>

// Firmware update from a json manifest reachable at OtaUrl (http or https, Google Drive links supported):
//   {"version": "45.4.0.0", "url": "https://.../AllInOneSonoff_v45.4.0.0.bin"}
// The firmware is downloaded only if the manifest version is newer than FW_VERSION and,
// if keys/public.key existed at build time, it is installed only if signed with keys/private.key.
// The pure logic (versions, urls, manifest) is in lib/AppLogic/OtaLogic.
void Ota_Loop();
void Ota_Check();
void Ota_Request();
String Ota_Status();
// Version of the last manifest read, "" if never read
String Ota_LatestVersion();
bool Ota_IsSigningEnabled();

#endif
