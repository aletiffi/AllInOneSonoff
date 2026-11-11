#ifndef Config_h
#define Config_h

//----------------------------------------------------------------------------
// Firmware
//----------------------------------------------------------------------------
// Releases: set by the GitHub pipeline from the git tags (FW_VERSION environment variable,
// see scripts/signing.py). This value is used for local builds and as minimum version.
#ifndef FW_VERSION
#define FW_VERSION                "45.4.0.0"
#endif
#define DEFAULT_AP_NAME           "SonoffAllInOne"
//----------------------------------------------------------------------------
// Sonoff r1 pins
//----------------------------------------------------------------------------
#define SONOFF_BUTTON             0
#define SONOFF_RELAY              12
#define SONOFF_LED                13
#define SONOFF_GPIO14             14          // Wall switch input (Switch mode) or ledstrip data (LedStrip mode)
//----------------------------------------------------------------------------
#define OFF_RELAY                 LOW
#define OFF_LED                   HIGH
//----------------------------------------------------------------------------
// Ledstrip
//----------------------------------------------------------------------------
#define NUM_LEDS                  33
#define MAX_RGB_VAL               255
#define INCREMENT_STEP            3
//----------------------------------------------------------------------------
// Timings
//----------------------------------------------------------------------------
#define T_100MS                   100
#define T_200MS                   200
#define T_250MS                   250
#define T_1S                      1000        // Delay change function
#define T_5S                      5000
#define T_10S                     10000
#define T_5MIN                    300000      // Connection check every 5 min
#define T_1H                      3600000     // Firmware update check every hour
//----------------------------------------------------------------------------
// EEPROM
//----------------------------------------------------------------------------
#define SETTIGNS_EEPROM_SIZE      400
#define STATE_EEPROM_SIZE         26
#define MAX_LENGTH_SETTING        20
#define MAX_LENGTH_URL            150
//----------------------------------------------------------------------------
// Firmware updates: used when the "Firmware update url" setting is empty.
// Always points to the manifest of the latest GitHub release.
//----------------------------------------------------------------------------
#define OTA_DEFAULT_URL           "https://github.com/aletiffi/AllInOneSonoff/releases/latest/download/manifest.json"
//----------------------------------------------------------------------------
// Json messages (payloads and mode names are in lib/AppLogic)
//----------------------------------------------------------------------------
#define JSON_MSG_LENGTH           256
//----------------------------------------------------------------------------

#endif
