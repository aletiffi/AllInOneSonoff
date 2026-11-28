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
            <label for="Sonoff_Model">Sonoff model (free pin: GPIO14 on the R1, the IO2 pad on the R2)</label>
          </p>
          <p>
            <select id="Sonoff_Model">
              <option value="R1">Basic R1 (GPIO14)</option>
              <option value="R2">Basic R2 (GPIO2)</option>
            </select>
          </p>
          <p>
            <label for="Hostname">Device name (name of the access point, in Home Assistant and in the MQTT topics)</label>
          </p>
          <p>
            <input autocomplete="off" value="" id="Hostname" placeholder="SonoffAllInOne" maxlength="20">
          </p>
          <p>
            <label for="AP_Password">Access point password (8-20 characters)</label>
          </p>
          <p>
            <input type="password" autocomplete="new-password" value="" id="AP_Password" placeholder="Access point password" maxlength="20">
          </p>
          <p class="info" id="ap_status"></p>
          <p>
            <label for="Ssid">Home WiFi name (Ssid)</label>
          </p>
          <p>
            <input autocomplete="off" value="" id="Ssid" placeholder="Wifi Net Name" maxlength="20">
          </p>
          <p>
            <label for="Password">Home WiFi password</label>
          </p>
          <p>
            <input type="password" autocomplete="new-password" value="" id="Password" placeholder="Wifi Password" maxlength="20">
          </p>
          <p>
            <label for="MQTT_Sub">MQTT command topic (optional, default sonoff/&lt;hostname&gt;/set)</label>
          </p>
          <p>
            <input autocomplete="off" value="" id="MQTT_Sub" placeholder="cmnd/MyDevice" maxlength="20">
          </p>
          <p>
            <label for="MQTT_Pub">MQTT state topic (optional, default sonoff/&lt;hostname&gt;/state)</label>
          </p>
          <p>
            <input autocomplete="off" value="" id="MQTT_Pub" placeholder="stat/MyDevice" maxlength="20">
          </p>
          <p>
            <label for="MQTT_Server">MQTT Server (ip or ip:port, e.g. Home Assistant ip)</label>
          </p>
          <p>
            <input autocomplete="off" value="" id="MQTT_Server" placeholder="192.168.1.100" maxlength="20">
          </p>
          <p>
            <label for="MQTT_User">MQTT User (optional)</label>
          </p>
          <p>
            <input autocomplete="off" value="" id="MQTT_User" placeholder="MQTT User" maxlength="20">
          </p>
          <p>
            <label for="MQTT_Password">MQTT Password (optional)</label>
          </p>
          <p>
            <input type="password" autocomplete="new-password" value="" id="MQTT_Password" placeholder="MQTT Password" maxlength="20">
          </p>
          <p>
            <label for="OTA_URL">Firmware update url (empty = latest GitHub release, "off" = disabled)</label>
          </p>
          <p>
            <input autocomplete="off" value="" id="OTA_URL" placeholder="latest GitHub release (default)" maxlength="150">
          </p>
          <br>
          <p>
            <button type="button" id="savebtn" style="background-color: cornflowerblue;" onclick="save()">Save Settings</button>
          </p>
          <p class="info" id="save_status"></p>
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
            <button id="cleanbtn" style="background-color: rgb(189, 0, 0);" onclick="eepromConfirm()">Clean EEPROM</button>
          </p>
          <p class="info" id="clean_status"></p>
          <hr>
        </div>
      </div>
    </body>

    <script>
      var currentMode = "Switch";

      window.addEventListener("load", startup, false);

      function startup() {
        document.getElementById("colorWell").addEventListener("change", sendColor, false);
        loadInfo(true);
      }

      // Password fields: the saved password is never sent by the device, only whether it is set
      function setPasswordField(id, isSet, placeholder, canRemove) {
        var el = document.getElementById(id);
        el.value = "";
        el.placeholder = isSet ? "saved - empty = keep" + (canRemove ? ", \"-\" = remove" : "") : placeholder;
      }

      // Saved values in the settings fields (at startup and after saving, not to overwrite what is being typed)
      // Values shown in the fields: an emptied field is sent as "-" (delete), because for the device
      // an empty value means "keep the saved one"
      var textIds = ["Hostname", "Ssid", "MQTT_Sub", "MQTT_Pub", "MQTT_Server", "MQTT_User", "OTA_URL"];
      var savedSettings = {};

      function fillSettings(s) {
        savedSettings = s;
        document.getElementById("Sonoff_Model").value = s.Sonoff_Model || "R1";
        for (var i = 0; i < textIds.length; i++) {
          document.getElementById(textIds[i]).value = s[textIds[i]] || "";
        }
        setPasswordField("Password", s.Password_set, "Wifi Password", false);
        setPasswordField("MQTT_Password", s.MQTT_Password_set, "MQTT Password", true);
        setPasswordField("AP_Password", s.AP_Password_set, "Access point password", true);
      }

      function loadInfo(withSettings) {
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function () {
          if (this.readyState == 4 && this.status == 200) {
            var info = JSON.parse(xhr.responseText);
            currentMode = info.mode;
            document.getElementById("Device_Mode").value = info.mode;
            document.getElementById("title").innerText = info.hostname;
            document.getElementById("info").innerText = "Sonoff " + info.model + " - " + info.mode + " mode - state " + info.state + " - fw " + info.version;
            document.getElementById("mqtt_info").innerText = "MQTT " + (info.mqtt_connected ? "connected" : "not connected") +
              " - id " + info.unique_id + " - command topic " + info.mqtt_command + " - state topic " + info.mqtt_state;
            document.getElementById("ota_status").innerText = "Update: " + info.ota_status +
              (info.ota_signed ? " (signed firmware only)" : " (WARNING: signature check disabled)");
            if (withSettings) {
              fillSettings(info.settings);
            }
            document.getElementById("ap_status").innerText = info.ap_protected ?
              "Access point protected by password" :
              "WARNING: access point without password, anyone nearby can change the settings";
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

      // No alert()/confirm(): browsers can block them for a site ("don't allow more dialogs") and the
      // login pages of phones on networks without internet often ignore them. The page shows the
      // messages and asks to press the button again to confirm.
      var pendingConfirm = {};

      function showStatus(id, msg, isError) {
        var el = document.getElementById(id);
        el.innerText = msg;
        el.style.color = isError ? "rgb(189, 0, 0)" : "";
      }

      // True on the second press within 10 s, after showing the message
      function confirmPress(btnId, statusId, msg) {
        var btn = document.getElementById(btnId);
        if (pendingConfirm[btnId]) {
          clearTimeout(pendingConfirm[btnId].timer);
          btn.innerText = pendingConfirm[btnId].label;
          delete pendingConfirm[btnId];
          return true;
        }
        var label = btn.innerText;
        btn.innerText = "Press again to confirm";
        showStatus(statusId, msg, true);
        pendingConfirm[btnId] = {label: label, timer: setTimeout(function () {
          btn.innerText = label;
          showStatus(statusId, "", false);
          delete pendingConfirm[btnId];
        }, 10000)};
        return false;
      }

      function save() {
        var ids = ["Hostname", "Ssid", "Password", "MQTT_Sub", "MQTT_Pub", "MQTT_Server",
                   "MQTT_User", "MQTT_Password", "OTA_URL", "Device_Mode", "AP_Password", "Sonoff_Model"];
        var data = {};
        for (var i = 0; i < ids.length; i++) {
          data[ids[i]] = document.getElementById(ids[i]).value;
        }
        for (var j = 0; j < textIds.length; j++) {
          var id = textIds[j];
          if (data[id].trim() == "" && savedSettings[id]) {
            data[id] = "-";
          }
        }
        var ap = data.AP_Password.trim();
        if (ap != "" && ap != "-" && (ap.length < 8 || ap.length > 20)) {
          showStatus("save_status", "Access point password: 8-20 characters", true);
          return;
        }
        var warnings = [];
        if (ap != "" && ap != "-") {
          warnings.push("The access point will be protected by the new password: write it down, it's needed to change the settings without the home WiFi.");
        }
        if (data.Device_Mode != currentMode) {
          warnings.push("The device mode will change to " + data.Device_Mode + " and the device will restart.");
        }
        if (data.Sonoff_Model != (savedSettings.Sonoff_Model || "R1")) {
          warnings.push("The model will change to " + data.Sonoff_Model + " and the device will restart.");
        }
        if (warnings.length > 0 && !confirmPress("savebtn", "save_status", warnings.join(" "))) {
          return;
        }
        showStatus("save_status", "Saving...", false);
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function () {
          if (this.readyState == 4) {
            var ok = xhr.status == 200;
            showStatus("save_status", xhr.responseText || "No response from the device", !ok);
            if (ok) {
              loadInfo(true);
            }
          }
        };
        xhr.timeout = 15000;
        xhr.open("POST", "/settings", true);
        xhr.send(JSON.stringify(data));
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
        if (confirmPress("cleanbtn", "clean_status", "All the settings will be deleted.")) {
          window.location.href = '/clean';
        }
      }
    </script>

  </html>
)=====";
