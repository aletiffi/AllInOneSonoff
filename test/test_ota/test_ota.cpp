#include <unity.h>
#include <string.h>
#include "OtaLogic.h"

void setUp() {}
void tearDown() {}

static void test_compare_versions() {
  TEST_ASSERT_EQUAL_INT(0, ota_compare_versions("45.2.0.0", "45.2.0.0"));
  TEST_ASSERT_TRUE(ota_compare_versions("45.2.0.1", "45.2.0.0") > 0);
  TEST_ASSERT_TRUE(ota_compare_versions("45.1.9.9", "45.2.0.0") < 0);
  // Numeric, not alphabetical
  TEST_ASSERT_TRUE(ota_compare_versions("45.10.0.0", "45.9.0.0") > 0);
  TEST_ASSERT_TRUE(ota_compare_versions("100.0", "99.9") > 0);
  // Missing segments are 0
  TEST_ASSERT_EQUAL_INT(0, ota_compare_versions("45.2", "45.2.0.0"));
  TEST_ASSERT_TRUE(ota_compare_versions("45.2.0.0.1", "45.2") > 0);
  // Invalid input does not crash
  TEST_ASSERT_EQUAL_INT(0, ota_compare_versions(NULL, ""));
  TEST_ASSERT_TRUE(ota_compare_versions("1", NULL) > 0);
}

static void test_update_needed() {
  TEST_ASSERT_TRUE(ota_is_update_needed("45.3.0.0", "45.2.0.0"));
  TEST_ASSERT_FALSE(ota_is_update_needed("45.2.0.0", "45.2.0.0"));
  // Never downgrade
  TEST_ASSERT_FALSE(ota_is_update_needed("45.1.0.0", "45.2.0.0"));
  TEST_ASSERT_FALSE(ota_is_update_needed("", "45.2.0.0"));
  TEST_ASSERT_FALSE(ota_is_update_needed(NULL, "45.2.0.0"));
}

static void test_valid_url() {
  TEST_ASSERT_TRUE(ota_is_valid_url("https://example.com/fw.bin"));
  TEST_ASSERT_TRUE(ota_is_valid_url("http://192.168.1.10/fw.bin"));
  TEST_ASSERT_FALSE(ota_is_valid_url("https://"));
  TEST_ASSERT_FALSE(ota_is_valid_url("ftp://example.com/fw.bin"));
  // Old OTA Drive product key stored in the same EEPROM position
  TEST_ASSERT_FALSE(ota_is_valid_url("abc12345-1a1a-12a1-a1ab-a1ab1a1ab123"));
  TEST_ASSERT_FALSE(ota_is_valid_url(""));
  TEST_ASSERT_FALSE(ota_is_valid_url(NULL));
  TEST_ASSERT_TRUE(ota_is_https("https://example.com"));
  TEST_ASSERT_FALSE(ota_is_https("http://example.com"));
}

