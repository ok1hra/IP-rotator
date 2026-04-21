#include "AsyncElegantOTA_IPR.h"
#if defined(ESP32)
#include <SPIFFS.h>
#endif

AsyncElegantOtaIprClass AsyncElegantOTA_IPR;

static const char ELEGANT_HTML_CUSTOM[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>OTA Update</title>
  <style>
    body{margin:0;padding:28px 16px;background:#101316;color:#d7dde2;font-family:Arial,Helvetica,sans-serif;}
    .wrap{max-width:700px;margin:0 auto;}
    .warn{max-width:470px;margin:0 auto 12px auto;border:1px solid #8d6b31;background:#2c2417;color:#f4d9a7;border-radius:8px;padding:10px 12px;line-height:1.45;font-size:14px;}
    .logo{text-align:center;font-size:26px;letter-spacing:.04em;font-weight:700;color:#f2f6fa;margin:0 0 18px 0;}
    .cards{display:grid;grid-template-columns:1fr;gap:12px;}
    .card{border:1px solid #34404a;background:#1a2026;border-radius:10px;padding:14px;}
    .card h2{margin:0 0 10px 0;font-size:18px;color:#f2f6fa;}
    .help{margin:0 0 10px 0;color:#9eaab5;font-size:13px;}
    .row{display:flex;align-items:center;gap:10px;flex-wrap:wrap;}
    .file{flex:1 1 340px;min-width:220px;box-sizing:border-box;margin:0;}
    .btn{border:1px solid #4e6f8a;background:#2e506c;color:#f1f7fc;border-radius:7px;padding:8px 12px;cursor:pointer;}
    .btn:hover{background:#366283;}
    .btn[disabled]{opacity:.6;cursor:default;}
    .progress{margin-top:12px;height:10px;border-radius:999px;background:#101316;border:1px solid #34404a;overflow:hidden;display:none;}
    .progress.show{display:block;}
    .progress-bar{height:100%;width:0;background:linear-gradient(90deg,#5ab0ff,#8be19f);transition:width .2s ease;}
    .card-status{margin-top:10px;min-height:18px;color:#9eaab5;font-size:13px;}
    .card-status.ok{color:#8be19f;}
    .card-status.err{color:#ff9b8f;}
    .check{display:inline-flex;align-items:center;justify-content:center;width:24px;height:24px;border-radius:999px;background:#163522;color:#8be19f;font-size:18px;font-weight:700;border:1px solid #2f6e46;}
    .backend-status{display:flex;align-items:center;gap:8px;width:max-content;max-width:100%;margin:0 auto 14px auto;padding:7px 12px;border-radius:999px;border:1px solid #34404a;background:#171d22;color:#cfd7dd;font-size:13px;}
    .backend-dot{width:9px;height:9px;border-radius:999px;background:#7f8b96;flex:0 0 auto;}
    .backend-status.online .backend-dot{background:#8be19f;box-shadow:0 0 0 3px rgba(139,225,159,.14);}
    .backend-status.waiting .backend-dot{background:#f4d9a7;box-shadow:0 0 0 3px rgba(244,217,167,.12);}
    .backend-status.err .backend-dot{background:#ff9b8f;box-shadow:0 0 0 3px rgba(255,155,143,.12);}
    .status{margin-top:12px;min-height:18px;color:#9eaab5;font-size:13px;}
    .ok{color:#8be19f;}
    .err{color:#ff9b8f;}
    @media (max-width: 560px){
      .row{align-items:stretch;}
      .file{min-width:0;width:100%;}
      .btn{width:100%;}
    }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="warn">
      <b>Warning: Follow this update order to avoid issues:</b><br>
      1. Upload firmware.bin as Firmware.<br>
      2. Upload spiffs.bin as Filesystem.
    </div>
    <div class="logo">ElegantOTA</div>
    <div class="backend-status online" id="backendStatus">
      <span class="backend-dot"></span>
      <span id="backendStatusText">Backend online</span>
    </div>
    <div class="cards">
      <form class="card" id="fwForm" action="/update?mode=firmware" method="post" enctype="multipart/form-data">
        <h2>Firmware</h2>
        <p class="help">Choose firmware binary. File name <b>must not contain</b> "spiffs".</p>
        <div class="row">
          <input class="file" type="file" id="fwFile" name="update" required>
          <button class="btn" id="fwSubmit" type="submit">Upload Firmware</button>
        </div>
        <div class="progress" id="fwProgress"><div class="progress-bar" id="fwProgressBar"></div></div>
        <div class="card-status" id="fwStatus"></div>
      </form>
      <form class="card" id="fsForm" action="/update?mode=filesystem" method="post" enctype="multipart/form-data">
        <h2>Filesystem</h2>
        <p class="help">Choose filesystem binary. File name <b>must contain</b> "spiffs".</p>
        <div class="row">
          <input class="file" type="file" id="fsFile" name="update" required>
          <button class="btn" id="fsSubmit" type="submit">Upload Filesystem</button>
        </div>
        <div class="progress" id="fsProgress"><div class="progress-bar" id="fsProgressBar"></div></div>
        <div class="card-status" id="fsStatus"></div>
      </form>
    </div>
    <div class="status" id="status"></div>
  </div>

  <script>
    function hasSpiffs(name){ return /spiffs/i.test(String(name || "")); }
    function setStatus(msg, ok){
      var el = document.getElementById("status");
      el.className = "status " + (ok ? "ok" : "err");
      el.textContent = msg || "";
    }

    function setBackendStatus(state, text){
      var el = document.getElementById("backendStatus");
      var textEl = document.getElementById("backendStatusText");
      el.className = "backend-status " + state;
      textEl.textContent = text || "";
    }

    function setCardStatus(mode, msg, ok, allowHtml){
      var el = document.getElementById(mode === "firmware" ? "fwStatus" : "fsStatus");
      el.className = "card-status" + (msg ? (" " + (ok ? "ok" : "err")) : "");
      if(allowHtml){
        el.innerHTML = msg || "";
      }else{
        el.textContent = msg || "";
      }
    }

    function setProgress(mode, percent, visible){
      var progress = document.getElementById(mode === "firmware" ? "fwProgress" : "fsProgress");
      var bar = document.getElementById(mode === "firmware" ? "fwProgressBar" : "fsProgressBar");
      if(visible){
        progress.className = "progress show";
      }else{
        progress.className = "progress";
      }
      bar.style.width = Math.max(0, Math.min(100, percent || 0)) + "%";
    }

    function setBusy(isBusy, activeMode){
      var inputs = document.querySelectorAll("input, button");
      var fwForm = document.getElementById("fwForm");
      var fsForm = document.getElementById("fsForm");
      fwForm.style.opacity = (!isBusy || activeMode === "firmware") ? "1" : "0.72";
      fsForm.style.opacity = (!isBusy || activeMode === "filesystem") ? "1" : "0.72";
      for(var i = 0; i < inputs.length; i++){
        inputs[i].disabled = !!isBusy;
      }
    }

    function markReadyForNextStep(mode){
      setBusy(false);
      setBackendStatus("online", "Backend online");
      setStatus("", true);
      setCardStatus(mode, "<span class=\"check\">&#10003;</span>", true, true);
    }

    function waitUntilDeviceReturns(mode, startedAt){
      window.setTimeout(function(){
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/update/identity", true);
        xhr.timeout = 2000;
        xhr.onload = function(){
          if(xhr.status >= 200 && xhr.status < 300){
            markReadyForNextStep(mode);
          }else{
            retry();
          }
        };
        xhr.onerror = retry;
        xhr.ontimeout = retry;
        xhr.send();
      }, 2000);

      function retry(){
        if(Date.now() - startedAt > 45000){
          setBusy(false);
          setBackendStatus("err", "Backend status unknown");
          setStatus("Upload completed, but backend reconnection was not confirmed yet.", true);
          return;
        }
        waitUntilDeviceReturns(mode, startedAt);
      }
    }

    function submitUpdate(mode, fileInput){
      if(!validate(mode, fileInput)){
        return;
      }

      var file = fileInput.files[0];
      var formData = new FormData();
      formData.append("update", file, file.name);
      var xhr = new XMLHttpRequest();
      var uploadFinished = false;
      setBusy(true, mode);
      setBackendStatus("waiting", "Backend busy");
      setProgress(mode, 0, true);
      setCardStatus(mode, "Uploading...", true);
      setStatus("Uploading " + mode + "...", true);

      xhr.upload.addEventListener("progress", function(evt){
        if(evt.lengthComputable){
          setProgress(mode, (evt.loaded / evt.total) * 100, true);
        }
      });

      xhr.upload.addEventListener("load", function(){
        uploadFinished = true;
      });

      xhr.onreadystatechange = function(){
        if(xhr.readyState !== 4){
          return;
        }
        if(xhr.status >= 200 && xhr.status < 300){
          setProgress(mode, 100, true);
          setCardStatus(mode, "Upload complete. Device is restarting...", true);
          setBackendStatus("waiting", "Backend restarting...");
          setStatus("Upload complete. Waiting for device restart...", true);
          fileInput.value = "";
          waitUntilDeviceReturns(mode, Date.now());
          return;
        }
        if(xhr.status === 0 && uploadFinished){
          setProgress(mode, 100, true);
          setCardStatus(mode, "Upload sent. Waiting for device restart...", true);
          setBackendStatus("waiting", "Backend restarting...");
          setStatus("Upload sent. Waiting for device restart...", true);
          fileInput.value = "";
          waitUntilDeviceReturns(mode, Date.now());
          return;
        }
        setBusy(false);
        setBackendStatus("online", "Backend online");
        setProgress(mode, 0, false);
        var message = xhr.responseText || ("Upload failed (HTTP " + xhr.status + ").");
        setCardStatus(mode, message, false);
        setStatus(message, false);
      };

      xhr.onerror = function(){
        setBusy(false);
        setBackendStatus("online", "Backend online");
        setProgress(mode, 0, false);
        setCardStatus(mode, "Upload failed due to network error.", false);
        setStatus("Upload failed due to network error.", false);
      };

      xhr.open("POST", "/update?mode=" + encodeURIComponent(mode), true);
      try{
        xhr.send(formData);
      }catch(err){
        setBusy(false);
        setBackendStatus("online", "Backend online");
        setProgress(mode, 0, false);
        var msg = (err && err.message) ? err.message : "Upload failed.";
        setCardStatus(mode, msg, false);
        setStatus(msg, false);
      }
    }

    function validate(mode, fileInput){
      if(!fileInput || !fileInput.files || !fileInput.files[0]){ return false; }
      var name = String(fileInput.files[0].name || "");
      var spiffs = hasSpiffs(name);
      if(mode === "firmware" && spiffs){
        setStatus("Firmware upload rejected: filename contains 'spiffs'.", false);
        setCardStatus(mode, "Firmware upload rejected: filename contains 'spiffs'.", false);
        fileInput.value = "";
        return false;
      }
      if(mode === "filesystem" && !spiffs){
        setStatus("Filesystem upload rejected: filename must contain 'spiffs'.", false);
        setCardStatus(mode, "Filesystem upload rejected: filename must contain 'spiffs'.", false);
        fileInput.value = "";
        return false;
      }
      setStatus("", true);
      setCardStatus(mode, "", true);
      return true;
    }

    var fwFile = document.getElementById("fwFile");
    var fsFile = document.getElementById("fsFile");
    var fwForm = document.getElementById("fwForm");
    var fsForm = document.getElementById("fsForm");

    fwFile.addEventListener("change", function(){ validate("firmware", fwFile); });
    fsFile.addEventListener("change", function(){ validate("filesystem", fsFile); });

    fwForm.addEventListener("submit", function(evt){
      evt.preventDefault();
      submitUpdate("firmware", fwFile);
    });

    fsForm.addEventListener("submit", function(evt){
      evt.preventDefault();
      submitUpdate("filesystem", fsFile);
    });
  </script>
</body>
</html>
)rawliteral";

static bool elegantFilenameHasSpiffs(const String& name){
    String lower = name;
    lower.toLowerCase();
    return lower.indexOf("spiffs") >= 0;
}

void AsyncElegantOtaIprClass::setID(const char* id){
    _id = id;
}

void AsyncElegantOtaIprClass::begin(AsyncWebServer *server, const char* username, const char* password){
    _server = server;

    if(strlen(username) > 0){
        _authRequired = true;
        _username = username;
        _password = password;
    }else{
        _authRequired = false;
        _username = "";
        _password = "";
    }

    _server->on("/update/identity", HTTP_GET, [&](AsyncWebServerRequest *request){
        if(_authRequired){
            if(!request->authenticate(_username.c_str(), _password.c_str())){
                return request->requestAuthentication();
            }
        }
        #if defined(ESP8266)
            request->send(200, "application/json", "{\"id\": \""+_id+"\", \"hardware\": \"ESP8266\"}");
        #elif defined(ESP32)
            request->send(200, "application/json", "{\"id\": \""+_id+"\", \"hardware\": \"ESP32\"}");
        #endif
    });

    _server->on("/update", HTTP_GET, [&](AsyncWebServerRequest *request){
        if(_authRequired){
            if(!request->authenticate(_username.c_str(), _password.c_str())){
                return request->requestAuthentication();
            }
        }
        request->send(200, "text/html", ELEGANT_HTML_CUSTOM);
    });

    _server->on("/update", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if(_authRequired){
            if(!request->authenticate(_username.c_str(), _password.c_str())){
                return request->requestAuthentication();
            }
        }
        AsyncWebServerResponse *response = request->beginResponse((Update.hasError())?500:200, "text/plain", (Update.hasError())?"FAIL":"OK");
        response->addHeader("Connection", "close");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        restart();
    }, [&](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        if(_authRequired){
            if(!request->authenticate(_username.c_str(), _password.c_str())){
                return request->requestAuthentication();
            }
        }

        if (!index) {
            String mode = "";
            if(request->hasParam("mode")){
                mode = request->getParam("mode")->value();
            }
            mode.toLowerCase();

            if(mode != "firmware" && mode != "filesystem") {
                return request->send(400, "text/plain", "Select upload type first.");
            }

            bool hasSpiffs = elegantFilenameHasSpiffs(filename);
            if(mode == "firmware" && hasSpiffs){
                return request->send(400, "text/plain", "Firmware upload rejected: filename contains 'spiffs'.");
            }
            if(mode == "filesystem" && !hasSpiffs){
                return request->send(400, "text/plain", "Filesystem upload rejected: filename must contain 'spiffs'.");
            }

            if(request->hasParam("MD5", true)){
                if(!Update.setMD5(request->getParam("MD5", true)->value().c_str())) {
                    return request->send(400, "text/plain", "MD5 parameter invalid");
                }
            }

            #if defined(ESP8266)
                int cmd = (mode == "filesystem") ? U_FS : U_FLASH;
                Update.runAsync(true);
                size_t fsSize = ((size_t) &_FS_end - (size_t) &_FS_start);
                uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
                if (!Update.begin((cmd == U_FS)?fsSize:maxSketchSpace, cmd)){
            #elif defined(ESP32)
                int cmd = (mode == "filesystem") ? U_SPIFFS : U_FLASH;
                if (cmd == U_SPIFFS) {
                    SPIFFS.end();
                }
                if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
            #endif
                Update.printError(Serial);
                return request->send(400, "text/plain", String("OTA could not begin: ") + Update.errorString());
            }
        }

        if(len){
            if (Update.write(data, len) != len) {
                return request->send(400, "text/plain", String("OTA could not write: ") + Update.errorString());
            }
        }

        if (final) {
            if (!Update.end(true)) {
                Update.printError(Serial);
                return request->send(400, "text/plain", String("Could not end OTA: ") + Update.errorString());
            }
        }
    });
}

void AsyncElegantOtaIprClass::loop() {
}

void AsyncElegantOtaIprClass::restart() {
    yield();
    delay(1000);
    yield();
    ESP.restart();
}

String AsyncElegantOtaIprClass::getID(){
    String id = "";
    #if defined(ESP8266)
        id = String(ESP.getChipId());
    #elif defined(ESP32)
        id = String((uint32_t)ESP.getEfuseMac(), HEX);
    #endif
    id.toUpperCase();
    return id;
}
