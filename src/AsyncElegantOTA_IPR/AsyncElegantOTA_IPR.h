#ifndef AsyncElegantOTA_IPR_h
#define AsyncElegantOTA_IPR_h

#include "Arduino.h"
#include "stdlib_noniso.h"

#if defined(ESP8266)
    #include "ESP8266WiFi.h"
    #include "ESPAsyncTCP.h"
    #include "flash_hal.h"
    #include "FS.h"
#elif defined(ESP32)
    #include "WiFi.h"
    #include "AsyncTCP.h"
    #include "Update.h"
    #include "esp_int_wdt.h"
    #include "esp_task_wdt.h"
#endif

#include "ESPAsyncWebServer.h"
#include "FS.h"


class AsyncElegantOtaIprClass{

    public:
        void
            setID(const char* id),
            begin(AsyncWebServer *server, const char* username = "", const char* password = "", bool* authEnabled = nullptr, const char* firmwareVersion = ""),
            loop(),
            restart();

    private:
        AsyncWebServer *_server;

        String getID();

        String _id = getID();
        String _username = "";
        String _password = "";
        String _firmwareVersion = "";
        bool _authRequired = false;
        bool* _authEnabled = nullptr;

};

extern AsyncElegantOtaIprClass AsyncElegantOTA_IPR;

#endif
