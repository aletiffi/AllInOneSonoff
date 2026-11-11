const char webPage[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html lang="en">

    <head>
      <meta charset="utf-8">
      <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=yes">
      <title>Home Page</title>
      <style>
        body {
          background-color: #b7b7ee;
          text-align: center;
          font-family: verdana;
        }
        div {
          padding: 10px;
        }
        input {
          border-radius: 5px;
          width: 100%;
          height: 40px;
          box-sizing: border-box;
          text-align: center;
        }
        select {
          border-radius: 5px;
          padding: 10px;
          width: 100%;
          text-align: center;
          text-align-last:center;
        }
        button {
          border-radius: 5px;
          width: 100%;
          height: 40px;
          color:black;
        }
        .container {
          border-radius: 5px;
          border: 2px solid #000000;
          width: 90%;
          max-width: 450px;
          background-color: rgb(62, 131, 131);
          margin: auto;
        }
        .actions-container {
          width: 90%;
          max-width: 350px;
          margin: auto;
        }
        .settings-container {
          width: 90%;
          max-width: 350px;
          margin: auto;
        }
        .hidden {
          display: none;
        }
        .info {
          font-size: small;
        }
      </style>
    </head>
    <body>
      <div class="container">
        <div class="actions-container">
          <h1 id="title">Home Page</h1>
          <p class="info" id="info"></p>
          <p class="info" id="mqtt_info"></p>
          <hr>
          <p>
            <a href="/on"><button style="background-color: rgb(14, 121, 14);">ON</button></a>
          </p>
          <p>
            <a href="/off"><button style="background-color: rgb(209, 16, 16);">OFF</button></a>
          </p>
          <hr>
          <div id="ledstrip" class="hidden">
            <h2>Color picker</h2>
            <input class="input" style="padding: 0%; height: 10ch;" type="color" id="colorWell" name="colorWell" value="#3289a8">
            <hr>
            <h2>Brightness</h2>
            <input type="range" value="85" id="Brightness" min="0" max="255" onchange="sendBrightness()">
            <hr>
            <h2>Effect Selector</h2>
            <select id="effect" onchange="sendEffect()">
              <option value="Solid">Solid</option>
              <option value="Random">Random</option>
            </select>
            <hr>
          </div>
        </div>
        <div class="settings-container">
          <h2>Settings</h2>
          <p>
            <label for="Device_Mode">Device mode</label>
          </p>
          <p>
            <select id="Device_Mode">
              <option value="Switch">Switch (relay + wall switch)</option>
              <option value="LedStrip">LedStrip (WS2812B)</option>
            </select>
          </p>
          <p>
            <label for="Hostname">Hostname</label>
          </p>
          <p>
            <input value="" id="Hostname" placeholder="DeviceName" maxlength="20">
          </p>
          <p>
            <label for="Ssid">Ssid</label>
          </p>
          <p>
            <input value="" id="Ssid" placeholder="Wifi Net Name" maxlength="20">
          </p>
          <p>
            <label for="Password">Password</label>
          </p>
          <p>
            <input type="password" value="" id="Password" placeholder="Wifi Password" maxlength="20">
          </p>
          <p>
            <label for="MQTT_Sub">MQTT command topic (optional, default sonoff/&lt;hostname&gt;/set)</label>
          </p>
          <p>
            <input value="" id="MQTT_Sub" placeholder="cmnd/MyDevice" maxlength="20">
          </p>
          <p>
            <label for="MQTT_Pub">MQTT state topic (optional, default sonoff/&lt;hostname&gt;/state)</label>
          </p>
          <p>
            <input value="" id="MQTT_Pub" placeholder="stat/MyDevice" maxlength="20">
          </p>
          <p>
            <label for="MQTT_Server">MQTT Server (ip or ip:port, e.g. Home Assistant ip)</label>
          </p>
          <p>
            <input value="" id="MQTT_Server" placeholder="192.168.1.100" maxlength="20">
          </p>
          <p>
            <label for="MQTT_User">MQTT User (optional)</label>
          </p>
          <p>
            <input value="" id="MQTT_User" placeholder="MQTT User" maxlength="20">
          </p>
          <p>
            <label for="MQTT_Password">MQTT Password (optional)</label>
          </p>
          <p>
            <input type="password" value="" id="MQTT_Password" placeholder="MQTT Password" maxlength="20">
          </p>
          <p>
            <label for="OTA_URL">Firmware update url (empty = latest GitHub release, "off" = disabled, "-" = back to default)</label>
          </p>
          <p>
            <input value="" id="OTA_URL" placeholder="latest GitHub release" maxlength="150">
          </p>
          <br>
          <p>
            <button type="button" id="savebtn" style="background-color: cornflowerblue;" onclick="save()">Save Settings</button>
          </p>
          <hr>
          <p>
            <button type="button" style="background-color: rgb(120, 190, 120);" onclick="checkUpdate()">Check firmware update</button>
          </p>
          <p class="info" id="ota_status"></p>
          <hr>
          <p>
            <a href="/restart"><button style="background-color: rgb(216, 196, 20);">Restart Device</button></a>
          </p>
          <hr>
          <p>
            <button style="background-color: rgb(189, 0, 0);" onclick="eepromConfirm()">Clean EEPROM</button>
          </p>
          <hr>
        </div>
      </div>
    </body>

    <script>
      var currentMode = "Switch";

      window.addEventListener("load", startup, false);

      function startup() {
        document.getElementById("colorWell").addEventListener("change", sendColor, false);
        loadInfo();
      }

      function loadInfo() {
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function () {
          if (this.readyState == 4 && this.status == 200) {
            var info = JSON.parse(xhr.responseText);
            currentMode = info.mode;
            document.getElementById("Device_Mode").value = info.mode;
            document.getElementById("title").innerText = info.hostname;
            document.getElementById("info").innerText = info.mode + " mode - state " + info.state + " - fw " + info.version;
            document.getElementById("mqtt_info").innerText = "MQTT " + (info.mqtt_connected ? "connected" : "not connected") +
              " - id " + info.unique_id + " - command topic " + info.mqtt_command + " - state topic " + info.mqtt_state;
            document.getElementById("ota_status").innerText = "Update: " + info.ota_status +
              (info.ota_signed ? " (signed firmware only)" : " (WARNING: signature check disabled)");
            document.getElementById("OTA_URL").placeholder = info.ota_url == "" ? "off (updates disabled)" :
              (info.ota_url_default ? "latest GitHub release (default)" : info.ota_url);
            if (info.mode == "LedStrip") {
              document.getElementById("ledstrip").classList.remove("hidden");
              document.getElementById("colorWell").value = info.color;
              document.getElementById("Brightness").value = info.brightness;
              document.getElementById("effect").value = info.effect;
            } else {
              document.getElementById("ledstrip").classList.add("hidden");
            }
          }
        };
        xhr.open("GET", "/info", true);
        xhr.send();
      }

      function post(url, data) {
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function () {
          if (this.readyState == 4 && this.status == 200) {
            if (xhr.responseText != null) {
              console.log(xhr.responseText);
            }
          }
        };
        xhr.open("POST", url, true);
        xhr.send(JSON.stringify(data));
      }

      function sendBrightness() {
        post("/brigthness", {Brightness:document.getElementById("Brightness").value});
      }

      function sendEffect() {
        post("/effect", {effect:document.getElementById("effect").value});
      }

      function sendColor(event) {
        post("/color", {color:event.target.value});
      }

      function save() {
        var ids = ["Hostname", "Ssid", "Password", "MQTT_Sub", "MQTT_Pub", "MQTT_Server",
                   "MQTT_User", "MQTT_Password", "OTA_URL", "Device_Mode"];
        var data = {};
        for (var i = 0; i < ids.length; i++) {
          data[ids[i]] = document.getElementById(ids[i]).value;
        }
        if (data.Device_Mode != currentMode &&
            !confirm("The device mode will change to " + data.Device_Mode + " and the device will restart. Continue?")) {
          return;
        }
        post("/settings", data);
      };

      function checkUpdate() {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/update", true);
        xhr.send();
        document.getElementById("ota_status").innerText = "Update: Checking...";
        // The device can be busy for a while (or restart after the update)
        setTimeout(loadInfo, 15000);
      }

      function eepromConfirm() {
        if (confirm("Are you sure to clean all EEPROM?")) {
          window.location.href = '/clean';
        }
      }
    </script>

  </html>
)=====";
