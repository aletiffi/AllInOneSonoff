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
  RUN_TEST(test_parse_manifest_errors);
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
