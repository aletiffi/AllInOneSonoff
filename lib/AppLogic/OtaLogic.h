#ifndef OtaLogic_h
#define OtaLogic_h

// Firmware update logic, independent from Arduino (unit tested on PC)

#include <stddef.h>

enum OtaManifestResult {
  OTA_MANIFEST_OK,
  OTA_MANIFEST_INVALID_JSON,
  OTA_MANIFEST_MISSING_FIELDS,
  OTA_MANIFEST_FIELD_TOO_LONG
};

// <0 if a < b, 0 if equal, >0 if a > b. Dotted numeric versions: "45.10.0.0" > "45.9.0.0", "45.2" == "45.2.0.0"
int ota_compare_versions(const char* a, const char* b);

// True if the available version is newer than the current one
bool ota_is_update_needed(const char* available, const char* current);

// Only http:// and https:// urls are accepted
bool ota_is_valid_url(const char* url);
bool ota_is_https(const char* url);

// Google Drive share links -> direct download link, other urls are copied unchanged:
//   https://drive.google.com/file/d/<ID>/view?usp=sharing
//   https://drive.google.com/open?id=<ID>
// Returns false if out is too small
bool ota_normalize_url(const char* url, char* out, size_t outSize);

// Manifest: {"version": "45.2.0.0", "url": "https://..."}; the url is normalized
OtaManifestResult ota_parse_manifest(const char* json, char* version, size_t versionSize, char* url, size_t urlSize);

#endif
