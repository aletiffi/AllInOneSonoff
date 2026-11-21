#ifndef OtaLogic_h
#define OtaLogic_h

// Firmware update logic, independent from Arduino (unit tested on PC)

#include <stddef.h>
#include <stdint.h>

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

// Url of the manifest to use, from the "Firmware update url" setting:
//   empty or not an http(s) url (e.g. old OTA Drive key)  -> defaultUrl
//   "off" (any case)                                       -> NULL, updates disabled
//   anything else                                          -> the setting
const char* ota_effective_url(const char* setting, const char* defaultUrl);
bool ota_is_disabled(const char* setting);

// Manifest: {"version": "45.2.0.0", "url": "https://..."}; the url is normalized
OtaManifestResult ota_parse_manifest(const char* json, char* version, size_t versionSize, char* url, size_t urlSize);

// --- Redirects ---
// The ESP8266 HTTPClient stores each response header line in a String growing a few bytes at a time:
// github.com sends a ~4 KB Content-Security-Policy header that, with the TLS buffers allocated,
// runs out of memory and the Location header can get lost (-> "HTTP 302").
// These functions follow the redirects with fixed buffers, reading only the status and Location.

// http(s)://host[:port][/path] -> host, port (default 80/443), https and path (pointer into url, "" if missing).
// False if the url is not valid or host doesn't fit
bool ota_split_url(const char* url, bool* https, char* host, size_t hostSize, uint16_t* port, const char** path);

// 301, 302, 303, 307, 308
bool ota_is_redirect(int code);

// Streaming parser of an HTTP response header, fed one byte at a time: memory used is fixed,
// lines too long (other than Location) are skipped
struct OtaHeaderParser {
  int state;
  char line[24];          // status line start or header name
  size_t lineLen;
  char* location;         // buffer for the Location value
  size_t locationSize;
  size_t locationLen;
  int status;             // HTTP status code, 0 if not received
  bool hasLocation;
  bool locationTooLong;
};
void ota_header_parser_init(OtaHeaderParser* p, char* locationBuf, size_t locationSize);
// True when the header is complete (empty line)
bool ota_header_parser_feed(OtaHeaderParser* p, char c);

// Redirect location -> absolute url, in place (buf contains the Location value, NUL terminated):
//   "https://host/x" unchanged, "//host/x" -> scheme of base, "/x" -> scheme://host[:port] of base,
//   "x" -> directory of the base path + x
// False if buf is too small or base is not valid
bool ota_resolve_location(const char* base, char* buf, size_t bufSize);

#endif
