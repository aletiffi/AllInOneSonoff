#include "OtaLogic.h"
#include <string.h>
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
