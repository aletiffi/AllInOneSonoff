#include "OtaLogic.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ArduinoJson.h>

static const char DRIVE_DOWNLOAD[] = "https://drive.google.com/uc?export=download&confirm=t&id=";

int ota_compare_versions(const char* a, const char* b) {
  const char* pa = a ? a : "";
  const char* pb = b ? b : "";
  while (*pa || *pb) {
    char* end;
    long na = strtol(pa, &end, 10);
    pa = end;
    long nb = strtol(pb, &end, 10);
    pb = end;
    if (na != nb) {
      return na < nb ? -1 : 1;
    }
    // Skips up to the next segment (also ignores suffixes like "-beta")
    while (*pa && *pa != '.') pa++;
    while (*pb && *pb != '.') pb++;
    if (*pa == '.') pa++;
    if (*pb == '.') pb++;
  }
  return 0;
}

bool ota_is_update_needed(const char* available, const char* current) {
  return available && *available && ota_compare_versions(available, current) > 0;
}

bool ota_is_https(const char* url) {
  return url && strncmp(url, "https://", 8) == 0 && url[8] != '\0';
}

bool ota_is_valid_url(const char* url) {
  return ota_is_https(url) || (url && strncmp(url, "http://", 7) == 0 && url[7] != '\0');
}

bool ota_is_disabled(const char* setting) {
  return setting && strcasecmp(setting, "off") == 0;
}

const char* ota_effective_url(const char* setting, const char* defaultUrl) {
  if (ota_is_disabled(setting)) {
    return NULL;
  }
  return ota_is_valid_url(setting) ? setting : defaultUrl;
}

static bool CopyString(const char* src, size_t len, char* out, size_t outSize) {
  if (len + 1 > outSize) {
    return false;
  }
  memcpy(out, src, len);
  out[len] = '\0';
  return true;
}

// Finds the Google Drive file id, returns its length (0 = not a Drive share link)
static size_t FindDriveId(const char* url, const char** id) {
  const char* fileTag = "drive.google.com/file/d/";
  const char* openTag = "drive.google.com/open?id=";
  const char* p = strstr(url, fileTag);
  const char* stop = "/?";

  if (p) {
    p += strlen(fileTag);
  } else if ((p = strstr(url, openTag)) != NULL) {
    p += strlen(openTag);
    stop = "&#";
  } else {
    return 0;
  }
  *id = p;
  return strcspn(p, stop);
}

bool ota_normalize_url(const char* url, char* out, size_t outSize) {
  if (!url || !out || outSize == 0) {
    return false;
  }
  const char* id = NULL;
  size_t idLen = FindDriveId(url, &id);

  if (idLen > 0) {
    size_t prefixLen = strlen(DRIVE_DOWNLOAD);
    if (prefixLen + idLen + 1 > outSize) {
      return false;
    }
    memcpy(out, DRIVE_DOWNLOAD, prefixLen);
    memcpy(out + prefixLen, id, idLen);
    out[prefixLen + idLen] = '\0';
    return true;
  }
  return CopyString(url, strlen(url), out, outSize);
}

OtaManifestResult ota_parse_manifest(const char* json, char* version, size_t versionSize, char* url, size_t urlSize) {
  StaticJsonDocument<512> doc;
  if (!json || deserializeJson(doc, json) || !doc.is<JsonObject>()) {
    return OTA_MANIFEST_INVALID_JSON;
  }
  const char* v = doc["version"] | "";
  const char* u = doc["url"] | "";
  if (*v == '\0' || !ota_is_valid_url(u)) {
    return OTA_MANIFEST_MISSING_FIELDS;
  }
  if (!CopyString(v, strlen(v), version, versionSize) || !ota_normalize_url(u, url, urlSize)) {
    return OTA_MANIFEST_FIELD_TOO_LONG;
  }
  return OTA_MANIFEST_OK;
}

// --- Redirects ---

bool ota_split_url(const char* url, bool* https, char* host, size_t hostSize, uint16_t* port, const char** path) {
  if (!ota_is_valid_url(url) || !host || hostSize == 0) {
    return false;
  }
  bool secure = ota_is_https(url);
  const char* p = url + (secure ? 8 : 7);
  size_t authLen = strcspn(p, "/?#");
  const char* colon = (const char*) memchr(p, ':', authLen);
  size_t hostLen = colon ? (size_t)(colon - p) : authLen;
  if (hostLen == 0 || hostLen >= hostSize) {
    return false;
  }
  uint16_t prt = secure ? 443 : 80;
  if (colon) {
    char* end;
    long v = strtol(colon + 1, &end, 10);
    if (end != p + authLen || v <= 0 || v > 65535) {
      return false;
    }
    prt = (uint16_t) v;
  }
  memcpy(host, p, hostLen);
  host[hostLen] = '\0';
  if (https) *https = secure;
  if (port) *port = prt;
  if (path) *path = p + authLen;
  return true;
}

