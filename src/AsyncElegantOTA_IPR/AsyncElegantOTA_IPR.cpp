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
    .file{width:100%;box-sizing:border-box;margin:0 0 10px 0;}
    .btn{border:1px solid #4e6f8a;background:#2e506c;color:#f1f7fc;border-radius:7px;padding:8px 12px;cursor:pointer;}
    .btn:hover{background:#366283;}
    .status{margin-top:12px;min-height:18px;color:#9eaab5;font-size:13px;}
    .ok{color:#8be19f;}
    .err{color:#ff9b8f;}
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
    <div class="cards">
      <form class="card" id="fwForm" action="/update?mode=firmware" method="post" enctype="multipart/form-data">
        <h2>Firmware</h2>
        <p class="help">Choose firmware binary. File name must not contain "spiffs".</p>
        <input class="file" type="file" id="fwFile" name="update" required>
        <button class="btn" type="submit">Upload Firmware</button>
      </form>
      <form class="card" id="fsForm" action="/update?mode=filesystem" method="post" enctype="multipart/form-data">
        <h2>Filesystem</h2>
        <p class="help">Choose filesystem binary. File name must contain "spiffs".</p>
        <input class="file" type="file" id="fsFile" name="update" required>
        <button class="btn" type="submit">Upload Filesystem</button>
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

    function validate(mode, fileInput){
      if(!fileInput || !fileInput.files || !fileInput.files[0]){ return false; }
      var name = String(fileInput.files[0].name || "");
      var spiffs = hasSpiffs(name);
      if(mode === "firmware" && spiffs){
        setStatus("Firmware upload rejected: filename contains 'spiffs'.", false);
        fileInput.value = "";
        return false;
      }
      if(mode === "filesystem" && !spiffs){
        setStatus("Filesystem upload rejected: filename must contain 'spiffs'.", false);
        fileInput.value = "";
        return false;
      }
      setStatus("", true);
      return true;
    }

    var fwFile = document.getElementById("fwFile");
    var fsFile = document.getElementById("fsFile");
    var fwForm = document.getElementById("fwForm");
    var fsForm = document.getElementById("fsForm");

    fwFile.addEventListener("change", function(){ validate("firmware", fwFile); });
    fsFile.addEventListener("change", function(){ validate("filesystem", fsFile); });

    fwForm.addEventListener("submit", function(evt){
      if(!validate("firmware", fwFile)){
        evt.preventDefault();
        return;
      }
      setStatus("Uploading firmware...", true);
    });

    fsForm.addEventListener("submit", function(evt){
      if(!validate("filesystem", fsFile)){
        evt.preventDefault();
        return;
      }
      setStatus("Uploading filesystem...", true);
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