static void test_normalize_drive_file_link() {
  char out[200];
  TEST_ASSERT_TRUE(ota_normalize_url("https://drive.google.com/file/d/1AbC_dEf-123/view?usp=sharing", out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("https://drive.google.com/uc?export=download&confirm=t&id=1AbC_dEf-123", out);
  TEST_ASSERT_TRUE(ota_normalize_url("https://drive.google.com/file/d/XYZ", out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("https://drive.google.com/uc?export=download&confirm=t&id=XYZ", out);
  TEST_ASSERT_TRUE(ota_normalize_url("https://drive.google.com/file/d/XYZ?usp=drive_link", out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("https://drive.google.com/uc?export=download&confirm=t&id=XYZ", out);
}

static void test_normalize_drive_open_link() {
  char out[200];
  TEST_ASSERT_TRUE(ota_normalize_url("https://drive.google.com/open?id=QWE&authuser=0", out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("https://drive.google.com/uc?export=download&confirm=t&id=QWE", out);
}

static void test_normalize_other_urls_unchanged() {
  char out[200];
  const char* url = "https://github.com/aletiffi/AllInOneSonoff/releases/latest/download/fw.bin";
  TEST_ASSERT_TRUE(ota_normalize_url(url, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING(url, out);
  // Already a direct download link
  url = "https://drive.google.com/uc?export=download&id=AAA";
  TEST_ASSERT_TRUE(ota_normalize_url(url, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING(url, out);
}

static void test_normalize_buffer_too_small() {
  char out[20];
  TEST_ASSERT_FALSE(ota_normalize_url("https://drive.google.com/file/d/XYZ/view", out, sizeof(out)));
  TEST_ASSERT_FALSE(ota_normalize_url("https://example.com/a/very/long/path.bin", out, sizeof(out)));
  TEST_ASSERT_TRUE(ota_normalize_url("http://a/b.bin", out, sizeof(out)));
}

static void test_parse_manifest_ok() {
  char version[16], url[200];
  const char* json = "{\"version\":\"45.3.0.0\",\"url\":\"https://drive.google.com/file/d/BIN/view?usp=sharing\"}";
  TEST_ASSERT_EQUAL_INT(OTA_MANIFEST_OK, ota_parse_manifest(json, version, sizeof(version), url, sizeof(url)));
  TEST_ASSERT_EQUAL_STRING("45.3.0.0", version);
  TEST_ASSERT_EQUAL_STRING("https://drive.google.com/uc?export=download&confirm=t&id=BIN", url);
}

static void test_parse_manifest_github_release() {
  // Manifest created by .github/workflows/release.yml
  char version[24], url[256];
  const char* json = "{\n  \"version\": \"45.5.0.0\",\n  \"url\": \"https://github.com/aletiffi/AllInOneSonoff/releases/download/v45.5.0.0/AllInOneSonoff_v45.5.0.0.ota.bin.gz\"\n}\n";
  TEST_ASSERT_EQUAL_INT(OTA_MANIFEST_OK, ota_parse_manifest(json, version, sizeof(version), url, sizeof(url)));
  TEST_ASSERT_EQUAL_STRING("45.5.0.0", version);
  TEST_ASSERT_EQUAL_STRING("https://github.com/aletiffi/AllInOneSonoff/releases/download/v45.5.0.0/AllInOneSonoff_v45.5.0.0.ota.bin.gz", url);
  // Url of the latest manifest: fits in the settings field (150 chars) and is not modified
  const char* latest = "https://github.com/aletiffi/AllInOneSonoff/releases/latest/download/manifest.json";
  TEST_ASSERT_TRUE(strlen(latest) <= 150);
  TEST_ASSERT_TRUE(ota_normalize_url(latest, url, sizeof(url)));
  TEST_ASSERT_EQUAL_STRING(latest, url);
}

static void test_effective_url() {
  const char* def = "https://github.com/aletiffi/AllInOneSonoff/releases/latest/download/manifest.json";
  // Empty setting: default (latest GitHub release)
  TEST_ASSERT_EQUAL_STRING(def, ota_effective_url("", def));
  TEST_ASSERT_EQUAL_STRING(def, ota_effective_url(NULL, def));
  // Old OTA Drive product key in the same EEPROM position, or a typo: default
  TEST_ASSERT_EQUAL_STRING(def, ota_effective_url("abc12345-1a1a-12a1-a1ab-a1ab1a1ab123", def));
  TEST_ASSERT_EQUAL_STRING(def, ota_effective_url("drive.google.com/file/d/X", def));
  // Custom url (e.g. Google Drive or a test manifest): used as it is
  const char* drive = "https://drive.google.com/file/d/MANIFEST/view?usp=sharing";
  TEST_ASSERT_EQUAL_STRING(drive, ota_effective_url(drive, def));
  TEST_ASSERT_EQUAL_STRING("http://192.168.1.10/manifest.json", ota_effective_url("http://192.168.1.10/manifest.json", def));
  // "off": updates disabled
  TEST_ASSERT_NULL(ota_effective_url("off", def));
  TEST_ASSERT_NULL(ota_effective_url("OFF", def));
  TEST_ASSERT_TRUE(ota_is_disabled("Off"));
  TEST_ASSERT_FALSE(ota_is_disabled("offline"));
  TEST_ASSERT_FALSE(ota_is_disabled(""));
  TEST_ASSERT_FALSE(ota_is_disabled(NULL));
}

static void test_parse_manifest_errors() {
  char version[16], url[200];
  TEST_ASSERT_EQUAL_INT(OTA_MANIFEST_INVALID_JSON, ota_parse_manifest("<html>Google Drive login</html>", version, sizeof(version), url, sizeof(url)));
  TEST_ASSERT_EQUAL_INT(OTA_MANIFEST_INVALID_JSON, ota_parse_manifest("[1,2]", version, sizeof(version), url, sizeof(url)));
  TEST_ASSERT_EQUAL_INT(OTA_MANIFEST_INVALID_JSON, ota_parse_manifest(NULL, version, sizeof(version), url, sizeof(url)));
  TEST_ASSERT_EQUAL_INT(OTA_MANIFEST_MISSING_FIELDS, ota_parse_manifest("{\"version\":\"45.3.0.0\"}", version, sizeof(version), url, sizeof(url)));
  TEST_ASSERT_EQUAL_INT(OTA_MANIFEST_MISSING_FIELDS, ota_parse_manifest("{\"url\":\"https://a/b.bin\"}", version, sizeof(version), url, sizeof(url)));
  TEST_ASSERT_EQUAL_INT(OTA_MANIFEST_MISSING_FIELDS, ota_parse_manifest("{\"version\":\"1\",\"url\":\"file://a\"}", version, sizeof(version), url, sizeof(url)));
  TEST_ASSERT_EQUAL_INT(OTA_MANIFEST_MISSING_FIELDS, ota_parse_manifest("{\"version\":45,\"url\":\"https://a/b.bin\"}", version, sizeof(version), url, sizeof(url)));
  TEST_ASSERT_EQUAL_INT(OTA_MANIFEST_FIELD_TOO_LONG, ota_parse_manifest("{\"version\":\"45.3.0.0-very-long-suffix\",\"url\":\"https://a/b.bin\"}", version, sizeof(version), url, sizeof(url)));
}

static void test_split_url() {
  bool https;
  char host[32];
  uint16_t port;
  const char* path;
  TEST_ASSERT_TRUE(ota_split_url("https://github.com/a/b?x=1", &https, host, sizeof(host), &port, &path));
  TEST_ASSERT_TRUE(https);
  TEST_ASSERT_EQUAL_STRING("github.com", host);
  TEST_ASSERT_EQUAL_UINT16(443, port);
  TEST_ASSERT_EQUAL_STRING("/a/b?x=1", path);

  TEST_ASSERT_TRUE(ota_split_url("http://192.168.1.10:8080/fw.bin", &https, host, sizeof(host), &port, &path));
  TEST_ASSERT_FALSE(https);
  TEST_ASSERT_EQUAL_STRING("192.168.1.10", host);
  TEST_ASSERT_EQUAL_UINT16(8080, port);
  TEST_ASSERT_EQUAL_STRING("/fw.bin", path);

  TEST_ASSERT_TRUE(ota_split_url("http://host", &https, host, sizeof(host), &port, &path));
  TEST_ASSERT_EQUAL_UINT16(80, port);
  TEST_ASSERT_EQUAL_STRING("", path);

  TEST_ASSERT_FALSE(ota_split_url("ftp://host/x", &https, host, sizeof(host), &port, &path));
  TEST_ASSERT_FALSE(ota_split_url("http://host:abc/x", &https, host, sizeof(host), &port, &path));
  TEST_ASSERT_FALSE(ota_split_url("http://host:70000/x", &https, host, sizeof(host), &port, &path));
  TEST_ASSERT_FALSE(ota_split_url("https:///x", &https, host, sizeof(host), &port, &path));
  // Host too long for the buffer
  TEST_ASSERT_FALSE(ota_split_url("https://a-very-long-host-name.example.com/x", &https, host, 16, &port, &path));
}

static void test_is_redirect() {
  TEST_ASSERT_TRUE(ota_is_redirect(301));
  TEST_ASSERT_TRUE(ota_is_redirect(302));
  TEST_ASSERT_TRUE(ota_is_redirect(303));
  TEST_ASSERT_TRUE(ota_is_redirect(307));
  TEST_ASSERT_TRUE(ota_is_redirect(308));
  TEST_ASSERT_FALSE(ota_is_redirect(200));
  TEST_ASSERT_FALSE(ota_is_redirect(304));
  TEST_ASSERT_FALSE(ota_is_redirect(404));
}

// Feeds the whole string, returns true if the header was completed
static bool Feed(OtaHeaderParser* p, const char* s) {
  bool done = false;
  for (; *s; s++) {
    done = ota_header_parser_feed(p, *s);
  }
  return done;
}

static void test_header_parser_github_redirect() {
  char location[128];
  OtaHeaderParser p;
  ota_header_parser_init(&p, location, sizeof(location));
  TEST_ASSERT_FALSE(Feed(&p, "HTTP/1.1 302 Found\r\nServer: github.com\r\n"));
  // Header much longer than any buffer: skipped without problems
  TEST_ASSERT_FALSE(Feed(&p, "Content-Security-Policy: "));
  for (int i = 0; i < 5000; i++) {
    ota_header_parser_feed(&p, 'x');
  }
  TEST_ASSERT_FALSE(Feed(&p, "\r\nlocation:   https://release-assets.example.com/a?b=1  \r\nContent-Length: 0\r\n"));
  TEST_ASSERT_TRUE(Feed(&p, "\r\n"));
  TEST_ASSERT_EQUAL_INT(302, p.status);
  TEST_ASSERT_TRUE(p.hasLocation);
  TEST_ASSERT_EQUAL_STRING("https://release-assets.example.com/a?b=1", location);
  // Body after the header is ignored
  TEST_ASSERT_TRUE(ota_header_parser_feed(&p, 'z'));
}

static void test_header_parser_no_location() {
  char location[64];
  OtaHeaderParser p;
  ota_header_parser_init(&p, location, sizeof(location));
  // Only \n as line terminator, header name longer than the internal buffer
  TEST_ASSERT_TRUE(Feed(&p, "HTTP/1.0 200 OK\nX-A-Very-Long-Header-Name-Over-The-Buffer: Location: no\nLocationX: no\n\n"));
  TEST_ASSERT_EQUAL_INT(200, p.status);
  TEST_ASSERT_FALSE(p.hasLocation);
  TEST_ASSERT_EQUAL_STRING("", location);
}

static void test_header_parser_location_too_long() {
  char location[16];
  OtaHeaderParser p;
  ota_header_parser_init(&p, location, sizeof(location));
  TEST_ASSERT_TRUE(Feed(&p, "HTTP/1.1 301 Moved\r\nLocation: https://example.com/very/long/path\r\n\r\n"));
  TEST_ASSERT_EQUAL_INT(301, p.status);
  TEST_ASSERT_FALSE(p.hasLocation);
  TEST_ASSERT_TRUE(p.locationTooLong);
}

static void test_header_parser_not_http() {
  char location[16];
  OtaHeaderParser p;
  ota_header_parser_init(&p, location, sizeof(location));
  TEST_ASSERT_TRUE(Feed(&p, "garbage\r\n\r\n"));
  TEST_ASSERT_EQUAL_INT(0, p.status);
}

static void test_resolve_location() {
  char buf[96];
  const char* base = "https://github.com:8443/owner/repo/releases/latest?x=/y";

  strcpy(buf, "https://other.com/file");
  TEST_ASSERT_TRUE(ota_resolve_location(base, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL_STRING("https://other.com/file", buf);

  strcpy(buf, "//cdn.com/file");
  TEST_ASSERT_TRUE(ota_resolve_location(base, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL_STRING("https://cdn.com/file", buf);

  strcpy(buf, "/owner/repo/releases/download/v1/m.json");
  TEST_ASSERT_TRUE(ota_resolve_location(base, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL_STRING("https://github.com:8443/owner/repo/releases/download/v1/m.json", buf);

  strcpy(buf, "m.json");
  TEST_ASSERT_TRUE(ota_resolve_location(base, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL_STRING("https://github.com:8443/owner/repo/releases/m.json", buf);

  strcpy(buf, "m.json");
  TEST_ASSERT_TRUE(ota_resolve_location("http://host", buf, sizeof(buf)));
  TEST_ASSERT_EQUAL_STRING("http://host/m.json", buf);

  // Buffer too small, empty location, invalid base
  char small[24] = "/a/long/path/to/file";
  TEST_ASSERT_FALSE(ota_resolve_location("https://github.com/x", small, sizeof(small)));
  buf[0] = '\0';
  TEST_ASSERT_FALSE(ota_resolve_location(base, buf, sizeof(buf)));
  strcpy(buf, "/x");
  TEST_ASSERT_FALSE(ota_resolve_location("ftp://host/", buf, sizeof(buf)));
}

int runUnityTests() {
  UNITY_BEGIN();
  RUN_TEST(test_compare_versions);
  RUN_TEST(test_update_needed);
  RUN_TEST(test_valid_url);
  RUN_TEST(test_normalize_drive_file_link);
  RUN_TEST(test_normalize_drive_open_link);
  RUN_TEST(test_normalize_other_urls_unchanged);
  RUN_TEST(test_normalize_buffer_too_small);
  RUN_TEST(test_parse_manifest_ok);
  RUN_TEST(test_parse_manifest_github_release);
  RUN_TEST(test_effective_url);
  RUN_TEST(test_parse_manifest_errors);
  RUN_TEST(test_split_url);
  RUN_TEST(test_is_redirect);
  RUN_TEST(test_header_parser_github_redirect);
  RUN_TEST(test_header_parser_no_location);
  RUN_TEST(test_header_parser_location_too_long);
  RUN_TEST(test_header_parser_not_http);
  RUN_TEST(test_resolve_location);
  return UNITY_END();
}

#ifdef ARDUINO
#include <Arduino.h>
void setup() {
  delay(2000);
  runUnityTests();
}
void loop() {}
#else
int main() {
  return runUnityTests();
}
#endif
