**All In One Sonoff (Switch / LedStrip)**
=======================
One firmware for the Sonoff r1, two uses selectable from the web interface:

- **Switch**: the Sonoff drives a relay (light bulb, wall socket...) from the wall switch, the integrated button, MQTT or the home page.
- **LedStrip**: the Sonoff powers a ws2812b led strip through the relay and drives it on GPIO14 (color, brightness, effects).

The Sonoff can communicate via MQTT protocol with a broker like mosquitto, and in stand-alone mode by accessing the device homepage. Once connected to your wifi network it will be accessible in localhost or at 192.168.4.1 by connecting to its wifi network.

This project merges the old [SwitchSonoff](https://github.com/aletiffi/SwitchSonoff) and [LedstripSonoff](https://github.com/aletiffi/LedstripSonoff) firmwares.

<p align="center">
  <img src="img/HomePage.PNG" alt="Home page"/>
</p>

**Wirings**
-----------------------------------------
GPIO14 has a different function depending on the selected mode:

| Mode     | GPIO14                        | Relay                         |
|----------|-------------------------------|-------------------------------|
| Switch   | Wall switch input (pull-up)   | Load (light bulb, socket...)  |
| LedStrip | ws2812b data line             | Led strip power supply        |

<p align="center">
  <img src="img/Schema.png" alt="Switch wiring"/>
</p>

In LedStrip mode use a power supply suitably sized for the load required by the led strip. The number of leds is set by `NUM_LEDS` in `include/GlobalVar.h` (default 33).

**Setup**
-----------------------------------------
Once the firmware has been loaded, the EEPROM is deleted at the first start, and a default name is assigned to the device.

Once connected to the wifi network named **SonoffAllInOne** (default name) open the browser at 192.168.4.1. Choose the **Device mode**, rename the device then enter the name of the wifi network and the password to which it must connect. The other settings can be entered later. By pressing the `Save Settings` button at the bottom of the page the Sonoff saves the settings, then tries to connect to your network. When the device mode changes the Sonoff restarts.

A device updated from an old firmware keeps its settings and starts in Switch mode.

<p align="center">
  <img src="img/Settings.PNG" alt="Settings"/>
</p>

The integrated led indicates successful connection to the wifi network.
If the LED does not light up, the network signal may be too weak, to restart the connection process press and hold the button until the LED flashes 2 times.

Now the device is connected to the local network, and it is possible to enter the MQTT parameters through the web page reachable in localhost, by entering `HostName.local` in the browser.

If the OTA Drive ProductKey is present, it is possible to manage the updating of future firmware with this platform without having to flash the device by hand.

**MQTT**
-----------------------------------------
Commands are JSON messages on the subscribe topic, the state is published on the publish topic (Home Assistant `json` schema, see `doc/Hassio configuration example.yaml`).

- Switch: `{"state": "ON"}` / `{"state": "OFF"}`
- LedStrip: `{"state": "ON", "brightness": 120, "color": {"r": 255, "g": 0, "b": 80}, "effect": "Random"}` (effects: `Solid`, `Random`)

**Integrated button functionality**
-----------------------------------------
While holding the Sonoff button, the LED will flash once per second. Based on the number of flashes reached at release, one of the following functions will be activated.

- 1:  Toggle
- 2:  Connection check
- 3:  Check FW update
- 4:  Show ip address (number of flashes equal to the last IP number)
- 5:  Wifi signal power
- 6:  Load settings (for debug purpose)
- 7:  Save settings (for debug purpose)
- 8:  EEPROM clean
- 9:  Read all EEPROM (for debug purpose)
- 10: Sonoff restart

**Build**
-----------------------------------------
The project is built with PlatformIO (`pio run`, `pio run -t upload`). The DigiOut, ReadInput and StoreStrings libraries are included in the `lib` folder; ArduinoJson, PubSubClient and FastLED are downloaded automatically.
Within this repository there is the .bin file to be installed in the Sonoff.
