# AllInOneSonoff

Firmware for the **Sonoff Basic R1** (ESP8266) that turns it into **a relay switch** or **an RGB led strip controller**. You choose the mode from a web page, and you can switch between them at any time.

- **Switch**: the Sonoff turns a light bulb, socket or other load on and off. You can control it from the wall switch, the Sonoff button, the web page, MQTT or Home Assistant.
- **LedStrip**: the Sonoff powers a **WS2812B** strip through its relay and controls it on GPIO14: color, brightness, effects.

What it can do:
- built-in web page for configuration and control, which also works without a WiFi network (access point mode);
- **automatic** integration with **Home Assistant** (MQTT discovery): the device appears by itself, no YAML needed;
- **OTA** firmware updates (over WiFi) from any address (Google Drive, GitHub...), accepting only firmware **signed** with your key;
- it remembers its state when power goes away (on/off, color, brightness);
- unit tests that check the logic on the PC, before you try it on the hardware.

This project merges the old [SwitchSonoff](https://github.com/aletiffi/SwitchSonoff) and [LedstripSonoff](https://github.com/aletiffi/LedstripSonoff).

<p align="center"><img src="img/HomePage.PNG" alt="Home page"/></p>

---

## Contents

1. [What you need](#1-what-you-need)
2. [Wiring](#2-wiring)
3. [Installing the software (VS Code + PlatformIO)](#3-installing-the-software-vs-code--platformio)
4. [First flash (by serial)](#4-first-flash-by-serial)
5. [First setup](#5-first-setup)
6. [Daily use: web page and button](#6-daily-use-web-page-and-button)
7. [Home Assistant](#7-home-assistant)
8. [MQTT without Home Assistant](#8-mqtt-without-home-assistant)
9. [Firmware updates (OTA)](#9-firmware-updates-ota)
10. [For developers](#10-for-developers)
11. [Troubleshooting](#11-troubleshooting)

---

## 1. What you need

| Item | Notes |
|---|---|
| Sonoff Basic **R1** | Must have the 5-pin header (3V3, RX, TX, GND, GPIO14) |
| **3.3 V** USB-serial adapter | For example FTDI, CP2102 or CH340. **Set to 3.3 V, not 5 V**, or the ESP8266 will be damaged |
| 4-5 female-female jumper wires | Plus a pin header to solder onto the Sonoff, if it doesn't have one |
| *Switch mode:* a wall switch | A normal switch (not a push button) |
| *LedStrip mode:* a WS2812B strip and a **5 V** power supply | About 60 mA per led at full white: 33 leds ≈ 2 A, so take a 3 A supply |
| A PC with [VS Code](https://code.visualstudio.com/) | Windows, Linux or macOS |

> ⚠️ **Mains safety.** The Sonoff runs on 230 V. **Never** connect the serial adapter while the Sonoff is plugged into mains, and never touch the board while it is powered. The low-voltage parts (GPIO14, GND, the led strip) are **not** guaranteed to be isolated from mains: install everything inside a closed box. If you're not sure, ask an electrician.

## 2. Wiring

The Sonoff's **GPIO14** has a different job depending on the mode:

| Mode | GPIO14 | Relay (L/N output) |
|---|---|---|
| Switch | Wall switch input: switch between GPIO14 and GND | Load (light bulb, socket...) |
| LedStrip | Strip data line (DIN) | Mains input of the strip's 5 V power supply |

**Switch mode:**

<p align="center"><img src="img/Schema.png" alt="Switch wiring"/></p>

Every time the switch changes position (up or down) the relay flips its state. The wall switch and Home Assistant work together, like a two-way (deviator) switch.

**LedStrip mode:**
- Sonoff L/N output → power supply input (the Sonoff turns the supply off when the strip is off);
- 5 V power supply → strip +5V and GND;
- Sonoff GPIO14 → strip DIN; Sonoff GND → strip GND (a common ground is required).
- Default setup: 33 leds, BRG color order. To change them, edit `NUM_LEDS` in `include/Config.h` and `BRG` in `src/LedStrip.cpp`. If red and blue come out swapped, use `RGB` or `GRB`.

## 3. Installing the software (VS Code + PlatformIO)

1. Install [VS Code](https://code.visualstudio.com/).
2. In VS Code open *Extensions* (Ctrl+Shift+X), search for **PlatformIO IDE** and install it. Wait for it to finish its setup, then restart VS Code.
3. Download this project (`git clone https://github.com/aletiffi/AllInOneSonoff.git`, or *Code > Download ZIP* on GitHub) and open the folder with *File > Open Folder*.
4. The first time, PlatformIO downloads the compiler and the libraries (a few minutes).

All the useful commands are available as **tasks**: menu *Terminal > Run Task...* (or Ctrl+Shift+P → "Tasks: Run Task").

| Task | What it does |
|---|---|
| **Build** (Ctrl+Shift+B) | Compiles the firmware |
| **Flash (serial)** | Compiles and writes the firmware to the Sonoff |
| **Serial monitor** | Shows the Sonoff log (useful when something doesn't work) |
| **Flash + Serial monitor** | Both, in sequence |
| **Unit test (PC)** | Runs the tests on the PC, no hardware needed |
| **Unit test (Sonoff)** | Runs the tests on the Sonoff (then flash the firmware again) |
| **Erase flash** | Erases the Sonoff completely, settings included |
| **Clean** | Deletes the build files |
| **Tests + Build** | Tests and then build: the check to run before publishing an update |

From the terminal, the same commands are `pio run`, `pio run -t upload`, `pio device monitor`, `pio test -e native`.

**After every build** you'll find two files in the project folder:
- `AllInOneSonoff_v<version>.bin`: the firmware to write **by serial**;
- `AllInOneSonoff_v<version>.ota.bin.gz`: compressed and **signed** firmware, for **OTA** updates (see [section 9](#9-firmware-updates-ota)).

The very first build creates the signing keys in `keys/`. **Make a copy of `keys/private.key`** right away in a safe place (password manager, USB stick). It isn't saved on git, and without it you can't update your devices over WiFi.

## 4. First flash (by serial)

The first flash must be done by cable. All the following updates can be done over WiFi.

1. **Unplug the Sonoff from mains.**
2. Connect the adapter: adapter **3.3V → 3V3**, **GND → GND**, adapter **TX → RX** of the Sonoff, adapter **RX → TX** of the Sonoff. TX and RX must be crossed.
3. Hold the **Sonoff button** pressed and plug the adapter into the PC. Release the button after a couple of seconds: the Sonoff is now in "programming mode".
4. Check the port in `platformio.ini` (`upload_port`): on Linux it's usually `/dev/ttyUSB0`, on Windows `COM3`, `COM4`... (see Device Manager).
5. Run the **Erase flash** task (only the first time). Then repeat step 3 and run **Flash (serial)**.
6. When the flash is done, unplug and plug back the adapter (without pressing the button) to start the firmware. With **Serial monitor** you can see the log.

> On Linux, if you get *permission denied* on `/dev/ttyUSB0`: `sudo usermod -aG dialout $USER`, then log out and back in.

## 5. First setup

1. When it first starts the Sonoff creates a WiFi network called **SonoffAllInOne**. Connect to it with your phone or PC (no password).
2. Open the browser at **http://192.168.4.1**.
3. In the **Settings** section fill in:

| Field | Required? | What to put |
|---|---|---|
| Device mode | yes | **Switch** or **LedStrip** |
| Hostname | recommended | Device name, e.g. `Cucina`. Also used as the name in Home Assistant and in the MQTT topics |
| Ssid / Password | yes | Name and password of your home WiFi |
| MQTT Server | for Home Assistant | IP address of the MQTT broker, e.g. `192.168.1.10` (or `192.168.1.10:1883`) |
| MQTT User / Password | if the broker requires them | User created for MQTT (see [section 7](#7-home-assistant)) |
| MQTT command/state topic | no | Leave empty: the defaults are `sonoff/<hostname>/set` and `sonoff/<hostname>/state` |
| Firmware update url | no | Address of the update manifest, see [section 9](#9-firmware-updates-ota) |

4. Press **Save Settings**. The Sonoff connects to your WiFi. When it's connected, the Sonoff's led stays on.
5. From now on the page is at `http://<hostname>.local` (e.g. `http://cucina.local`) or at the IP assigned by the router.

Empty fields don't overwrite the saved values: you can change one field only and leave the others empty. To delete the update url, enter `-`. When you change the mode the Sonoff restarts by itself.

<p align="center"><img src="img/Settings.PNG" alt="Settings"/></p>

## 6. Daily use: web page and button

**Web page:** ON/OFF buttons. In LedStrip mode you also get color, brightness and effect (Solid = fixed color, Random = colors that change slowly). Under the title you see the mode, the state, the firmware version, the MQTT state and the topics in use.

**Integrated button:** hold it pressed. The led flashes once per second; release it after the number of flashes of the function you want:

| Flashes | Function |
|---|---|
| 1 | On/off |
| 2 | Reconnect to WiFi |
| 3 | Check for firmware updates |
| 4 | Show the IP address (number of flashes = last number of the IP) |
| 5 | WiFi signal strength (0-5 flashes) |
| 6 / 7 | Load / save settings (debug) |
| 8 | **Erase all settings** |
| 9 | Print the EEPROM on the serial port (debug) |
| 10 | Restart |

**Status led:** on = connected to WiFi; 10 fast flashes = WiFi connection failed.

## 7. Home Assistant

### 7.1 What you need on the Home Assistant side (one time only)

Home Assistant talks to the Sonoff through an **MQTT broker** (a "post office" for messages). The simplest way is the official add-on:

1. *Settings > Add-ons > Add-on Store* → install **Mosquitto broker** → *Start* (enable *Start on boot*).
2. *Settings > People > Users* (advanced mode required) → create a user, for example `mqtt_sonoff` with a password. You'll use it in the Sonoff's MQTT User/Password fields.
3. *Settings > Devices & services*: Home Assistant suggests the **MQTT** integration → *Configure* → *Submit*.

In the Sonoff settings put: **MQTT Server** = Home Assistant IP, **MQTT User/Password** = the user you created.

> If you use another broker (e.g. Mosquitto on a NAS), put its IP in the Sonoff and configure the MQTT integration in Home Assistant with the same broker.

### 7.2 Automatic integration (recommended): nothing to write in YAML

The Sonoff uses **MQTT discovery**: as soon as it connects, it tells Home Assistant which entities it has. Go to *Settings > Devices & services > MQTT > Devices*: you'll find a device with the name of the Sonoff, with:

| Entity | Type | Mode |
|---|---|---|
| *Device name* | `switch` (on/off) | Switch |
| *Device name* | `light` (color, brightness, effects) | LedStrip |
| Restart | button | both |
| Check firmware update | button | both |
| Firmware | `update` (installed / available version, Install button) | both |
| WiFi signal | sensor in dBm (diagnostic) | both |
| IP address | sensor (diagnostic) | both |

Also:
- **Availability**: if the Sonoff loses power or WiFi, within a few seconds its entities show as *unavailable* (thanks to the MQTT "last will").
- When Home Assistant restarts, the Sonoff notices and announces itself again (birth message `homeassistant/status`). You don't have to do anything.
- If you change the mode (Switch ↔ LedStrip), the old entity is removed and the new one appears.
- **Is the relay powering a lamp?** You'd rather see it as a *light*: open the switch entity → ⚙️ → *Show as* → **Light**.
- The device page has the *Visit* link that opens the Sonoff web page.

It follows the current Home Assistant documentation: [MQTT discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) (device discovery), [JSON light](https://www.home-assistant.io/integrations/light.mqtt/) with `supported_color_modes`, [switch](https://www.home-assistant.io/integrations/switch.mqtt/), [button](https://www.home-assistant.io/integrations/button.mqtt/), [update](https://www.home-assistant.io/integrations/update.mqtt/).

### 7.3 Manual YAML configuration (only if you don't use discovery)

Only needed if you have disabled MQTT discovery in Home Assistant. If you write the YAML **and** discovery is active, you'll get duplicated entities. The complete file is in [`doc/home_assistant_manual.yaml`](doc/home_assistant_manual.yaml); copy it into `configuration.yaml` and replace `cucina` with your hostname (lowercase, spaces → `_`).

> ❌ **Old format, no longer working** (it was in the old projects' documentation):
> ```yaml
> light:
>   - platform: mqtt      # removed from Home Assistant
>     schema: json
> ```
> ✅ **Current format:** everything under the `mqtt:` key:

```yaml
mqtt:
  # Switch mode
  - switch:
      name: "Cucina"
      unique_id: sonoff_cucina_switch
      command_topic: "sonoff/cucina/set"
      state_topic: "sonoff/cucina/state"
      value_template: "{{ value_json.state }}"
      payload_on: "ON"
      payload_off: "OFF"
      availability_topic: "sonoff/cucina/availability"

  # LedStrip mode
  - light:
      name: "Led cucina"
      unique_id: sonoff_cucina_light
      schema: json
      command_topic: "sonoff/cucina/set"
      state_topic: "sonoff/cucina/state"
      availability_topic: "sonoff/cucina/availability"
      supported_color_modes: ["rgb"]
      brightness: true
      effect: true
      effect_list: ["Solid", "Random"]

  # Restart button
  - button:
      name: "Cucina restart"
      unique_id: sonoff_cucina_restart
      command_topic: "sonoff/cucina/action"
      payload_press: "restart"
      device_class: restart
```

After changing `configuration.yaml`: *Developer tools > YAML > Check configuration*, then *MQTT entities* (or restart Home Assistant).

### 7.4 Automation and dashboard examples

Turn on the led strip at sunset, orange at 60%:

```yaml
automation:
  - alias: "Led cucina at sunset"
    triggers:
      - trigger: sun
        event: sunset
    actions:
      - action: light.turn_on
        target:
          entity_id: light.cucina
        data:
          rgb_color: [255, 120, 0]
          brightness_pct: 60
```

Turn on the random effect:

```yaml
action: light.turn_on
target:
  entity_id: light.cucina
data:
  effect: "Random"
```

Turn off the switch at midnight:

```yaml
automation:
  - alias: "Turn off at midnight"
    triggers:
      - trigger: time
        at: "00:00:00"
    actions:
      - action: switch.turn_off
        target:
          entity_id: switch.cucina
```

Notification when the Sonoff goes offline for more than 5 minutes:

```yaml
automation:
  - alias: "Sonoff cucina offline"
    triggers:
      - trigger: state
        entity_id: switch.cucina
        to: "unavailable"
        for: "00:05:00"
    actions:
      - action: notify.notify
        data:
          message: "The Sonoff in the kitchen is not responding"
```

Dashboard card (*Edit dashboard > Add card > Manual*):

```yaml
type: entities
title: Kitchen
entities:
  - entity: light.cucina
  - entity: update.cucina_firmware
  - entity: sensor.cucina_wifi_signal
  - entity: button.cucina_restart
```

> The `entity_id`s depend on the device name: check them under *Settings > Devices & services > Entities*.

## 8. MQTT without Home Assistant

You can control the Sonoff from any MQTT client (Node-RED, `mosquitto_pub`, phone apps...). `<base>` is `sonoff/<hostname>` (lowercase, spaces → `_`), the exact topics are shown in the web page.

| Topic | Direction | Content |
|---|---|---|
| `<base>/set` | to the Sonoff | `ON`, `OFF`, `TOGGLE` or json `{"state":"ON"}`. LedStrip: `{"state":"ON","brightness":120,"color":{"r":255,"g":0,"b":80},"effect":"Random"}` (each field is optional) |
| `<base>/state` | from the Sonoff | `{"state":"OFF"}` / `{"state":"ON"}`. LedStrip on: `{"state":"ON","color_mode":"rgb","brightness":120,"effect":"Solid","color":{"r":255,"g":0,"b":80}}` |
| `<base>/availability` | from the Sonoff | `online` / `offline` (retained) |
| `<base>/action` | to the Sonoff | `restart`, `check_update`, `install` |
| `<base>/update` | from the Sonoff | `{"installed_version":"45.4.0.0","latest_version":"45.4.0.0","in_progress":false}` |
| `<base>/telemetry` | from the Sonoff | `{"rssi":-61,"ip":"192.168.1.20","uptime":3600}` every 5 minutes |
| `homeassistant/device/sonoff_<id>/config` | from the Sonoff | Discovery for Home Assistant |

Examples from the terminal:

```bash
# Turn on
mosquitto_pub -h 192.168.1.10 -u mqtt_sonoff -P password -t sonoff/cucina/set -m ON
# Purple at half brightness
mosquitto_pub -h 192.168.1.10 -u mqtt_sonoff -P password -t sonoff/cucina/set \
  -m '{"state":"ON","brightness":128,"color":{"r":160,"g":0,"b":255}}'
# Watch everything the Sonoff publishes
mosquitto_sub -h 192.168.1.10 -u mqtt_sonoff -P password -t 'sonoff/#' -v
```

The MQTT_Sub / MQTT_Pub topics of the old firmwares still work: if you fill them in the settings, they replace `<base>/set` and `<base>/state`.

## 9. Firmware updates (OTA)

After the first serial flash, updates arrive over WiFi.

### 9.1 How it works

In the settings, **Firmware update url** contains the address of a small **manifest** file:

```json
{
  "version": "45.4.0.0",
  "url": "https://drive.google.com/file/d/<ID_OF_THE_OTA_FILE>/view?usp=sharing"
}
```

The Sonoff reads the manifest at startup, then every hour, with the button (3 flashes), with the *Check firmware update* button (web page or Home Assistant) or with *Install* on the Home Assistant `update` entity. If `version` is **newer** than the installed one, it downloads the file at `url`, checks the **signature**, installs it and restarts. The result is shown in the web page and in Home Assistant.

### 9.2 Publishing an update on Google Drive

1. Change the version in `include/Config.h` (`FW_VERSION`), e.g. from `45.4.0.0` to `45.5.0.0`.
2. Run the **Tests + Build** task.
3. Upload `AllInOneSonoff_v45.5.0.0.ota.bin.gz` to Google Drive → right click → *Share* → *General access: Anyone with the link* → *Copy link*.
4. Create `manifest.json` (example in [`doc/ota_manifest.json`](doc/ota_manifest.json)) with the new version and the link from step 3, upload it to Drive and share it the same way.
5. In the Sonoff settings, in *Firmware update url*, paste the **manifest link**. Drive share links are converted to direct download links automatically.

For the next updates, upload the files as a **new version of the same files** (right click → *File information* → *Manage versions* → *Upload new version*): the links stay the same and you don't have to touch the Sonoff.

The same firmware works in both modes, so one manifest is enough for all your devices. If you want to try an update on one device first, give it a different manifest. Any other web server works too (GitHub Releases, nginx on a NAS...), as long as it sends the `Content-Length` of the file.

### 9.3 Security: signed firmware

- Downloads over **https** are encrypted. The server certificate isn't verified (the Sonoff has no certificate list or reliable clock), but that's fine because the firmware itself is verified.
- Every OTA image is **signed** (RSA 2048 + SHA256) with `keys/private.key`. The Sonoff contains `keys/public.key` and **refuses** unsigned, damaged or differently signed firmware, and keeps running the current one.
- Only use the `.ota.bin.gz` file for OTA. The plain `.bin` is for serial flashing.
- **If you lose `keys/private.key`** you can only update the devices by serial. If you think someone has copied it, generate new keys (delete both files in `keys/` and build) and flash all the devices once by serial.
- The OTA image is compressed (gzip), and the Sonoff's bootloader decompresses it by itself. That way the update needs less space: the build prints the *OTA space margin*.

## 10. For developers

### Project structure

| Folder / file | Content |
|---|---|
| `src/`, `include/` | Firmware modules (one `.h` and one `.cpp` each) |
| `lib/AppLogic/` | Pure logic with no Arduino dependencies, unit tested |
| `lib/DigiOut`, `ReadInput`, `StoreStrings` | Libraries for outputs, debounced inputs, EEPROM strings |
| `test/` | Unity unit tests |
| `scripts/signing.py` | Key generation, signing and compression of the firmware (run by PlatformIO) |
| `keys/` | Signing keys (`private.key` is not committed) |
| `doc/` | Manual Home Assistant YAML, OTA manifest example, notes |
| `.vscode/tasks.json` | VS Code tasks |
| `.github/workflows/ci.yml` | Tests and build on GitHub at every push |

| Module | Content |
|---|---|
| Config | Version, pins, timings, sizes |
| Settings | Settings and state in EEPROM, device mode |
| Device | Relay, led, on/off, restart |
| WallSwitch | Wall switch (Switch mode) |
| LedStrip | WS2812B strip, colors, effects (LedStrip mode) |
| Button | Multifunction button |
| Network | WiFi, access point, mDNS |
| Mqtt | MQTT client, Home Assistant integration |
| WebUI | Web server and page (`HomePage.h`) |
| Ota | Updates from manifest with signature check |
| AppLogic: OtaLogic, ColorLogic, MqttLogic, SettingsLogic, HaDiscovery | Logic tested on PC |

### Unit tests

```bash
pio test -e native        # on the PC, a few seconds
pio test -e sonoff_sv     # on the Sonoff connected by serial
```

The tests cover: version comparison and Google Drive links, OTA manifest, hex colors and effects, MQTT commands and states (also the messages sent by Home Assistant), settings validation, and Home Assistant discovery (required fields, unique ids, topics, color modes, message size). The parts that need the hardware (WiFi, relay, EEPROM) must be tested on the device.

### Memory and flash

- 1 MB flash without filesystem (`eagle.flash.1m.ld`), EEPROM in the last sector: settings are kept across updates.
- EEPROM: settings (400 byte) + last state (26 byte). The mode is saved after the old fields, so devices coming from the old firmwares start in Switch mode with their settings.

## 11. Troubleshooting

| Problem | Solution |
|---|---|
| The flash fails / "Timed out waiting for packet header" | The Sonoff isn't in programming mode: repeat step 3 of [section 4](#4-first-flash-by-serial). Check that TX/RX are crossed and the adapter is at 3.3 V |
| I can't find the *SonoffAllInOne* network | Wait 30 s after power on. If the WiFi is configured but not reachable, the Sonoff keeps its access point active anyway (with the hostname as name) |
| The Sonoff doesn't appear in Home Assistant | Check *MQTT connected* in the web page. If "not connected": check broker IP, user and password. Make sure the MQTT integration is configured |
| Entities "unavailable" | The Sonoff isn't connected to the broker: check WiFi and power. With the Serial monitor you can see the MQTT error code |
| Colors swapped in the strip | Change the color order (`BRG`) in `src/LedStrip.cpp` |
| Update refused: "not signed or signed with another key" | You uploaded the `.bin` instead of the `.ota.bin.gz`, or you compiled with different keys |
| Update: "Manifest is not a valid json" | The Drive file isn't shared as "Anyone with the link" |
| Update: "Content-Length" error | The server doesn't send the file size: use another hosting (e.g. GitHub Releases) |
| Old retained MQTT messages | `mosquitto_sub -h <broker> -u <user> -P <password> -t '#' --remove-retained --retained-only` |