bool ota_is_redirect(int code) {
  return code == 301 || code == 302 || code == 303 || code == 307 || code == 308;
}

enum {
  HP_STATUS,          // reading the status line
  HP_NAME,            // reading a header name
  HP_VALUE_START,     // Location: skipping the spaces before the value
  HP_VALUE,           // Location: reading the value
  HP_SKIP,            // skipping the rest of the line
  HP_DONE
};

static void EndStatusLine(OtaHeaderParser* p) {
  p->line[p->lineLen] = '\0';
  // "HTTP/1.1 302 Found"
  if (strncmp(p->line, "HTTP/", 5) == 0) {
    const char* sp = strchr(p->line, ' ');
    if (sp) {
      p->status = atoi(sp + 1);
    }
  }
}

static void EndLocation(OtaHeaderParser* p) {
  // Trailing spaces
  while (p->locationLen > 0 && (p->location[p->locationLen - 1] == ' ' || p->location[p->locationLen - 1] == '\t')) {
    p->locationLen--;
  }
  p->location[p->locationLen] = '\0';
  p->hasLocation = p->locationLen > 0 && !p->locationTooLong;
}

void ota_header_parser_init(OtaHeaderParser* p, char* locationBuf, size_t locationSize) {
  memset(p, 0, sizeof(*p));
  p->state = HP_STATUS;
  p->location = locationBuf;
  p->locationSize = locationSize;
  if (locationBuf && locationSize > 0) {
    locationBuf[0] = '\0';
  }
}

bool ota_header_parser_feed(OtaHeaderParser* p, char c) {
  if (p->state == HP_DONE) {
    return true;
  }
  if (c == '\r') {
    return false;
  }
  if (c == '\n') {
    switch (p->state) {
      case HP_STATUS:
        EndStatusLine(p);
        break;
      case HP_NAME:
        if (p->lineLen == 0) {
          // Empty line: end of the header
          p->state = HP_DONE;
          return true;
        }
        break;
      case HP_VALUE_START:
      case HP_VALUE:
        EndLocation(p);
        break;
      default:
        break;
    }
    p->state = HP_NAME;
    p->lineLen = 0;
    return false;
  }

  switch (p->state) {
    case HP_STATUS:
      if (p->lineLen < sizeof(p->line) - 1) {
        p->line[p->lineLen++] = c;
      }
      break;
    case HP_NAME:
      if (c == ':') {
        p->line[p->lineLen] = '\0';
        if (strcasecmp(p->line, "Location") == 0 && p->location && p->locationSize > 0) {
          p->locationLen = 0;
          p->locationTooLong = false;
          p->hasLocation = false;
          p->state = HP_VALUE_START;
        } else {
          p->state = HP_SKIP;
        }
      } else if (p->lineLen < sizeof(p->line) - 1) {
        p->line[p->lineLen++] = c;
      } else {
        // Name too long: not Location
        p->state = HP_SKIP;
      }
      break;
    case HP_VALUE_START:
      if (c == ' ' || c == '\t') {
        break;
      }
      p->state = HP_VALUE;
      // fall through
    case HP_VALUE:
      if (p->locationLen < p->locationSize - 1) {
        p->location[p->locationLen++] = c;
      } else {
        p->locationTooLong = true;
      }
      break;
    default:
      break;
  }
  return false;
}

static bool Prepend(char* buf, size_t bufSize, const char* prefix, size_t prefixLen) {
  size_t len = strlen(buf);
  if (prefixLen + len + 1 > bufSize) {
    return false;
  }
  memmove(buf + prefixLen, buf, len + 1);
  memcpy(buf, prefix, prefixLen);
  return true;
}

bool ota_resolve_location(const char* base, char* buf, size_t bufSize) {
  if (!buf || bufSize == 0 || *buf == '\0') {
    return false;
  }
  if (ota_is_valid_url(buf)) {
    return true;
  }
  if (!ota_is_valid_url(base)) {
    return false;
  }
  size_t schemeLen = ota_is_https(base) ? 6 : 5;      // "https:" / "http:"
  if (buf[0] == '/' && buf[1] == '/') {
    return Prepend(buf, bufSize, base, schemeLen);
  }
  const char* auth = base + schemeLen + 2;
  const char* pathStart = auth + strcspn(auth, "/?#");
  if (buf[0] == '/') {
    return Prepend(buf, bufSize, base, (size_t)(pathStart - base));
  }
  // Relative to the directory of the base path (query excluded)
  const char* pathEnd = pathStart + strcspn(pathStart, "?#");
  const char* lastSlash = NULL;
  for (const char* q = pathStart; q < pathEnd; q++) {
    if (*q == '/') lastSlash = q;
  }
  if (lastSlash) {
    return Prepend(buf, bufSize, base, (size_t)(lastSlash + 1 - base));
  }
  // Base without path: "scheme://host" + "/" + location
  return Prepend(buf, bufSize, "/", 1) && Prepend(buf, bufSize, base, (size_t)(pathStart - base));
}
