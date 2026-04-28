/*

3D printed IP rotator
----------------------
1. Compile for HARDWARE ESP32-POE + Tools/Partition Scheme:"Default" | export bin or upload
2. $ python3 tools/generate_map_dataset.py
3. ~/inst/IP-rotator$ tools/build_spiffs_image.sh | generate bin
4. Tools/ESP32 Sketch Data Upload | upload map or use OTA

 ___               _        ___ _____ _  _
| _ \___ _ __  ___| |_ ___ / _ \_   _| || |  __ ___ _ __
|   / -_) '  \/ _ \  _/ -_) (_) || | | __ |_/ _/ _ \ '  \
|_|_\___|_|_|_\___/\__\___|\__\_\|_| |_||_(_)__\___/_|_|_|


This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Used MQTT-WALL, credit Adam Hořčica, is under MIT license,
see https://github.com/bastlirna/mqtt-wall/blob/master/license.txt

Contributors: Joerg DL3QQ, Mark G4MEM

MQTT monitor
mosquitto_sub -v -h 192.168.1.200 -t 'OK1HRA/ROT/#'
mosquitto_sub -v -h 54.38.157.134 -t 'OK1HRA/1/ROT/#'

MQTT topic
mosquitto_pub -h 192.168.1.200 -t OK1HRA/0/ROT/Target -m '10'
mosquitto_pub -h 54.38.157.134 -t BD:2F/0/ROT/Target -m '10'
mosquitto_pub -h 54.38.157.134 -t 3D:D3/0/ROT/RxAzimuth -m '10'


Changelog:
+ rotator name
+ watchdog timer, azimuth change
+ stop zone gray color
+ under 11V POE voltage watchdog
+ azimuth target
+ rotator ID for more units mqtt identification
+ calibrate gui set
+ software endstop zone
+ darkzone map, if range < 360
+ watchdog speed limit in gui
+ HW detection show in GUI
+ web online status (timeout 2s)
+ on hover button
+ fix url button
+ diffrent low/high endstop zone
+ control key
+ AC functionality
+ AZ potentiometer
+ LED
+ Supported GS-232 commands: R L A S C Mxxx O F
+ CW/CCW pulse inputs gui
+ serial baudrate set
+ warm up timer for stable value
+ AZtwoWire L=three H=two wire AZ pot
+ AZpreamp L=preamp on H=pre amp off
+ if source AZsource == false && TwoWire == true && CwRaw < 1577 | then show recomended
+ add reverse azimuth button
+ click to map during rotate stoped
+ BRAKE in DC mode support
+ if mqtt_server_ip[0]=0 then disable MQTT
+ Disable PWM from setup gui
+ if PWM in gui OFF, AC-cw out for DC input, if PWM disable and high voltage
+ small stop ramp if(abs(AzimuthTarget-Azimuth)<2)
+ rename PWR to POE on main control page
+ reset button for calibrate settings in setup
+ calibrate control potentiometer on north (show ° in setup gui)
+ DC use brake relay
+ add AzimuthStop mqttpub
+ support HW rev 6 and 7
+ add new settings to setup gui for PWM
  - PWM ramp length
  - PWM start distance
+ add new azimuth source from MQTT with topic /RxAzimuth
+ add new parameter MQTT Login and Password
+ add support for Elevation only

ToDo
- test
  - if stop, after run over target
  + 10 turn pot without preamp
  + test preamp linearity
- save settings (dump eeprom?)
- show target from az pot in map
- disable target in map if status = 0 (not rotate)
- need implement
  - CW/CCW pulse functionality
  - CW/CCW pulse pulse per grad
  - serial protocol easycomm2
- watchdog if eth_connected = false; > 5s, then reconect
- do debugu vypisovat prumer casu behu hlavni smycky v ms
- vycistit kod
  https://stackoverflow.com/questions/3420975/html5-canvas-zooming
  https://codepen.io/chengarda/pen/wRxoyB

IDE 1.8.19
Použití knihovny WiFi ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/WiFi
Použití knihovny EEPROM ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/EEPROM
Použití knihovny WebServer ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/WebServer
Použití knihovny Ethernet ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/Ethernet
Použití knihovny ESPmDNS ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/ESPmDNS
Použití knihovny ArduinoOTA ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/ArduinoOTA
Použití knihovny Update ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/Update
Použití knihovny AsyncTCP ve verzi 1.1.1 v adresáři: /home/dan/Arduino/libraries/AsyncTCP
Použití knihovny ESPAsyncWebServer ve verzi 1.2.3 v adresáři: /home/dan/Arduino/libraries/ESPAsyncWebServer
Použití knihovny FS ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/FS
Použití knihovny AsyncElegantOTA ve verzi 2.2.7 v adresáři: /home/dan/Arduino/libraries/AsyncElegantOTA
Použití knihovny PubSubClient ve verzi 2.8 v adresáři: /home/dan/Arduino/libraries/PubSubClient
Použití knihovny Wire ve verzi 2.0.0 v adresáři: /home/dan/Arduino/hardware/espressif/esp32/libraries/Wire

*/
//-------------------------------------------------------------------------------------------------------
const char* REV = "20260423";
const char* FS_BUILD_INFO_PATH = "/fs_build.txt";

// #define CN3A                      // fix ip
float NoEndstopHighZone = 0;
float NoEndstopLowZone = 0;
bool Reverse = false;
bool ReverseAZ = false;
short CcwRaw = 0;
short CwRaw = 0;
short HardwareRev = 99;
unsigned int OneTurnLimitSec = 0;

// set from GUI
String YOUR_CALL = "";
String NET_ID = "";
String RotName = "";
int StartAzimuth = 0;         // max CCW limit callibrate in real using
unsigned int MaxRotateDegree = 0;
unsigned int AntRadiationAngle = 10;
String MapUrl = "" ;
byte MapSource = 1;          // 0 = URL bitmap, 1 = locator-based map
String MapLocator = "JO60UC";
unsigned int MapZoomKm = 20000;
String GraylineNtpServer = "pool.ntp.org";
byte GraylineDarkness = 80;
byte MapTheme = 1;
bool DefaultDegOverlayEnabled = true;
bool DefaultAntOverlayEnabled = true;
bool DefaultMapLocGridEnabled = false;
bool DefaultMapGraylineEnabled = true;
bool DefaultMapBordersEnabled = false;
bool DefaultMapDxccEnabled = false;
bool FsMounted = false;
bool FsBuildInfoPresent = false;
bool FsBuildMatchesFirmware = false;
size_t FsTotalBytes = 0;
size_t FsUsedBytes = 0;
String FsBuildRev = "";
String FsBuildOffset = "";
String FsBuildSize = "";
                //$ /usr/bin/xplanet -window -config ./geoconfig -longitude 13.8 -latitude 50.0 -geometry 600x600 -projection azimuthal -num_times 1 -output ./map.png
                //$ /usr/bin/xplanet -window -config ./geoconfig -longitude 13.8 -latitude 50.0 -geometry 600x600 -projection azimuthal -radius 500 -num_times 1 -output ./OK500.png
bool Endstop =  false;
bool ACmotor =  false;
byte AZsource = 0;
short PulsePerDegree = 0;
bool AZtwoWire =  false;
bool AZpreamp =  false;

bool PWMenable = true;
unsigned int PwmRampSteps = 100;
unsigned int PwmUpDelay  = 3;  // [ms]*255steps
unsigned int PwmDwnDelay = 2;  // [ms]*255steps
byte dutyCycle = 0;
byte PwmMaxDuty = 255;
float AzimuthRawDeg = 0.0;
float AzimuthControlDeg = 0.0;
bool AzimuthControlInit = false;
float AzimuthNoiseDeg = 0.2;
float PwmSlowWindowDeg = 12.0;
byte PwmTuneAggressiveness = 2;
bool SkipBrakeLearningOnManualStop = false;
long PwmTuneSaveTimer = 0;
long StatusWatchdogTimer = 0;
long RotateWatchdogTimer = 0;
int AzimuthWatchdog = 0;
bool ErrorDetect = 0;
long TxMqttAzimuthTimer = 0;

//---------------------------------------------------------
// #define HWREV 8                     // PCB version [7-8]
#define OTAWEB                      // enable upload firmware via web
#define ETHERNET                    // Enable ESP32 ethernet (DHCP IPv4)
#define ETH_ADDR 0
#define ETH_TYPE ETH_PHY_LAN8720
#define ETH_POWER 0 //12                // #define ETH_PHY_POWER 0 ./Arduino/hardware/espressif/esp32/variants/esp32-poe/pins_arduino.h
#define ETH_MDC 23                  // MDC pin17
#define ETH_MDIO 18                 // MDIO pin16
#define ETH_CLK ETH_CLOCK_GPIO17_OUT    // CLKIN pin5 | settings for ESP32 GATEWAY rev f-g
String MACString;
char MACchar[18];
// #define ETH_CLK ETH_CLOCK_GPIO0_OUT    // settings for ESP32 GATEWAY rev c and older
// ETH.begin(ETH_ADDR, ETH_POWER, ETH_MDC, ETH_MDIO, ETH_TYPE, ETH_CLK);
// #define WIFI                     // Enable ESP32 WIFI (DHCP IPv4) - NOT TESTED
//-------------------------------------------------------------------------------------------------------

#include "driver/pcnt.h"
#include "esp_adc_cal.h"
const int AzimuthPin    = 39;  // analog
float AzimuthValue      = 0.0;
int Azimuth             = 0;
int AzimuthTarget       = -1;
int UiTargetAzimuth     = -1;
int RxAzimuth           = 0;
int Status              = 0; // -23 ManualPwmDwnCCW|-22 ManualCCW|-21 ManualStartCCW|-3 PwmDwnCCW|-2 CCW|-1 StartCCW|0 off|1 StartCW|2 CW|3 PwmDwnCW|21 ManualStartCW|22 ManualCW|23 ManualPwmDwnCW
const int VoltagePin    = 35;  // analog
float VoltageValue      = 0.0;
const float VoltageLimit = 11.5; // Keep in sync with the client-side undervoltage warning shown in data/index.html.
const int ReversePin    = 16;  //
const int PwmPin        = 4;   //

const int HWidPin       = 34;  // analog
int HWidValue           = 0;
// HardwareRev 1
const int ACcwPin       = 2;
const int BrakePin      = 33;
// const int CwCcwButtPin  = 36;  // analog
const int CwInputPin    = 36;
const int CcwInputPin   = 5;
int CwCcwInputValue      = 0;
const pcnt_unit_t CwPulseCounterUnit = PCNT_UNIT_0;
const pcnt_unit_t CcwPulseCounterUnit = PCNT_UNIT_1;
long PulseCounterValue = 0;
unsigned int PulseAzimuthTenths = 0;
bool PulseCounterConfigured = false;
const int AZmasterPin   = 32;  // analog
int AZmaster            = 142;
int AZmasterValue       = 0;
const int LedRPin       = 15;
const int LedGPin       = 14;
const int LedBPin       = 0;
const int AZtwoWirePin  = 13;
const int AZpreampPin   = 12;

// setting PWM properties
const int PwmFreq = 1000;
const int PwmResolution = 8;
const int mosfetPWMChannel = 0;
const int greenPWMChannel = 2;

// unsigned long TimerTemp;

// interrupts
#include "esp_attr.h"

// values
char WebAuthPasswordBuffer[101];
long MeasureTimer[2]={2800000,300000};   //  millis,timer (5 min)

/*
1mm rain = 15,7cm^2/10 = 1,57ml     <- by rain funnel radius
10ml = 11,5 pulses = 0,87ml/pulse   <- constanta tilting measuring cup
*/

long AlertTimer[2]={0,60000};
//  |alert...........|alert........... everry max 1 minutes and with publish max value in period

// 73 seconds WDT (WatchDogTimer)
#include <esp_task_wdt.h>
#define WDT_TIMEOUT 73
long WdtTimer=0;

byte InputByte[21];
// #define Ser2net                  // Serial to ip proxy - DISABLE if board revision 0.3 or lower
int EnableSerialDebug     = 0;
long FreneticModeTimer ;
#define HTTP_SERVER_PORT  80     // Web server port
#define DEFAULT_SWITCH_UDP_PORT 88
#define ShiftOut                 // Enable ShiftOut register
#define UdpAnswer                // Send UDP answer confirm packet

int BaudRate = 115200; // serial debug baudrate
int incomingByte = 0;   // for incoming serial data

int i = 0;
#include <WiFi.h>
#include <WiFiUdp.h>
#include <MD5Builder.h>
#include "EEPROM.h"
#define EEPROM_SIZE 421   /*

 0|Byte    1|128
 1|Char    1|A
 2|UChar   1|255
 3|Short   2|-32768
 5|UShort  2|65535
 7|Int     4|-2147483648
11|Uint    4|4294967295
15|Long    4|-2147483648
19|Ulong   4|4294967295
23|Long64  8|0x00FFFF8000FF4180
31|Ulong64 8|0x00FFFF8000FF4180
39|Float   4|1234.1234
43|Double  8|123456789.12345679
51|Bool    1|1

0-1   - NET_ID
2-22  - RotName
23  - StartAzimuth UShort
25  - MaxRotateDegree UShort
27  - AntRadiationAngle UShort
29  - Endstop Bool
30  - ACmotor Bool
31-32 - CcwRaw
33-34 - CwRaw
35 - Reverse
36 - NoEndstopLowZone
37-40 - reserved legacy
41-140 - Web authentication password
141-160 - YOUR_CALL
161-164 - MQTT broker IP
165-166 - MQTT_PORT
167 - ELEVATION
168 - MQTT_LOGIN
169-219 - MapUrl
220-221 - OneTurnLimitSec
222 - NoEndstopHighZone
223 - AZsource
224-225 PulsePerDegree
226-227 BaudRate
228 AZtwoWire
229 AZpreamp
230 - ReverseAZ
231 - PWMenable
232 - WebAuthEnabled
233 - WebAuth password storage format marker
234-5 PwmRampSteps UShort
236-245 - MQTT_USER
246-265 - MQTT_PASS
266 - MapSource (0 URL, 1 Locator)
267-272 - MapLocator (6 chars Maidenhead)
273-274 - MapZoomKm (1000-20000)
275-324 - GraylineNtpServer
325 - GraylineDarkness (0-100)
326 - MapTheme (0-5)
327 - PwmMaxDuty (40-255)
328-329 - PwmSlowWindowDeg x10 UShort
330 - PwmTuneAggressiveness (0-4)
331-394 - DXC host/IP
395-396 - DXC port
397-412 - DXC callsign
413 - DefaultDegOverlayEnabled
414 - DefaultAntOverlayEnabled
415 - DefaultMapLocGridEnabled
416 - DefaultMapGraylineEnabled
417 - DefaultMapBordersEnabled
418 - DefaultMapDxccEnabled
419-420 - PulseAzimuthTenths UShort

!! Increment EEPROM_SIZE #define !!

*/
#define CONFIG_BACKUP_FORMAT "ip-rotator-config"
#define CONFIG_BACKUP_VERSION 1
int Altitude = 0;
unsigned long WatchdogTimer=0;
String ConfigBackupUploadBuffer = "";
String ConfigBackupUploadError = "";
bool WebAuthEnabled = false;
#include <mbedtls/sha1.h>

//ajax
#include <WebServer.h>
#include "SPIFFS.h"
WebServer ajaxserver(HTTP_SERVER_PORT+8);
const char* WEB_AUTH_USER = "admin";
const char* WEB_AUTH_REALM = "IP rotator";
const char* WEB_AUTH_HEADERS[] = {"Authorization"};
String WebDigestNonce = "";
String WebDigestOpaque = "";
String WebAuthPassword = "";
WiFiClient DxcTelnetClient;
WiFiClient DxcWsClient;
String DxcHost = "";
uint16_t DxcPort = 7300;
String DxcCallsign = "";
bool DxcTelnetStatus = false;
bool DxcWsStatus = false;
bool DxcTelnetLoginPending = false;
unsigned long DxcReconnectTimer = 0;

WiFiServer server(HTTP_SERVER_PORT);
#if defined(CN3A)
  bool DHCP_ENABLE = 0;
#else
  bool DHCP_ENABLE = 1;
#endif
// Client variables
char linebuf[80];
int charcount=0;
//The udp library class
WiFiUDP UdpCommand;
uint8_t buffer[50] = "";
unsigned char packetBuffer[10];
int UDPpacketSize;
#include <ETH.h>
static bool eth_connected = false;
String HTTP_req;
#if defined(OTAWEB)
  #include <AsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include "src/AsyncElegantOTA_IPR/AsyncElegantOTA_IPR.h"
  AsyncWebServer OTAserver(82);
#endif

#define MQTT               // Enable MQTT debug
#if defined(MQTT)
  #include <PubSubClient.h>
  // #include "PubSubClient.h" // lokalni verze s upravou #define MQTT_MAX_PACKET_SIZE 128
  // WiFiClient esp32Client;
  // PubSubClient mqttClient(esp32Client);
  WiFiClient espClient;
  PubSubClient mqttClient(espClient);
  // PubSubClient mqttClient(ethClient);
   // PubSubClient mqttClient(server, 1883, callback, ethClient);
   long lastMqttReconnectAttempt = 0;
#endif
boolean MQTT_ENABLE     = 1;          // enable public to MQTT broker
IPAddress mqtt_server_ip(0, 0, 0, 0);
// byte BrokerIpArray[2][4]{
//   // {192,168,1,200},   // MQTT broker remoteqth.com
//   {54,38,157,134},   // MQTT broker remoteqth.com
// };
// IPAddress server(10, 24, 213, 92);    // MQTT broker
int MQTT_PORT;       // MQTT broker PORT
// int MQTT_PORT_Array[2] = {
//   1883,
//   1883
// };       // MQTT broker PORT
boolean ELEVATION      = 0;          // enable Elevation function
boolean MQTT_LOGIN      = 0;          // enable MQTT broker login
String MQTT_USER= "";    // MQTT broker user login
String MQTT_PASS= "";   // MQTT broker password

const int MqttBuferSize = 1000; // 1000
char mqttTX[MqttBuferSize];
char mqttPath[MqttBuferSize];
// char mqttTX[100];
// char mqttPath[100];
long MqttStatusTimer[2]{1500,1000};
long HeartBeatTimer[2]={0,1000};

// Shift register
// CC1 12 CLOCK
// CC2 13 DATA
// SBU1 14 LATCH
// SBU2 15
// const int ShiftOutClockPin = 12;
// const int ShiftOutDataPin = 13;
// const int ShiftOutLatchPin = 14;
// byte ShiftOutByte=0x00;

bool rxShiftInRead;
// https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/
#include <Wire.h>
#define I2C_SDA 33
#define I2C_SCL 32

// const int EnablePin = 13;
// const int ButtonPin = 34;

#if defined(Ser2net)
  #define RX1 3
  #define TX1 1
  HardwareSerial Serial_one(1);
#endif

// SD
// #define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
// #define ETH_PHY_POWER 12
// #include "FS.h"
// #include "SD_MMC.h"

// ntp
#include "time.h"
// const char* ntpServer = "tik.cesnet.cz";
// const char* ntpServer = "time.google.com";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;

void ApplyGraylineNtpConfig(){
  configTime(gmtOffset_sec, daylightOffset_sec, GraylineNtpServer.c_str());
}

bool GraylineUtcAvailable(time_t* nowOut = nullptr){
  time_t now = time(nullptr);
  if(now < 1700000000){
    return false;
  }
  if(nowOut != nullptr){
    *nowOut = now;
  }
  return true;
}

// Explicit prototypes keep Arduino's sketch preprocessor from breaking
// if the large comment header is malformed or contains unusual content.
uint32_t readADC_Cal(int ADC_Raw);
bool IsSafeWebAuthPasswordValue(const String& value);
bool LooksLikeLegacyWebAuthKey(const String& value);
void Watchdog();
void LedStatus();
void LedStatusErr();
void RotCalculate();
void RunTimer();
void DetectEndstopZone();
void EthTest();
void RunByStatus();
void ReverseProcedure(bool CCW);
String LeadingZero(int NumberOfZero, int NR);
void CLI2();
void Enter();
void EnterChar(int OUT);
void Prn(int OUT, int LN, String STR);
void ListCommands(int OUT);
String ReadSerialCommandTail(byte maxLen, unsigned long timeoutMs);
void PrintWebKey(int OUT);
void DisableWebAuth(int OUT);
void http();
void EthEvent(WiFiEvent_t event);
void Mqtt();
bool mqttReconnect();
void reSubscribe();
void MqttRx(char *topic, byte *payload, unsigned int length);
void AfterMQTTconnect();
void MqttPubString(String TOPIC, String DATA, bool RETAIN);
String UtcTime(int format);
void InitWebAuth();
String Md5Hex(const String& value);
String DigestRandomHex();
String ExtractHttpHeader(const String& request, const String& headerName);
String ExtractDigestParam(const String& authHeader, const String& name, char delimiter);
bool CheckDigestAuthorization(const String& authHeader, const String& method, const String& uri);
void SendHttpDigestChallenge(WiFiClient& webClient);
bool RequireAjaxAuth();
bool AjaxAuthOk();
void RegisterAjaxRoute(const char* uri, WebServer::THandlerFunction handler);
void RegisterAjaxRoute(const char* uri, HTTPMethod method, WebServer::THandlerFunction handler);
void RegisterAjaxUploadRoute(const char* uri, HTTPMethod method, WebServer::THandlerFunction handler, WebServer::THandlerFunction uploadHandler);
void handlePostRot();
void handleSet();
void handleCal();
bool streamStaticFile(const char* path, const char* contentType);
void handleRoot();
void handleADC();
void handleAZ();
void handleFrontAZ();
void handleAZadc();
void handleAZsource();
void handlePulseCounter();
void handleStat();
void handleTargetUi();
void handleStart();
void handleElevation();
void handleMax();
void handleAnt();
void handleAntName();
void handleMapUrl();
void handleMapSource();
void handleMapLocator();
void handleMapZoomKm();
void handleSetMapLocator();
void handleSetMapZoomKm();
void handleMapTheme();
void handleGraylineDarkness();
void handleMapDisplayDefaults();
void handleGraylineInfo();
void handleRev();
void handleFsDiag();
void handlePwmUi();
void handleSetPwmMaxDuty();
void handleMap50js();
void handleMap50jsGz();
void handleFontRegular();
void handleFontBold();
void handleBackupConfigDownload();
void handleBackupConfigUpload();
void handleBackupConfigUploadData();
void handleEndstop();
void handleEndstopLowZone();
void handleEndstopHighZone();
void handleSetEndstopZones();
void handleCwraw();
void handleCcwraw();
void handleMAC();
void handleUptime();
void handleDxcHtml();
void DxcLoop();
void DxcDisconnectTelnet();
void DxcDisconnectWebSocket();
bool DxcConfigReady();
void DxcRequestReconnect();
bool DxcConnectTelnet();
void DxcUpdateTelnetStatus(bool connected, bool forceSend = false);
void DxcSendTelnetStatus();
bool DxcHandleWebSocketUpgrade(WiFiClient& webClient, const String& request, const String& method, const String& uri);
String DxcComputeWebSocketAccept(const String& secKey);
String Base64Encode(const uint8_t* data, size_t length);
bool DxcSendWebSocketFrame(uint8_t opcode, const uint8_t* payload, size_t length);
bool DxcSendWebSocketText(const char* text);
bool DxcSendWebSocketText(const String& text);
void DxcHandleWebSocketClient();
void DxcHandleTelnetClient();
void WriteFixedStringToEeprom(int start, int length, const String& text);
float ClampFloat(float value, float minValue, float maxValue);
byte ClampDuty(int value);
float ReadControlAzimuthDeg();
float ReadRawAzimuthDeg();
float ReadRemainingDistanceDeg(int directionSign, float azimuthDeg);
float GetStopToleranceDeg();
bool TargetReachedInDirection(int directionSign);
byte ComputeClosedLoopDuty(int directionSign);
void ApplyDutySlew(byte targetDuty, long &PwmTimer);
void MaybeBeginBrakeLearning(int directionSign, bool &brakeLearningActive, int &brakeLearningDirection, float &brakeStartDistance);
void UpdateAutoSlowWindow(float brakeStartDistance, float finalDistance);
void PersistAutoSlowWindowIfNeeded();
void RefreshFilesystemDiagnostics();
bool LoadFilesystemBuildInfo();
float GetPwmTuneAggressionFactor();
String JsonEscape(const String& value);
bool IsSafeConfigChar(byte value);
String ReadFixedStringFromEeprom(int start, int length);
bool JsonExtractString(const String& json, const char* key, String& value);
bool JsonExtractLong(const String& json, const char* key, long& value);
bool JsonExtractFloat(const String& json, const char* key, float& value);
bool JsonExtractBool(const String& json, const char* key, bool& value);
String ExportConfigBackupJson();
String ImportConfigBackupJson(const String& jsonPayload);
float GetPwmTuneLeadOffsetDeg();
bool ShouldForceStopFromPwmStall(int directionSign, byte currentDuty, float rawAzimuthDeg, byte maxDuty);
void RequestStopRamp(bool suppressBrakeLearning);
bool IsManualStatusValue(int statusValue);
bool IsManualPwmKeyControlEnabled();
void RequestManualStopRamp();
bool HandleManualPwmStatus(long &PwmTimer);
bool IsPulseAzimuthSource();
void ConfigurePulseCounter();
void ApplyAzimuthSourceMode(bool useStoredPulseAzimuth);
void UpdatePulseAzimuthFromCounter();
void PersistPulseAzimuthIfNeeded(bool forceSave);
unsigned int ClampPulseAzimuthTenths(int valueTenths);
long ClampPulseCount(long pulseCount);
int GetPulseVirtualAdcValue();
int GetPulseCounterValue();

//-------------------------------------------------------------------------------------------------------

void setup() {

  pinMode(AzimuthPin, INPUT);
  // pinMode(CwCcwButtPin, INPUT);
  pinMode(CwInputPin, INPUT);
  pinMode(CcwInputPin, INPUT);
  ConfigurePulseCounter();

  pinMode(HWidPin, INPUT);
    HWidValue = readADC_Cal(analogRead(HWidPin));
    if(HWidValue<=150){
      HardwareRev=0;
    }else if(HWidValue>150 && HWidValue<=450){
      HardwareRev=3;  // 319
    }else if(HWidValue>450 && HWidValue<=700){
      HardwareRev=4;  // 604
    }else if(HWidValue>700 && HWidValue<=900){
      HardwareRev=5;  // 807
    }else if(HWidValue>900 && HWidValue<=1170){
      HardwareRev=6;  // 1036
    }else if(HWidValue>1170){
      HardwareRev=7;  // 1304
    }
  pinMode(VoltagePin, INPUT);
  pinMode(ReversePin, OUTPUT);
    digitalWrite(ReversePin, LOW);


  ledcSetup(mosfetPWMChannel, PwmFreq, PwmResolution);
  ledcSetup(greenPWMChannel, PwmFreq, PwmResolution);
  ledcAttachPin(PwmPin, mosfetPWMChannel);
  ledcAttachPin(LedGPin, greenPWMChannel);
  ledcWrite(mosfetPWMChannel, 0);
  ledcWrite(greenPWMChannel, 0);

  // HardwareRev 1
  pinMode(AZmasterPin, INPUT);
  pinMode(ACcwPin, OUTPUT);
    digitalWrite(ACcwPin, LOW);
  pinMode(BrakePin, OUTPUT);
    digitalWrite(BrakePin, LOW);
  pinMode(LedRPin, OUTPUT);
    digitalWrite(LedRPin, LOW);
  // pinMode(LedGPin, OUTPUT);
  //   digitalWrite(LedGPin, HIGH);
  pinMode(LedBPin, OUTPUT);
    digitalWrite(LedBPin, LOW);

  pinMode(AZtwoWirePin, OUTPUT);
    digitalWrite(AZtwoWirePin, LOW);
  pinMode(AZpreampPin, OUTPUT);
    digitalWrite(AZpreampPin, LOW);

  // pinMode(ShiftOutClockPin, OUTPUT);
  // pinMode(ShiftOutDataPin, OUTPUT);
  // pinMode(ShiftOutLatchPin, OUTPUT);
  // digitalWrite(ShiftOutLatchPin, HIGH);

  // for (int i = 0; i < 8; i++) {
  //   pinMode(TestPin[i], INPUT);
  // }
  // pinMode(EnablePin, OUTPUT);
  // digitalWrite(EnablePin,1);

  // pinMode(ButtonPin, INPUT);
  // SHIFT IN
  // pinMode(ShiftInLatchPin, OUTPUT);
  // pinMode(ShiftInClockPin, OUTPUT);
  // pinMode(ShiftInDataPin, INPUT);

    // Listen source
  if (!EEPROM.begin(EEPROM_SIZE)){
    // if(EnableSerialDebug>0){
    //   Serial.println("failed to initialise EEPROM"); delay(1);
    // }
  }

  // 226-227 BaudRate
  if(EEPROM.read(226)==0xff || EEPROM.readUShort(226) > 9600){
    BaudRate=115200;
  }else{
    BaudRate = EEPROM.readUShort(226);
  }

  Serial.begin(BaudRate); //BaudRate
  while(!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // // SD
  // if(!SD_MMC.begin()){
  //   Serial.println("SD card Mount Failed");
  //   // return;
  // }


  // 0-1 net ID
    if(EEPROM.read(0)==0xff){
      NET_ID="0";
    }else{
      NET_ID = ReadFixedStringFromEeprom(0, 2);
    }

  // 2-22 RotName
  if(EEPROM.read(2)==0xff){
    RotName="Antenna";
  }else{
    RotName = ReadFixedStringFromEeprom(2, 21);
  }

  // 23 StartAzimuth
  if(EEPROM.read(23)==0xff){
    StartAzimuth=0;
  }else{
    if(EEPROM.readUShort(23)<359 && EEPROM.readUShort(23)>0){
      StartAzimuth = EEPROM.readUShort(23);
    }
  }

  // 25 MaxRotateDegree
  if(EEPROM.read(25)==0xff){
    MaxRotateDegree=360;
  }else{
    MaxRotateDegree = EEPROM.readUShort(25);
  }

  // 27 AntRadiationAngle
  if(EEPROM.read(27)==0xff){
    AntRadiationAngle=45;
  }else{
    if(EEPROM.readUShort(27)<180 && EEPROM.readUShort(27)>0){
      AntRadiationAngle = EEPROM.readUShort(27);
    }else{
      AntRadiationAngle=46;
    }
  }

  // 169-219 - MapUrl
  if(EEPROM.read(169)==0xff){
    MapUrl="https://remoteqth.com/xplanet/OK.png";
  }else{
    MapUrl = ReadFixedStringFromEeprom(169, 51);
  }

  // 266 - MapSource
  if(EEPROM.read(266)==0xff){
    MapSource=1;
  }else{
    if(EEPROM.readByte(266)<2){
      MapSource = EEPROM.readByte(266);
    }else{
      MapSource=1;
    }
  }

  // 267-272 - MapLocator
  if(EEPROM.read(267)==0xff){
    MapLocator="JO60UC";
  }else{
    MapLocator = ReadFixedStringFromEeprom(267, 6);
    MapLocator.toUpperCase();
    if(MapLocator.length()!=6){
      MapLocator="JO60UC";
    }
  }

  // 273-274 - MapZoomKm
  if(EEPROM.read(273)==0xff){
    MapZoomKm=20000;
  }else{
    if(EEPROM.readUShort(273)>=1000 && EEPROM.readUShort(273)<=20000){
      MapZoomKm = EEPROM.readUShort(273);
    }else{
      MapZoomKm=20000;
    }
  }

  // 275-324 - GraylineNtpServer
  if(EEPROM.read(275)==0xff){
    GraylineNtpServer = "pool.ntp.org";
  }else{
    GraylineNtpServer = ReadFixedStringFromEeprom(275, 50);
    GraylineNtpServer.trim();
    if(GraylineNtpServer.length()<1 || GraylineNtpServer.length()>50){
      GraylineNtpServer = "pool.ntp.org";
    }
  }

  // 325 - GraylineDarkness
  if(EEPROM.read(325)==0xff){
    GraylineDarkness = 80;
  }else{
    if(EEPROM.readByte(325) <= 100){
      GraylineDarkness = EEPROM.readByte(325);
    }else{
      GraylineDarkness = 80;
    }
  }

  // 326 - MapTheme
  if(EEPROM.read(326)==0xff){
    MapTheme = 1;
  }else{
    if(EEPROM.readByte(326) <= 5){
      MapTheme = EEPROM.readByte(326);
    }else{
      MapTheme = 1;
    }
  }

  // 413-418 - control page default map/display toggles
  if(EEPROM.read(413)==0xff){
    DefaultDegOverlayEnabled = true;
  }else{
    DefaultDegOverlayEnabled = EEPROM.readBool(413);
  }
  if(EEPROM.read(414)==0xff){
    DefaultAntOverlayEnabled = true;
  }else{
    DefaultAntOverlayEnabled = EEPROM.readBool(414);
  }
  if(EEPROM.read(415)==0xff){
    DefaultMapLocGridEnabled = false;
  }else{
    DefaultMapLocGridEnabled = EEPROM.readBool(415);
  }
  if(EEPROM.read(416)==0xff){
    DefaultMapGraylineEnabled = true;
  }else{
    DefaultMapGraylineEnabled = EEPROM.readBool(416);
  }
  if(EEPROM.read(417)==0xff){
    DefaultMapBordersEnabled = false;
  }else{
    DefaultMapBordersEnabled = EEPROM.readBool(417);
  }
  if(EEPROM.read(418)==0xff){
    DefaultMapDxccEnabled = false;
  }else{
    DefaultMapDxccEnabled = EEPROM.readBool(418);
  }
  if(EEPROM.read(419)==0xff){
    PulseAzimuthTenths = 0;
  }else{
    PulseAzimuthTenths = ClampPulseAzimuthTenths(EEPROM.readUShort(419));
  }

  // 29  - Endstop
  if(EEPROM.read(29)==0xff){
    Endstop=false;
  }else{
    if(EEPROM.readBool(29)==1){
      Endstop=true;
    }else{
      Endstop=false;
    }
  }

  // 30  - ACmotor
  if(EEPROM.read(30)==0xff){
    ACmotor=false;
  }else{
    if(EEPROM.readBool(30)==1){
      ACmotor=true;
    }else{
      ACmotor=false;
    }
  }

  // 31-32 CcwRawBaudRate
  if(EEPROM.read(31)==0xff){
    CcwRaw=392;
  }else{
    CcwRaw = EEPROM.readUShort(31);
  }

  // 33-34  CwRaw
  if(EEPROM.read(33)==0xff){
    CwRaw=3000;
  }else{
    CwRaw = EEPROM.readUShort(33);
  }

  // 35 - Reverse
  if(EEPROM.read(35)==0xff){
    Reverse=false;
  }else{
    if(EEPROM.readBool(35)==1){
      Reverse=true;
    }else{
      Reverse=false;
    }
  }

  // 36 - NoEndstopLowZone
  if(EEPROM.read(36)==0xff){
    NoEndstopLowZone = 0.5;
  }else{
    if(EEPROM.readByte(36)<16){
      NoEndstopLowZone = float(EEPROM.readByte(36))/10;
    }else{
      NoEndstopLowZone = 0.5;
    }
  }


  // 167  - ELEVATION
  if(EEPROM.read(167)==0xff){
    ELEVATION=false;
  }else{
    if(EEPROM.readBool(167)==1){
      ELEVATION=true;
    }else{
      ELEVATION=false;
    }
  }

  // 168  - MQTT_LOGIN
  if(EEPROM.read(168)==0xff){
    MQTT_LOGIN=false;
  }else{
    if(EEPROM.readBool(168)==1){
      MQTT_LOGIN=true;
    }else{
      MQTT_LOGIN=false;
    }
  }

  // 222 - NoEndstopHighZone
  if(EEPROM.read(222)==0xff){
    NoEndstopHighZone = 2.8;
  }else{
    if(EEPROM.readByte(222)>15 && EEPROM.readByte(222)<34){
      NoEndstopHighZone = float(EEPROM.readByte(222))/10;
    }else{
      NoEndstopHighZone = 2.8;
    }
  }
  // NoEndstopHighZone = 3.3 - NoEndstopLowZone;
  // NoEndstopLowZone = NoEndstopLowZone;

  // 223 - AZsource
  if(EEPROM.read(223)==0xff){
    AZsource=0;
  }else{
    AZsource=EEPROM.readByte(223);
  }

  // 224-225  PulsePerDegree
  if(EEPROM.read(224)==0xff){
    PulsePerDegree=1;
  }else{
    PulsePerDegree = EEPROM.readUShort(224);
  }

  // 228 AZtwoWire
  if(EEPROM.read(228)==0xff){
    AZtwoWire=false;
  }else{
    if(EEPROM.readBool(228)==1){
      AZtwoWire=true;
    }else{
      AZtwoWire=false;
    }
  }
  digitalWrite(AZtwoWirePin, AZtwoWire);

  // 229 AZpreamp
  if(EEPROM.read(229)==0xff){
    AZpreamp=false;
  }else{
    if(EEPROM.readBool(229)==1){
      AZpreamp=true;
    }else{
      AZpreamp=false;
    }
  }
  digitalWrite(AZpreampPin, AZpreamp);

  // 230 - ReverseAZ
  if(EEPROM.read(230)==0xff){
    ReverseAZ=false;
  }else{
    if(EEPROM.readBool(230)==1){
      ReverseAZ=true;
    }else{
      ReverseAZ=false;
    }
  }

  // 231 - PWMenable = true;
  if(EEPROM.read(231)==0xff){
    PWMenable=false;
  }else{
    if(EEPROM.readBool(231)==1){
      PWMenable=true;
    }else{
      PWMenable=false;
    }
  }

  // 234 PwmRampSteps UShort
  if(EEPROM.read(234)==0xff){
    PwmRampSteps=100;
  }else{
    PwmRampSteps = EEPROM.readUShort(234);
    if(PwmRampSteps<1 || PwmRampSteps>200){
      PwmRampSteps=100;
    }
  }

  // 327 PwmMaxDuty
  if(EEPROM.read(327)==0xff){
    PwmMaxDuty=255;
  }else{
    PwmMaxDuty = EEPROM.readByte(327);
    if(PwmMaxDuty<40){
      PwmMaxDuty=40;
    }
  }

  // 328-329 PwmSlowWindowDeg x10
  if(EEPROM.read(328)==0xff){
    PwmSlowWindowDeg=12.0;
  }else{
    PwmSlowWindowDeg = float(EEPROM.readUShort(328)) / 10.0;
    if(PwmSlowWindowDeg < 3.0 || PwmSlowWindowDeg > 60.0){
      PwmSlowWindowDeg=12.0;
    }
  }

  // 330 PwmTuneAggressiveness
  if(EEPROM.read(330)==0xff){
    PwmTuneAggressiveness=2;
  }else{
    PwmTuneAggressiveness = EEPROM.readByte(330);
    if(PwmTuneAggressiveness > 4){
      PwmTuneAggressiveness=2;
    }
  }

  // 331-394 DXC host/IP
  if(EEPROM.read(331)==0xff){
    DxcHost = "";
  }else{
    DxcHost = ReadFixedStringFromEeprom(331, 64);
  }

  // 395-396 DXC port
  if(EEPROM.read(395)==0xff){
    DxcPort = 7300;
  }else{
    DxcPort = EEPROM.readUShort(395);
    if(DxcPort < 1){
      DxcPort = 7300;
    }
  }

  // 397-412 DXC callsign
  if(EEPROM.read(397)==0xff){
    DxcCallsign = YOUR_CALL;
  }else{
    DxcCallsign = ReadFixedStringFromEeprom(397, 16);
  }

  // 236-245 - MQTT_USER
  if(EEPROM.read(236)==0xff){
    MQTT_USER="Login";
  }else{
    MQTT_USER = ReadFixedStringFromEeprom(236, 10);
  }

  // 246-265 - MQTT_PASS
  if(EEPROM.read(246)==0xff){
    MQTT_PASS="Password";
  }else{
    MQTT_PASS = ReadFixedStringFromEeprom(246, 20);
  }

  // 41-140 WebAuthPassword
  for(int i=41; i<141; i++){
    char currentChar = EEPROM.readChar(i);
    if(currentChar == char(0xff) || currentChar == '\0'){
      WebAuthPasswordBuffer[i-41] = '\0';
      break;
    }
    WebAuthPasswordBuffer[i-41] = currentChar;
  }
  WebAuthPasswordBuffer[100] = '\0';
  WebAuthPassword = String(WebAuthPasswordBuffer);
  bool webAuthPasswordStoredDirectly = EEPROM.readByte(233) == 0xA5;
  if(!webAuthPasswordStoredDirectly && LooksLikeLegacyWebAuthKey(WebAuthPassword)){
    WebAuthPassword = Md5Hex(WebAuthPassword);
    WriteFixedStringToEeprom(41, 100, WebAuthPassword);
    EEPROM.writeByte(233, 0xA5);
    EEPROM.commit();
  }else if(!webAuthPasswordStoredDirectly){
    EEPROM.writeByte(233, 0xA5);
    EEPROM.commit();
  }

  // 232 - WebAuthEnabled, default OFF on blank EEPROM.
  if(EEPROM.read(232)==0xff){
    WebAuthEnabled = false;
  }else{
    WebAuthEnabled = EEPROM.readBool(232);
  }
  InitWebAuth();
  ApplyAzimuthSourceMode(true);

  // YOUR_CALL
  // move after ETH init

  // MQTT broker IP
  if(EEPROM.read(161)==0xff){
    mqtt_server_ip[0]=54;
  }else{
    mqtt_server_ip[0]=EEPROM.readByte(161);
    if(mqtt_server_ip[0]==0){
      MQTT_ENABLE = false;
    }
  }

  if(EEPROM.read(162)==0xff){
    mqtt_server_ip[1]=38;
  }else{
    mqtt_server_ip[1]=EEPROM.readByte(162);
  }

  if(EEPROM.read(163)==0xff){
    mqtt_server_ip[2]=157;
  }else{
    mqtt_server_ip[2]=EEPROM.readByte(163);
  }

  if(EEPROM.read(164)==0xff){
    mqtt_server_ip[3]=134;
  }else{
    mqtt_server_ip[3]=EEPROM.readByte(164);
  }

  if(EEPROM.read(165)==0xff){
    MQTT_PORT=1883;
  }else{
    MQTT_PORT = EEPROM.readUShort(165);
  }

  // OneTurnLimitSec
  if(EEPROM.read(220)==0xff){
    OneTurnLimitSec=360;
  }else{
    OneTurnLimitSec = EEPROM.readUShort(220);
  }
  // // 231-234 Altitude
  // if(EEPROM.readByte(231)!=255){
  //   Altitude = EEPROM.readInt(231);
  // }

  #if defined(ETHERNET)
    // mqtt_server_ip=BrokerIpArray[0];
    // MQTT_PORT = MQTT_PORT_Array[0];

    WiFi.onEvent(EthEvent);
    // ETH.begin();
    ETH.begin(ETH_ADDR, ETH_POWER, ETH_MDC, ETH_MDIO, ETH_TYPE, ETH_CLK);
    if(DHCP_ENABLE==false){
      ETH.config(IPAddress(192, 168, 0, 11), IPAddress(192, 168, 0, 1),IPAddress(255, 255, 255, 0),IPAddress(8, 8, 8, 8));
      //config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = (uint32_t)0x00000000, IPAddress dns2 = (uint32_t)0x00000000);
    }
  #endif
    RefreshFilesystemDiagnostics();
    server.begin();
    UdpCommand.begin(DEFAULT_SWITCH_UDP_PORT);    // incoming udp port
    // chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
    //   unsigned long long1 = (unsigned long)((chipid & 0xFFFF0000) >> 16 );
    //   unsigned long long2 = (unsigned long)((chipid & 0x0000FFFF));
    //   ChipidHex = String(long1, HEX) + String(long2, HEX); // six octets
    //   YOUR_CALL=ChipidHex;

  #if defined(OTAWEB)
    OTAserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if(WebAuthEnabled && !request->authenticate(WEB_AUTH_USER, WebAuthPassword.c_str())){
          return request->requestAuthentication();
        }
        request->send(200, "text/plain", "PSE QSY to /update");
    });
    AsyncElegantOTA_IPR.begin(&OTAserver, WEB_AUTH_USER, WebAuthPassword.c_str(), &WebAuthEnabled);    // Start OTA
    OTAserver.begin();
  #endif
  //------------------------------------------------

  // digitalWrite(EnablePin,0);

  // WDT
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch
  WdtTimer=millis();

  // init and get UTC for grayline and other time-dependent features
   ApplyGraylineNtpConfig();

   // ajax
   ajaxserver.collectHeaders(WEB_AUTH_HEADERS, 1);
   RegisterAjaxRoute("/", HTTP_POST, handlePostRot);
   // ajaxserver.on("/STOP",HTTP_POST, handlePostStop);
   RegisterAjaxRoute("/", handleRoot);      //This is display page
   RegisterAjaxRoute("/readADC", handleADC);//To get update of ADC Value only
  RegisterAjaxRoute("/readAZ", handleAZ);
  RegisterAjaxRoute("/readFrontAZ", handleFrontAZ);
  RegisterAjaxRoute("/readAZadc", handleAZadc);
  RegisterAjaxRoute("/readAZsource", handleAZsource);
  RegisterAjaxRoute("/readPulseCounter", handlePulseCounter);
  RegisterAjaxRoute("/readStat", handleStat);
   RegisterAjaxRoute("/readTargetUi", handleTargetUi);
   RegisterAjaxRoute("/readStart", handleStart);
   RegisterAjaxRoute("/readElevation", handleElevation);
   RegisterAjaxRoute("/readMax", handleMax);
   RegisterAjaxRoute("/readAnt", handleAnt);
   RegisterAjaxRoute("/readAntName", handleAntName);
   RegisterAjaxRoute("/readMapUrl", handleMapUrl);
   RegisterAjaxRoute("/readMapSource", handleMapSource);
   RegisterAjaxRoute("/readMapLocator", handleMapLocator);
   RegisterAjaxRoute("/readMapZoomKm", handleMapZoomKm);
   RegisterAjaxRoute("/setMapLocator", handleSetMapLocator);
  RegisterAjaxRoute("/setMapZoomKm", handleSetMapZoomKm);
  RegisterAjaxRoute("/readMapTheme", handleMapTheme);
  RegisterAjaxRoute("/readMapDisplayDefaults", handleMapDisplayDefaults);
  RegisterAjaxRoute("/readGraylineDarkness", handleGraylineDarkness);
  RegisterAjaxRoute("/readGraylineInfo", handleGraylineInfo);
  RegisterAjaxRoute("/readRev", handleRev);
 RegisterAjaxRoute("/readFsDiag", handleFsDiag);
 RegisterAjaxRoute("/readPwmUi", handlePwmUi);
 RegisterAjaxRoute("/setPwmMaxDuty", handleSetPwmMaxDuty);
 RegisterAjaxRoute("/backup/config", HTTP_GET, handleBackupConfigDownload);
 RegisterAjaxUploadRoute("/backup/config", HTTP_POST, handleBackupConfigUpload, handleBackupConfigUploadData);
 RegisterAjaxRoute("/map50.js", handleMap50js);
  RegisterAjaxRoute("/map50.js.gz", handleMap50jsGz);
  RegisterAjaxRoute("/RC-R.ttf", handleFontRegular);
  RegisterAjaxRoute("/RC-B.ttf", handleFontBold);
  RegisterAjaxRoute("/dxc.html", handleDxcHtml);
  RegisterAjaxRoute("/set", handleSet);
  RegisterAjaxRoute("/cal", handleCal);
  RegisterAjaxRoute("/readEndstop", handleEndstop);
  RegisterAjaxRoute("/readEndstopLowZone", handleEndstopLowZone);
  RegisterAjaxRoute("/readEndstopHighZone", handleEndstopHighZone);
  RegisterAjaxRoute("/setEndstopZones", handleSetEndstopZones);
  RegisterAjaxRoute("/readCwraw", handleCwraw);
  RegisterAjaxRoute("/readCcwraw", handleCcwraw);
  RegisterAjaxRoute("/readMAC", handleMAC);
  RegisterAjaxRoute("/readUptime", handleUptime);
   // ajaxserver.on("/cal/readAZ", handleAZ);
   ajaxserver.begin();                  //Start server
   Serial.println("HTTP ajax server started");

}

//-------------------------------------------------------------------------------------------------------

void loop() {
  http();
  Mqtt();
  CLI2();
  ajaxserver.handleClient();
  DxcLoop();
  RunByStatus();
  Watchdog();

  #if defined(OTAWEB)
   // OTAserver.on("/printIp", HTTP_GET, [](AsyncWebServerRequest *request){
   //     request->send(200, "text/plain", "ok");
   //     Serial.println(request->client()->remoteIP());
   // });
   AsyncElegantOTA_IPR.loop();
  #endif


  // shift register test
  // for (int i=0; i<8; i++){
  //   bitSet(ShiftOutByte, i);
  //   ShiftOutSet(ShiftOutByte);
  //   bitClear(ShiftOutByte, i);
  //   delay(100);
  // }

}
// SUBROUTINES -------------------------------------------------------------------------------------------------------

// void ShiftOutSet(byte SetByte){
//   digitalWrite(ShiftOutLatchPin, LOW);
//   shiftOut(ShiftOutDataPin, ShiftOutClockPin, LSBFIRST, SetByte);
//   digitalWrite(ShiftOutLatchPin, HIGH);
// }

//-------------------------------------------------------------------------------------------------------
uint32_t readADC_Cal(int ADC_Raw)
{
  esp_adc_cal_characteristics_t adc_chars;

  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  return(esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}
//-------------------------------------------------------------------------------------------------------
void Watchdog(){
  static long ADCTimer = 0;
  static long ADCCounter = 0;
  static int AZBuffer = 0;
  static int AZmasterBuffer = 0;
  static float VoltageBuffer = 0;
  // Azimuth refresh 175 ms
  if(millis()-ADCTimer > 5){
    AZBuffer = AZBuffer + readADC_Cal(analogRead(AzimuthPin));
    AZmasterBuffer = AZmasterBuffer + readADC_Cal(analogRead(AZmasterPin));
    // R divider | 12,95/2,95=4,38983050847458
    VoltageBuffer = VoltageBuffer + readADC_Cal(analogRead(VoltagePin))/1000.0*4.39;
    ADCCounter ++;
    if(ADCCounter > 34){
      AzimuthValue = AZBuffer/35;
      AZmasterValue = AZmasterBuffer/35;
      VoltageValue = VoltageBuffer/35;
      ADCCounter = 0;
      AZBuffer = 0;
      AZmasterBuffer = 0;
      VoltageBuffer = 0;
      CwCcwInputValue = 0;
      if(digitalRead(CwInputPin)==LOW){
        CwCcwInputValue = 1;
      }
      if(digitalRead(CcwInputPin)==LOW){
        CwCcwInputValue = 2;
      }

      if(ReverseAZ==true){
        AzimuthValue=map(AzimuthValue, 142, 3155, 3155, 142);
      }
    }
    if(AZsource == 0){ //potentiometer
      AzimuthRawDeg=map(AzimuthValue, CcwRaw, CwRaw, 0, MaxRotateDegree);
      float controlAlpha = (Status==0) ? 0.18 : 0.45;
      if(AzimuthControlInit==false){
        AzimuthControlDeg = AzimuthRawDeg;
        AzimuthControlInit = true;
      }else{
        AzimuthControlDeg = AzimuthControlDeg + (AzimuthRawDeg - AzimuthControlDeg) * controlAlpha;
      }
      AzimuthNoiseDeg = AzimuthNoiseDeg * 0.92 + abs(AzimuthRawDeg - AzimuthControlDeg) * 0.08;
      Azimuth=int(round(AzimuthRawDeg));
    }else if(IsPulseAzimuthSource()){
      UpdatePulseAzimuthFromCounter();
    }else{
      AzimuthRawDeg=Azimuth;
      AzimuthControlDeg=Azimuth;
      AzimuthNoiseDeg=0.2;
    }
    ADCTimer=millis();
    AZmaster=map(AZmasterValue, 142, 3150, 0, 360);
    if(AZmaster<=180){
      AZmaster=AZmaster+180;
    }else{
      AZmaster=AZmaster-180;
    }
  }

  // KEY
  if(AZsource==0){ // potentiometer
    static bool RunByKey = false;
    if(IsManualPwmKeyControlEnabled()){
      RunByKey=false;
      if(CwCcwInputValue==1){
        if(Status==0){
          Status=21;
        }
      }else if(CwCcwInputValue==2){
        if(Status==0){
          Status=-21;
        }
      }else if(IsManualStatusValue(Status)){
        RequestManualStopRamp();
      }
    }else if(CwCcwInputValue==1 && Status>=0){
      if(Status==0){
        Status=1; //digitalWrite(BrakePin, HIGH); delay(24);
        RunByKey=true;
      }
      // RunTimer();
    }else if(CwCcwInputValue==2 && Status<=0){
      if(Status==0){
        Status=-1; //digitalWrite(BrakePin, HIGH); delay(24);
        RunByKey=true;
      }
      // RunTimer();
    }else if(RunByKey==true){
      RequestStopRamp(true);
      RunByKey=false;
    }
  }

  static long AZchangeTimer = 0;
  static int AzimuthTmp = 0;
  static int CwCcwInputValueTmp = 0;
  static float VoltageValueTmp = 0;
  static int StatusTmp = 0; // -3 PwmDwnCCW|-2 CCW|-1 PwmUpCCW|0 off|1 PwmUpCW|2 CW|3 PwmDwnCW
  static String StatusStr = "";

  // Status change
  if(StatusTmp!=Status){
    switch (Status) {
      case -23: {StatusStr = "ManualPwmDwn-CCW"; break; }
      case -22: {StatusStr = "Manual-CCW"; break; }
      case -21: {StatusStr = "ManualStart-CCW"; break; }
      case -3: {StatusStr = "PwmDwn-CCW"; break; }
      case -2: {StatusStr = "CCW"; break; }
      case -11: {StatusStr = "PwmUp-CCW"; break; }
      case -1: {StatusStr = "START-CCW"; break; }
      case  0: {StatusStr = "STOP"; break; }
      case  1: {StatusStr = "START-CW"; break; }
      case  21: {StatusStr = "ManualStart-CW"; break; }
      case  22: {StatusStr = "Manual-CW"; break; }
      case  23: {StatusStr = "ManualPwmDwn-CW"; break; }
      case  11: {StatusStr = "PwmUp-CW"; break; }
      case  2: {StatusStr = "CW"; break; }
      case  3: {StatusStr = "PwmDwn-CW"; break; }
    }
    MqttPubString("StatusHuman", StatusStr, false);
    MqttPubString("Status", String(Status+0), false); // +4)
    if(Status == 0 && StatusTmp != 0){
      PersistPulseAzimuthIfNeeded(true);
    }
    StatusTmp=Status;
    LedStatus();
  }

  // info if change AZ and voltage
  if( (Status==0 && millis()-AZchangeTimer >2000) || (Status!=0 && millis()-AZchangeTimer >100) ){
    if(AzimuthTmp!=Azimuth){
      MqttPubString("Azimuth", String(Azimuth), false);
      if(Status==0){
        MqttPubString("AzimuthStop", String(Azimuth), false);
      }
      AzimuthTmp=Azimuth;
      TxMqttAzimuthTimer=millis();
    }
    if(CwCcwInputValueTmp!=CwCcwInputValue){
      MqttPubString("CwCcwInputValue", String(CwCcwInputValue), false);
      CwCcwInputValueTmp=CwCcwInputValue;
    }
    if(abs(VoltageValueTmp-VoltageValue)>0.5){
      MqttPubString("VoltageValue", String(VoltageValue), false);
      VoltageValueTmp=VoltageValue;
    }
    AZchangeTimer=millis();
  }

  // minimal Azimuth propagation (heartbeat) 1 min
  if(millis()-TxMqttAzimuthTimer > 60000){
    MqttPubString("Azimuth", String(Azimuth), false);
    if(Status==0){
      MqttPubString("AzimuthStop", String(Azimuth), false);
    }
    TxMqttAzimuthTimer=millis();
  }
  PersistPulseAzimuthIfNeeded(false);

  static long PwmTelemetryTimer = 0;
  static float PwmSlowWindowPrev = -1.0;
  static float StopTolerancePrev = -1.0;
  if(millis()-PwmTelemetryTimer > 5000){
    float stopToleranceDeg = GetStopToleranceDeg();
    if(abs(PwmSlowWindowPrev-PwmSlowWindowDeg) > 0.09 || PwmSlowWindowPrev < 0){
      MqttPubString("PwmSlowWindowDeg", String(PwmSlowWindowDeg, 1), false);
      PwmSlowWindowPrev = PwmSlowWindowDeg;
    }
    if(abs(StopTolerancePrev-stopToleranceDeg) > 0.04 || StopTolerancePrev < 0){
      MqttPubString("StopToleranceDeg", String(stopToleranceDeg, 2), false);
      StopTolerancePrev = stopToleranceDeg;
    }
    PwmTelemetryTimer=millis();
  }

  if(Status!=0){
    // status watchdog timeout
    if(millis()-StatusWatchdogTimer > 120000){ // after 90sec
      if(IsManualStatusValue(Status)){
        RequestManualStopRamp();
      }else if(Status<0){
        Status=-3;
      }else{
        Status=3;
      }
      ErrorDetect=1;
      MqttPubString("Debug", "Stopped by 120s timeout", false);
      StatusWatchdogTimer = StatusWatchdogTimer+5000; // next 5sec check
    }
    // Azimuth change watchdog
    if(millis()-RotateWatchdogTimer > 10000){    // check every 10 sec
      if(abs(AzimuthWatchdog-Azimuth)> 360/(OneTurnLimitSec/10) ){ // OneTurnLimitSec 90 = 360/90/10
        RotateWatchdogTimer=millis();
        AzimuthWatchdog=Azimuth;
      }else{
        if(IsManualStatusValue(Status)){
          RequestManualStopRamp();
        }else if(Status<0){
          Status=-3;
        }else{
          Status=3;
        }
        ErrorDetect=1;
        MqttPubString("Debug", "Stopped after not reaching "+String(360/(OneTurnLimitSec/10) )+"° change in 10s", false);
        RotateWatchdogTimer=millis();
      }
    }
  }

  //POE voltage check
  static long DCunderVoltageWatchdog = 0;
  if(millis()-DCunderVoltageWatchdog > 1000){
    if(Status==1 || Status==2 || Status==-1 || Status==-2 || Status==21 || Status==22 || Status==-21 || Status==-22){
        if(VoltageValue < VoltageLimit){
          if(IsManualStatusValue(Status)){
            RequestManualStopRamp();
          }else if(Status<0){
            Status=-3;
          }else{
            Status=3;
          }
          MqttPubString("Debug", "Stopped by under voltage 11,5V POE", false);
          ErrorDetect=1;
        }
    }
    DCunderVoltageWatchdog=millis();
  }

  // debug
  static long DebugTimer = 0;
  if(millis()-DebugTimer > 2000){
      // MqttPubString("AzimuthRAW", String(analogRead(AzimuthPin)), false);
      // MqttPubString("MaxRotateDegree", String(MaxRotateDegree), false);
      // MqttPubString("AzimuthRAWcal", String(AzimuthValue), false);
      // MqttPubString("MaxLimitDegree", String(MaxLimitDegree), false);
      // MqttPubString("AzimuthValue", String(AzimuthValue/1000.0), false);
      // MqttPubString("CwCcwInputValue", String(CwCcwInputValue/1000.0), false);
      // MqttPubString("HWidValue", String(HWidValue/1000.0), false);

      DebugTimer=millis();
  }

  // WDT
  if(millis()-WdtTimer > 60000){
    esp_task_wdt_reset();
    WdtTimer=millis();
    if(EnableSerialDebug>0){
      Prn(3, 0,"WDT reset ");
      Prn(3, 1, UtcTime(1));
    }
  }

  // AZ master potentiometer - must ne on end of Watchdog() because RunByStatus() must be next step
  // static int AZtargetTmp = 0;
  static bool Run = false;
  static long AZmasterChangeTimer = 0;
  static int AZmasterTmp = AZmaster;
  if(millis()<10000 || Status!=0){
    AZmasterTmp=AZmaster;
  }
  if( Status==0 && abs(AZmaster-AZmasterTmp)>3 && Run==false){
    AZmasterTmp=AZmaster;
    Run = true;
    UiTargetAzimuth = AZmaster;
    AZmasterChangeTimer=millis();
    // MqttPubString("AZmasterStart", String(AZmaster), false);
  }
  if( Status==0 && abs(AZmaster-AZmasterTmp)>3 && Run==true){
    AZmasterChangeTimer=millis();
    AZmasterTmp=AZmaster;
    UiTargetAzimuth = AZmaster;
    // MqttPubString("AZmasterRun", String(AZmaster), false);
  }
  if( Status==0 && millis()-AZmasterChangeTimer >2000 && abs(AZmaster-AZmasterTmp)<=3 && Run==true){
    AZmasterTmp=AZmaster;
    UiTargetAzimuth = AZmaster;
    AzimuthTarget = AZmaster - StartAzimuth;
    if(AzimuthTarget<0){
        AzimuthTarget = 360+AzimuthTarget;
    }
    RotCalculate();
    Run = false;
    MqttPubString("AzimuthTargetPot", String(AzimuthTarget), false);
    // MqttPubString("AZmasterStop", String(AZmaster), false);
  }
}

//-------------------------------------------------------------------------------------------------------

// LED
void LedStatus(){
  switch (Status) {
    case -23: {
      digitalWrite(LedRPin, HIGH);
      ledcWrite(greenPWMChannel, 0);
      break; }
    case -22: {
      digitalWrite(LedRPin, HIGH);
      ledcWrite(greenPWMChannel, 0);
      break; }
    case -21: {
      digitalWrite(LedRPin, HIGH);
      ledcWrite(greenPWMChannel, 0);
      break; }
    case -3: {
      digitalWrite(LedRPin, HIGH);
      ledcWrite(greenPWMChannel, 0);
      break; }
    case -2: {
      digitalWrite(LedRPin, HIGH);
      ledcWrite(greenPWMChannel, 0);
      break; }
    case -1: {
      digitalWrite(LedRPin, HIGH);
      ledcWrite(greenPWMChannel, 0);
      break; }
    case  0: {
      digitalWrite(LedRPin, LOW);
      digitalWrite(LedBPin, LOW);
      break; }
    case  1: {
      digitalWrite(LedRPin, HIGH);
      ledcWrite(greenPWMChannel, 0);
      break; }
    case  21: {
      digitalWrite(LedRPin, HIGH);
      ledcWrite(greenPWMChannel, 0);
      break; }
    case  22: {
      digitalWrite(LedRPin, HIGH);
      ledcWrite(greenPWMChannel, 0);
      break; }
    case  23: {
      digitalWrite(LedRPin, HIGH);
      ledcWrite(greenPWMChannel, 0);
      break; }
    case  2: {
      digitalWrite(LedRPin, HIGH);
      ledcWrite(greenPWMChannel, 0);
      break; }
    case  3: {
      digitalWrite(LedRPin, HIGH);
      ledcWrite(greenPWMChannel, 0);
      break; }
  }
}
//-------------------------------------------------------------------------------------------------------
void LedStatusErr(){
  static byte dutyCycleLed = 0;
  static bool dutyCycleLedDown = 0;
  if(ErrorDetect==1){
    if(dutyCycleLedDown==0){
      dutyCycleLed++;
      if(dutyCycleLed==255){
        dutyCycleLedDown=1;
      }
    }else{
      dutyCycleLed--;
      if(dutyCycleLed==0){
        dutyCycleLedDown=0;
      }
    }
    ledcWrite(greenPWMChannel, dutyCycleLed);
  }else{

    ledcWrite(greenPWMChannel, 255);
  }
}

//-------------------------------------------------------------------------------------------------------

void RotCalculate(){
  if(VoltageValue < VoltageLimit){
    AzimuthTarget=-1;
    UiTargetAzimuth=-1;
    MqttPubString("Debug", "Target ignore by under voltage 11,5V POE", false);
  }else{
    // overlap detect
    if(Azimuth < MaxRotateDegree/2 && AzimuthTarget < MaxRotateDegree/2){ // MaxRotateDegree/2 = HalfPoint
      //CCW
    }else{
      if(MaxRotateDegree>360 && AzimuthTarget<MaxRotateDegree-360){  // if in overlap
        AzimuthTarget=AzimuthTarget+360;      // go to overlap
      }
      //CW
    }

    // direction
    if(AzimuthTarget>=0 && AzimuthTarget <=MaxRotateDegree){
      if(AzimuthTarget>Azimuth){
        Status=1; //digitalWrite(BrakePin, HIGH); delay(24);
        // RunTimer();
      }else{
        Status=-1; //digitalWrite(BrakePin, HIGH); delay(24);
        // RunTimer();
      }

    // escape from the forbidden zone
    }else if(Azimuth<0 && AzimuthTarget>0){
      Status=1; //digitalWrite(BrakePin, HIGH); delay(24);
      // RunTimer();
    }else if(Azimuth>MaxRotateDegree && AzimuthTarget<MaxRotateDegree){
      Status=-1; //digitalWrite(BrakePin, HIGH); delay(24);
      // RunTimer();
    }else{
      AzimuthTarget=-1;
      UiTargetAzimuth=-1;
    }
  }
}

//-------------------------------------------------------------------------------------------------------
void RunTimer(){
  StatusWatchdogTimer = millis();
  RotateWatchdogTimer = millis();
  AzimuthWatchdog=Azimuth;
}

//-------------------------------------------------------------------------------------------------------
void DetectEndstopZone(){
  if(Endstop==false){
    if(Status==-1 || Status==-11 || Status==-2 || Status==-21 || Status==-22 || Status==-23){  // run status CCW
      if(CcwRaw<CwRaw){ // standard az potentiometer
        if(AzimuthValue/1000>NoEndstopLowZone){
          // run
        }else{
          // MqttPubString("Debug", "2|"+String(Endstop)+"|"+String(AzimuthValue/1000)+"<"+String(NoEndstopHighZone), false);
          Status=-3;
        }
      }else{ // reverse az potentiometer value
        if(AzimuthValue/1000<NoEndstopHighZone){
          // run
        }else{
          // MqttPubString("Debug", "4|"+String(Endstop)+"|"+String(AzimuthValue/1000)+"<"+String(NoEndstopHighZone), false);
          Status=-3;
        }
      }
    }
    if(Status==1 || Status==11 || Status==2 || Status==21 || Status==22 || Status==23){  // run status CW
      if(CcwRaw<CwRaw){ // standard az potentiometer
        if(AzimuthValue/1000<NoEndstopHighZone){
          // run
        }else{
          // MqttPubString("Debug", "6|"+String(Endstop)+"|"+String(AzimuthValue/1000)+"<"+String(NoEndstopHighZone), false);
          Status=3;
        }
      }else{ // reverse az potentiometer value
        if(AzimuthValue/1000>NoEndstopLowZone){
          // run
        }else{
          // MqttPubString("Debug", "8|"+String(Endstop)+"|"+String(AzimuthValue/1000)+"<"+String(NoEndstopHighZone), false);
          Status=3;
        }
      }
    }
  }
}
//-------------------------------------------------------------------------------------------------------

void EthTest(){
  if(Status != 0 && eth_connected == false){
    Status=0;
  }   
}
//-------------------------------------------------------------------------------------------------------

float ClampFloat(float value, float minValue, float maxValue){
  if(value < minValue){
    return minValue;
  }
  if(value > maxValue){
    return maxValue;
  }
  return value;
}

byte ClampDuty(int value){
  if(value < 0){
    return 0;
  }
  if(value > 255){
    return 255;
  }
  return byte(value);
}

float ReadControlAzimuthDeg(){
  if(AZsource == 0){
    return AzimuthControlDeg;
  }
  return float(Azimuth);
}

float ReadRawAzimuthDeg(){
  if(AZsource == 0){
    return AzimuthRawDeg;
  }
  return float(Azimuth);
}

float ReadRemainingDistanceDeg(int directionSign, float azimuthDeg){
  if(AzimuthTarget < 0){
    return 0.0;
  }
  if(directionSign > 0){
    return float(AzimuthTarget) - azimuthDeg;
  }
  return azimuthDeg - float(AzimuthTarget);
}

float GetStopToleranceDeg(){
  if(AZsource != 0){
    return 0.6;
  }
  float tolerance = 0.35 + AzimuthNoiseDeg * 2.2;
  return ClampFloat(tolerance, 0.45, 1.6);
}

bool TargetReachedInDirection(int directionSign){
  return ReadRemainingDistanceDeg(directionSign, ReadRawAzimuthDeg()) <= GetStopToleranceDeg();
}

float GetPwmTuneAggressionFactor(){
  switch (PwmTuneAggressiveness) {
    case 0: { return 0.95f; }
    case 1: { return 1.40f; }
    case 2: { return 2.05f; }
    case 3: { return 2.85f; }
    default: { return 3.70f; }
  }
}

float GetPwmTuneLeadOffsetDeg(){
  switch (PwmTuneAggressiveness) {
    case 0: { return 0.00f; }
    case 1: { return 0.20f; }
    case 2: { return 0.45f; }
    case 3: { return 0.90f; }
    default: { return 1.40f; }
  }
}

bool ShouldForceStopFromPwmStall(int directionSign, byte currentDuty, float rawAzimuthDeg, byte maxDuty){
  static int trackedDirection = 0;
  static float stallStartAzimuthDeg = 0.0f;
  static unsigned long stallTimer = 0;

  int lowDutyThreshold = min(int(maxDuty), max(10, int(round(float(maxDuty) * 0.18f))));
  if(currentDuty > lowDutyThreshold){
    trackedDirection = 0;
    stallTimer = 0;
    stallStartAzimuthDeg = rawAzimuthDeg;
    return false;
  }

  if(trackedDirection != directionSign || stallTimer == 0){
    trackedDirection = directionSign;
    stallStartAzimuthDeg = rawAzimuthDeg;
    stallTimer = millis();
    return false;
  }

  if(abs(rawAzimuthDeg - stallStartAzimuthDeg) > 0.18f){
    stallStartAzimuthDeg = rawAzimuthDeg;
    stallTimer = millis();
    return false;
  }

  return millis() - stallTimer > 2500;
}

byte ComputeClosedLoopDuty(int directionSign){
  const byte pwmMinStart = 64;
  const byte pwmMinRun = 40;
  float stopToleranceDeg = GetStopToleranceDeg();
  float slowWindowDeg = max(PwmSlowWindowDeg, stopToleranceDeg + 1.0f);

  float remainingDeg = ReadRemainingDistanceDeg(directionSign, ReadControlAzimuthDeg()) - GetPwmTuneLeadOffsetDeg();
  if(remainingDeg <= stopToleranceDeg){
    return 0;
  }

  float normalized = (remainingDeg - stopToleranceDeg) / (slowWindowDeg - stopToleranceDeg);
  normalized = ClampFloat(normalized, 0.0, 1.0);

  float aggression = GetPwmTuneAggressionFactor();
  int maxDuty = int(PwmMaxDuty);
  float currentDutyRatio = (maxDuty > 0) ? ClampFloat(float(dutyCycle) / float(maxDuty), 0.0, 1.0) : 0.0;
  float brakeWindowShape = (0.24f + 0.76f * currentDutyRatio * currentDutyRatio) * (0.96f + aggression * 0.16f);
  float dynamicBrakeWindow = stopToleranceDeg + (slowWindowDeg - stopToleranceDeg) * ClampFloat(brakeWindowShape, 0.0, 1.30);
  float brakingUrgency = 0.0f;
  if(dynamicBrakeWindow > stopToleranceDeg){
    brakingUrgency = (dynamicBrakeWindow - remainingDeg) / (dynamicBrakeWindow - stopToleranceDeg);
    brakingUrgency = ClampFloat(brakingUrgency, 0.0, 1.0);
  }

  // When the current speed is too high for the remaining angle, bend the curve down
  // and allow a lower holding PWM so the stop ramp reacts sooner.
  normalized = powf(normalized, 1.0f + brakingUrgency * (1.35f + aggression * 0.95f));

  float minDutyReduction = ClampFloat(0.50f + aggression * 0.18f, 0.50f, 0.88f);
  int minDuty = min(maxDuty, max(8, int(round(float(pwmMinRun) * (1.0f - brakingUrgency * minDutyReduction)))));
  int targetDuty = minDuty + int((maxDuty - minDuty) * normalized);

  if(dutyCycle == 0 && targetDuty > 0){
    targetDuty = max(targetDuty, min(maxDuty, int(pwmMinStart)));
  }

  return ClampDuty(targetDuty);
}

void ApplyDutySlew(byte targetDuty, long &PwmTimer){
  unsigned int slewInterval = max(1u, PwmRampSteps);
  if(millis()-PwmTimer < slewInterval){
    return;
  }

  int nextDuty = dutyCycle;
  if(nextDuty < targetDuty){
    nextDuty += 10;
    if(nextDuty > targetDuty){
      nextDuty = targetDuty;
    }
  }else if(nextDuty > targetDuty){
    nextDuty -= 10;
    if(nextDuty < targetDuty){
      nextDuty = targetDuty;
    }
  }

  dutyCycle = ClampDuty(nextDuty);
  ledcWrite(mosfetPWMChannel, dutyCycle);
  PwmTimer=millis();
}

String FormatPwmTotalRampTime(unsigned int stepIntervalMs, byte maxDuty){
  unsigned int dutySteps = max(1, (int(maxDuty) + 9) / 10);
  unsigned long normalizedStepIntervalMs = (stepIntervalMs == 0) ? 1UL : static_cast<unsigned long>(stepIntervalMs);
  unsigned long totalRampMs = normalizedStepIntervalMs * static_cast<unsigned long>(dutySteps);

  if(totalRampMs >= 1000){
    return String(float(totalRampMs) / 1000.0f, 2) + " s total";
  }
  return String(totalRampMs) + " ms total";
}

bool IsSafeConfigChar(byte value){
  return value >= 32 && value <= 126;
}

bool IsSafeWebAuthPasswordValue(const String& value){
  for(size_t i = 0; i < value.length(); i++){
    char c = value[i];
    if(c == '"' || c == '\'' || c == '<' || c == '>' || c == '&'){
      return false;
    }
  }
  return true;
}

bool LooksLikeLegacyWebAuthKey(const String& value){
  if(value.length() != 100){
    return false;
  }
  for(size_t i = 0; i < value.length(); i++){
    char c = value[i];
    bool isDigit = (c >= '0' && c <= '9');
    bool isUpper = (c >= 'A' && c <= 'Z');
    bool isLower = (c >= 'a' && c <= 'z');
    if(!(isDigit || isUpper || isLower)){
      return false;
    }
  }
  return true;
}

String ReadFixedStringFromEeprom(int start, int length){
  String value = "";
  value.reserve(length);
  for(int i = 0; i < length; i++){
    byte raw = EEPROM.read(start + i);
    if(raw == 0xff || raw == 0x00){
      break;
    }
    if(IsSafeConfigChar(raw)){
      value += char(raw);
    }
  }
  return value;
}

void WriteFixedStringToEeprom(int start, int length, const String& text){
  for(int i = 0; i < length; i++){
    EEPROM.write(start + i, (i < text.length()) ? text[i] : 0xff);
  }
}

String JsonEscape(const String& value){
  String escaped = "";
  escaped.reserve(value.length() + 8);
  for(size_t i = 0; i < value.length(); i++){
    unsigned char c = static_cast<unsigned char>(value[i]);
    switch(c){
      case '\\': escaped += "\\\\"; break;
      case '"': escaped += "\\\""; break;
      case '\n': escaped += "\\n"; break;
      case '\r': escaped += "\\r"; break;
      case '\t': escaped += "\\t"; break;
      default:
        if(c < 0x20){
          const char* hex = "0123456789ABCDEF";
          escaped += "\\u00";
          escaped += hex[(c >> 4) & 0x0F];
          escaped += hex[c & 0x0F];
        }else{
          escaped += char(c);
        }
        break;
    }
  }
  return escaped;
}

static int FindJsonValueStart(const String& json, const char* key){
  String needle = String("\"") + key + "\"";
  int keyPos = json.indexOf(needle);
  if(keyPos < 0){
    return -1;
  }
  int colonPos = json.indexOf(':', keyPos + needle.length());
  if(colonPos < 0){
    return -1;
  }
  int valuePos = colonPos + 1;
  while(valuePos < json.length() && isspace(static_cast<unsigned char>(json[valuePos]))){
    valuePos++;
  }
  if(valuePos >= json.length()){
    return -1;
  }
  return valuePos;
}

bool JsonExtractString(const String& json, const char* key, String& value){
  int valuePos = FindJsonValueStart(json, key);
  if(valuePos < 0 || json[valuePos] != '"'){
    return false;
  }
  value = "";
  for(int i = valuePos + 1; i < json.length(); i++){
    char c = json[i];
    if(c == '\\'){
      if(i + 1 >= json.length()){
        return false;
      }
      char next = json[++i];
      switch(next){
        case '"': value += '"'; break;
        case '\\': value += '\\'; break;
        case 'n': value += '\n'; break;
        case 'r': value += '\r'; break;
        case 't': value += '\t'; break;
        default: value += next; break;
      }
    }else if(c == '"'){
      return true;
    }else{
      value += c;
    }
  }
  return false;
}

bool JsonExtractLong(const String& json, const char* key, long& value){
  int valuePos = FindJsonValueStart(json, key);
  if(valuePos < 0){
    return false;
  }
  int endPos = valuePos;
  while(endPos < json.length()){
    char c = json[endPos];
    if((c >= '0' && c <= '9') || c == '-' || c == '+'){
      endPos++;
    }else{
      break;
    }
  }
  if(endPos == valuePos){
    return false;
  }
  value = json.substring(valuePos, endPos).toInt();
  return true;
}

bool JsonExtractFloat(const String& json, const char* key, float& value){
  int valuePos = FindJsonValueStart(json, key);
  if(valuePos < 0){
    return false;
  }
  int endPos = valuePos;
  while(endPos < json.length()){
    char c = json[endPos];
    if((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.'){
      endPos++;
    }else{
      break;
    }
  }
  if(endPos == valuePos){
    return false;
  }
  value = json.substring(valuePos, endPos).toFloat();
  return true;
}

bool JsonExtractBool(const String& json, const char* key, bool& value){
  int valuePos = FindJsonValueStart(json, key);
  if(valuePos < 0){
    return false;
  }
  if(json.startsWith("true", valuePos)){
    value = true;
    return true;
  }
  if(json.startsWith("false", valuePos)){
    value = false;
    return true;
  }
  return false;
}

String ExportConfigBackupJson(){
  String json = "{\n";
  json += "  \"format\": \"" CONFIG_BACKUP_FORMAT "\",\n";
  json += "  \"version\": " + String(CONFIG_BACKUP_VERSION) + ",\n";
  json += "  \"fw_rev\": \"" + JsonEscape(String(REV)) + "\",\n";
  json += "  \"config\": {\n";
  json += "    \"net_id\": \"" + JsonEscape(NET_ID) + "\",\n";
  json += "    \"rot_name\": \"" + JsonEscape(RotName) + "\",\n";
  json += "    \"your_call\": \"" + JsonEscape(YOUR_CALL) + "\",\n";
  json += "    \"start_azimuth\": " + String(StartAzimuth) + ",\n";
  json += "    \"max_rotate_degree\": " + String(MaxRotateDegree) + ",\n";
  json += "    \"ant_radiation_angle\": " + String(AntRadiationAngle) + ",\n";
  json += "    \"endstop\": " + String(Endstop ? "true" : "false") + ",\n";
  json += "    \"ac_motor\": " + String(ACmotor ? "true" : "false") + ",\n";
  json += "    \"ccw_raw\": " + String(CcwRaw) + ",\n";
  json += "    \"cw_raw\": " + String(CwRaw) + ",\n";
  json += "    \"reverse\": " + String(Reverse ? "true" : "false") + ",\n";
  json += "    \"no_endstop_low_zone\": " + String(NoEndstopLowZone, 1) + ",\n";
  json += "    \"no_endstop_high_zone\": " + String(NoEndstopHighZone, 1) + ",\n";
  json += "    \"az_source\": " + String(AZsource) + ",\n";
  json += "    \"pulse_per_degree\": " + String(PulsePerDegree) + ",\n";
  json += "    \"baud_rate\": " + String(BaudRate) + ",\n";
  json += "    \"az_two_wire\": " + String(AZtwoWire ? "true" : "false") + ",\n";
  json += "    \"az_preamp\": " + String(AZpreamp ? "true" : "false") + ",\n";
  json += "    \"reverse_az\": " + String(ReverseAZ ? "true" : "false") + ",\n";
  json += "    \"web_auth_enabled\": " + String(WebAuthEnabled ? "true" : "false") + ",\n";
  json += "    \"web_auth_password\": \"" + JsonEscape(WebAuthPassword) + "\",\n";
  json += "    \"pwm_enable\": " + String(PWMenable ? "true" : "false") + ",\n";
  json += "    \"pwm_ramp_steps\": " + String(PwmRampSteps) + ",\n";
  json += "    \"pwm_max_duty\": " + String(PwmMaxDuty) + ",\n";
  json += "    \"pwm_slow_window_deg\": " + String(PwmSlowWindowDeg, 1) + ",\n";
  json += "    \"pwm_tune_aggressiveness\": " + String(PwmTuneAggressiveness) + ",\n";
  json += "    \"mqtt_ip\": \"" + String(mqtt_server_ip[0]) + "." + String(mqtt_server_ip[1]) + "." + String(mqtt_server_ip[2]) + "." + String(mqtt_server_ip[3]) + "\",\n";
  json += "    \"mqtt_port\": " + String(MQTT_PORT) + ",\n";
  json += "    \"mqtt_login\": " + String(MQTT_LOGIN ? "true" : "false") + ",\n";
  json += "    \"mqtt_user\": \"" + JsonEscape(MQTT_USER) + "\",\n";
  json += "    \"mqtt_pass\": \"" + JsonEscape(MQTT_PASS) + "\",\n";
  json += "    \"dxc_host\": \"" + JsonEscape(DxcHost) + "\",\n";
  json += "    \"dxc_port\": " + String(DxcPort) + ",\n";
  json += "    \"dxc_callsign\": \"" + JsonEscape(DxcCallsign) + "\",\n";
  json += "    \"elevation\": " + String(ELEVATION ? "true" : "false") + ",\n";
  json += "    \"map_url\": \"" + JsonEscape(MapUrl) + "\",\n";
  json += "    \"map_source\": " + String(MapSource) + ",\n";
  json += "    \"map_locator\": \"" + JsonEscape(MapLocator) + "\",\n";
  json += "    \"map_zoom_km\": " + String(MapZoomKm) + ",\n";
  json += "    \"grayline_ntp_server\": \"" + JsonEscape(GraylineNtpServer) + "\",\n";
  json += "    \"grayline_darkness\": " + String(GraylineDarkness) + ",\n";
  json += "    \"map_theme\": " + String(MapTheme) + ",\n";
  json += "    \"default_deg_overlay\": " + String(DefaultDegOverlayEnabled ? "true" : "false") + ",\n";
  json += "    \"default_ant_overlay\": " + String(DefaultAntOverlayEnabled ? "true" : "false") + ",\n";
  json += "    \"default_loc_grid\": " + String(DefaultMapLocGridEnabled ? "true" : "false") + ",\n";
  json += "    \"default_grayline\": " + String(DefaultMapGraylineEnabled ? "true" : "false") + ",\n";
  json += "    \"default_state_borders\": " + String(DefaultMapBordersEnabled ? "true" : "false") + ",\n";
  json += "    \"default_dxcc_prefixes\": " + String(DefaultMapDxccEnabled ? "true" : "false") + ",\n";
  json += "    \"one_turn_limit_sec\": " + String(OneTurnLimitSec) + "\n";
  json += "  }\n";
  json += "}\n";
  return json;
}

String ImportConfigBackupJson(const String& jsonPayload){
  String format;
  long version = 0;
  if(!JsonExtractString(jsonPayload, "format", format) || format != CONFIG_BACKUP_FORMAT){
    return "Invalid backup format";
  }
  if(!JsonExtractLong(jsonPayload, "version", version) || version != CONFIG_BACKUP_VERSION){
    return "Unsupported backup version";
  }

  String newNetId, newRotName, newYourCall, newMapUrl, newMqttUser, newMqttPass, newMapLocator, newGraylineNtp, mqttIpText, newDxcHost, newDxcCallsign, newWebAuthPassword;
  long startAzimuthValue = 0, maxRotateValue = 0, antAngleValue = 0, ccwRawValue = 0, cwRawValue = 0;
  long azSourceValue = 0, pulsePerDegreeValue = 0, baudRateValue = 0, pwmRampStepsValue = 0, pwmMaxDutyValue = 0;
  long pwmTuneValue = 0, mqttPortValue = 0, dxcPortValue = 0, mapSourceValue = 0, mapZoomValue = 0, graylineDarknessValue = 0, mapThemeValue = 0, oneTurnLimitValue = 0;
  float lowZoneValue = 0.0f, highZoneValue = 0.0f, pwmSlowWindowValue = 0.0f;
  bool endstopValue = false, acMotorValue = false, reverseValue = false, azTwoWireValue = false, azPreampValue = false;
  bool reverseAzValue = false, webAuthEnabledValue = false, pwmEnableValue = false, mqttLoginValue = false, elevationValue = false;
  bool defaultDegOverlayValue = true, defaultAntOverlayValue = true, defaultLocGridValue = false, defaultGraylineValue = true, defaultStateBordersValue = false, defaultDxccPrefixesValue = false;

  if(!JsonExtractString(jsonPayload, "net_id", newNetId) || newNetId.length() < 1 || newNetId.length() > 2){
    return "Invalid net_id";
  }
  if(!JsonExtractString(jsonPayload, "rot_name", newRotName) || newRotName.length() < 1 || newRotName.length() > 20){
    return "Invalid rot_name";
  }
  if(!JsonExtractString(jsonPayload, "your_call", newYourCall) || newYourCall.length() < 1 || newYourCall.length() > 20){
    return "Invalid your_call";
  }
  if(!JsonExtractLong(jsonPayload, "start_azimuth", startAzimuthValue) || startAzimuthValue < 0 || startAzimuthValue > 359){
    return "Invalid start_azimuth";
  }
  if(!JsonExtractLong(jsonPayload, "max_rotate_degree", maxRotateValue) || maxRotateValue < 0 || maxRotateValue > 719){
    return "Invalid max_rotate_degree";
  }
  if(!JsonExtractLong(jsonPayload, "ant_radiation_angle", antAngleValue) || antAngleValue < 1 || antAngleValue > 180){
    return "Invalid ant_radiation_angle";
  }
  if(!JsonExtractBool(jsonPayload, "endstop", endstopValue)){
    return "Invalid endstop";
  }
  if(!JsonExtractBool(jsonPayload, "ac_motor", acMotorValue)){
    return "Invalid ac_motor";
  }
  if(!JsonExtractLong(jsonPayload, "ccw_raw", ccwRawValue) || ccwRawValue < 0 || ccwRawValue > 4095){
    return "Invalid ccw_raw";
  }
  if(!JsonExtractLong(jsonPayload, "cw_raw", cwRawValue) || cwRawValue < 0 || cwRawValue > 4095){
    return "Invalid cw_raw";
  }
  if(!JsonExtractBool(jsonPayload, "reverse", reverseValue)){
    return "Invalid reverse";
  }
  if(!JsonExtractFloat(jsonPayload, "no_endstop_low_zone", lowZoneValue) || lowZoneValue < 0.2f || lowZoneValue > 1.5f){
    return "Invalid no_endstop_low_zone";
  }
  if(!JsonExtractFloat(jsonPayload, "no_endstop_high_zone", highZoneValue) || highZoneValue < 1.6f || highZoneValue > 3.1f){
    return "Invalid no_endstop_high_zone";
  }
  if(!JsonExtractLong(jsonPayload, "az_source", azSourceValue) || azSourceValue < 0 || azSourceValue > 2){
    return "Invalid az_source";
  }
  if(!JsonExtractLong(jsonPayload, "pulse_per_degree", pulsePerDegreeValue) || pulsePerDegreeValue < 1 || pulsePerDegreeValue > 100){
    return "Invalid pulse_per_degree";
  }
  if(!JsonExtractLong(jsonPayload, "baud_rate", baudRateValue) || (baudRateValue != 1200 && baudRateValue != 2400 && baudRateValue != 4800 && baudRateValue != 9600 && baudRateValue != 115200)){
    return "Invalid baud_rate";
  }
  if(!JsonExtractBool(jsonPayload, "az_two_wire", azTwoWireValue)){
    return "Invalid az_two_wire";
  }
  if(!JsonExtractBool(jsonPayload, "az_preamp", azPreampValue)){
    return "Invalid az_preamp";
  }
  if(!JsonExtractBool(jsonPayload, "reverse_az", reverseAzValue)){
    return "Invalid reverse_az";
  }
  if(!JsonExtractBool(jsonPayload, "web_auth_enabled", webAuthEnabledValue)){
    webAuthEnabledValue = false;
  }
  if(!JsonExtractString(jsonPayload, "web_auth_password", newWebAuthPassword)){
    newWebAuthPassword = WebAuthPassword;
  }
  if(newWebAuthPassword.length() > 100 || !IsSafeWebAuthPasswordValue(newWebAuthPassword)){
    return "Invalid web_auth_password";
  }
  if(webAuthEnabledValue && newWebAuthPassword.length() < 1){
    return "web_auth_password is required when web_auth_enabled is true";
  }
  if(!JsonExtractBool(jsonPayload, "pwm_enable", pwmEnableValue)){
    return "Invalid pwm_enable";
  }
  if(!JsonExtractLong(jsonPayload, "pwm_ramp_steps", pwmRampStepsValue) || pwmRampStepsValue < 1 || pwmRampStepsValue > 200){
    return "Invalid pwm_ramp_steps";
  }
  if(!JsonExtractLong(jsonPayload, "pwm_max_duty", pwmMaxDutyValue) || pwmMaxDutyValue < 40 || pwmMaxDutyValue > 255){
    return "Invalid pwm_max_duty";
  }
  if(!JsonExtractFloat(jsonPayload, "pwm_slow_window_deg", pwmSlowWindowValue) || pwmSlowWindowValue < 3.0f || pwmSlowWindowValue > 60.0f){
    return "Invalid pwm_slow_window_deg";
  }
  if(!JsonExtractLong(jsonPayload, "pwm_tune_aggressiveness", pwmTuneValue) || pwmTuneValue < 0 || pwmTuneValue > 4){
    return "Invalid pwm_tune_aggressiveness";
  }
  if(!JsonExtractString(jsonPayload, "mqtt_ip", mqttIpText)){
    return "Invalid mqtt_ip";
  }
  if(!JsonExtractLong(jsonPayload, "mqtt_port", mqttPortValue) || mqttPortValue < 1 || mqttPortValue > 65535){
    return "Invalid mqtt_port";
  }
  if(!JsonExtractBool(jsonPayload, "mqtt_login", mqttLoginValue)){
    return "Invalid mqtt_login";
  }
  if(!JsonExtractString(jsonPayload, "mqtt_user", newMqttUser) || newMqttUser.length() < 1 || newMqttUser.length() > 10){
    return "Invalid mqtt_user";
  }
  if(!JsonExtractString(jsonPayload, "mqtt_pass", newMqttPass) || newMqttPass.length() < 1 || newMqttPass.length() > 20){
    return "Invalid mqtt_pass";
  }
  if(!JsonExtractString(jsonPayload, "dxc_host", newDxcHost) || newDxcHost.length() > 63){
    return "Invalid dxc_host";
  }
  if(!JsonExtractLong(jsonPayload, "dxc_port", dxcPortValue) || dxcPortValue < 1 || dxcPortValue > 65535){
    return "Invalid dxc_port";
  }
  if(!JsonExtractString(jsonPayload, "dxc_callsign", newDxcCallsign) || newDxcCallsign.length() > 16){
    return "Invalid dxc_callsign";
  }
  newDxcHost.trim();
  newDxcCallsign.trim();
  newDxcCallsign.toUpperCase();
  if(newDxcHost.length() > 0 && newDxcCallsign.length() < 1){
    return "DXC callsign is required when dxc_host is set";
  }
  if(!JsonExtractBool(jsonPayload, "elevation", elevationValue)){
    return "Invalid elevation";
  }
  if(!JsonExtractString(jsonPayload, "map_url", newMapUrl) || newMapUrl.length() < 1 || newMapUrl.length() > 50){
    return "Invalid map_url";
  }
  if(!JsonExtractLong(jsonPayload, "map_source", mapSourceValue) || mapSourceValue < 0 || mapSourceValue > 1){
    return "Invalid map_source";
  }
  if(!JsonExtractString(jsonPayload, "map_locator", newMapLocator)){
    return "Invalid map_locator";
  }
  newMapLocator.trim();
  newMapLocator.toUpperCase();
  if(newMapLocator.length()!=6 || newMapLocator[0]<'A' || newMapLocator[0]>'R' || newMapLocator[1]<'A' || newMapLocator[1]>'R' || newMapLocator[2]<'0' || newMapLocator[2]>'9' || newMapLocator[3]<'0' || newMapLocator[3]>'9' || newMapLocator[4]<'A' || newMapLocator[4]>'X' || newMapLocator[5]<'A' || newMapLocator[5]>'X'){
    return "Invalid map_locator";
  }
  if(!JsonExtractLong(jsonPayload, "map_zoom_km", mapZoomValue) || mapZoomValue < 1000 || mapZoomValue > 20000){
    return "Invalid map_zoom_km";
  }
  if(!JsonExtractString(jsonPayload, "grayline_ntp_server", newGraylineNtp) || newGraylineNtp.length() < 1 || newGraylineNtp.length() > 50){
    return "Invalid grayline_ntp_server";
  }
  if(!JsonExtractLong(jsonPayload, "grayline_darkness", graylineDarknessValue) || graylineDarknessValue < 0 || graylineDarknessValue > 100){
    return "Invalid grayline_darkness";
  }
  if(!JsonExtractLong(jsonPayload, "map_theme", mapThemeValue) || mapThemeValue < 0 || mapThemeValue > 5){
    return "Invalid map_theme";
  }
  if(!JsonExtractBool(jsonPayload, "default_deg_overlay", defaultDegOverlayValue)){
    defaultDegOverlayValue = true;
  }
  if(!JsonExtractBool(jsonPayload, "default_ant_overlay", defaultAntOverlayValue)){
    defaultAntOverlayValue = true;
  }
  if(!JsonExtractBool(jsonPayload, "default_loc_grid", defaultLocGridValue)){
    defaultLocGridValue = false;
  }
  if(!JsonExtractBool(jsonPayload, "default_grayline", defaultGraylineValue)){
    defaultGraylineValue = true;
  }
  if(!JsonExtractBool(jsonPayload, "default_state_borders", defaultStateBordersValue)){
    defaultStateBordersValue = false;
  }
  if(!JsonExtractBool(jsonPayload, "default_dxcc_prefixes", defaultDxccPrefixesValue)){
    defaultDxccPrefixesValue = false;
  }
  if(!JsonExtractLong(jsonPayload, "one_turn_limit_sec", oneTurnLimitValue) || oneTurnLimitValue < 20 || oneTurnLimitValue > 600){
    return "Invalid one_turn_limit_sec";
  }

  int mqttIp0 = -1, mqttIp1 = -1, mqttIp2 = -1, mqttIp3 = -1;
  if(sscanf(mqttIpText.c_str(), "%d.%d.%d.%d", &mqttIp0, &mqttIp1, &mqttIp2, &mqttIp3) != 4){
    return "Invalid mqtt_ip";
  }
  if(mqttIp0 < 0 || mqttIp0 > 255 || mqttIp1 < 0 || mqttIp1 > 255 || mqttIp2 < 0 || mqttIp2 > 255 || mqttIp3 < 0 || mqttIp3 > 255){
    return "Invalid mqtt_ip";
  }

  if(elevationValue && maxRotateValue > 180){
    return "Elevation backup requires max_rotate_degree in range 0-180";
  }

  int lowZoneTenths = int(lowZoneValue * 10.0f + 0.5f);
  int highZoneTenths = int(highZoneValue * 10.0f + 0.5f);
  int pwmSlowWindowTenths = int(pwmSlowWindowValue * 10.0f + 0.5f);

  YOUR_CALL = newYourCall;
  NET_ID = newNetId;
  RotName = newRotName;
  StartAzimuth = startAzimuthValue;
  MaxRotateDegree = maxRotateValue;
  AntRadiationAngle = antAngleValue;
  Endstop = ((azSourceValue == 1 || azSourceValue == 2) ? true : endstopValue);
  ACmotor = acMotorValue;
  CcwRaw = ccwRawValue;
  CwRaw = cwRawValue;
  Reverse = reverseValue;
  NoEndstopLowZone = float(lowZoneTenths) / 10.0f;
  NoEndstopHighZone = float(highZoneTenths) / 10.0f;
  AZsource = byte(azSourceValue);
  PulsePerDegree = pulsePerDegreeValue;
  BaudRate = baudRateValue;
  AZtwoWire = azTwoWireValue;
  AZpreamp = azPreampValue;
  ReverseAZ = reverseAzValue;
  WebAuthEnabled = webAuthEnabledValue;
  WebAuthPassword = newWebAuthPassword;
  PWMenable = pwmEnableValue && !ACmotor;
  PwmRampSteps = pwmRampStepsValue;
  PwmMaxDuty = pwmMaxDutyValue;
  PwmSlowWindowDeg = float(pwmSlowWindowTenths) / 10.0f;
  PwmTuneAggressiveness = pwmTuneValue;
  mqtt_server_ip[0] = byte(mqttIp0);
  mqtt_server_ip[1] = byte(mqttIp1);
  mqtt_server_ip[2] = byte(mqttIp2);
  mqtt_server_ip[3] = byte(mqttIp3);
  MQTT_PORT = mqttPortValue;
  MQTT_LOGIN = mqttLoginValue;
  MQTT_USER = newMqttUser;
  MQTT_PASS = newMqttPass;
  DxcHost = newDxcHost;
  DxcPort = uint16_t(dxcPortValue);
  DxcCallsign = newDxcCallsign;
  ELEVATION = elevationValue;
  MapUrl = newMapUrl;
  MapSource = byte(mapSourceValue);
  MapLocator = newMapLocator;
  MapZoomKm = mapZoomValue;
  GraylineNtpServer = newGraylineNtp;
  GraylineDarkness = byte(graylineDarknessValue);
  MapTheme = byte(mapThemeValue);
  DefaultDegOverlayEnabled = defaultDegOverlayValue;
  DefaultAntOverlayEnabled = defaultAntOverlayValue;
  DefaultMapLocGridEnabled = defaultLocGridValue;
  DefaultMapGraylineEnabled = defaultGraylineValue;
  DefaultMapBordersEnabled = defaultStateBordersValue;
  DefaultMapDxccEnabled = defaultDxccPrefixesValue;
  OneTurnLimitSec = oneTurnLimitValue;

  WriteFixedStringToEeprom(0, 2, NET_ID);
  WriteFixedStringToEeprom(2, 20, RotName);
  EEPROM.writeUShort(23, StartAzimuth);
  EEPROM.writeUShort(25, MaxRotateDegree);
  EEPROM.writeUShort(27, AntRadiationAngle);
  EEPROM.writeBool(29, Endstop);
  EEPROM.writeBool(30, ACmotor);
  EEPROM.writeUShort(31, CcwRaw);
  EEPROM.writeUShort(33, CwRaw);
  EEPROM.writeBool(35, Reverse);
  EEPROM.writeByte(36, lowZoneTenths);
  WriteFixedStringToEeprom(141, 20, YOUR_CALL);
  EEPROM.writeByte(161, mqtt_server_ip[0]);
  EEPROM.writeByte(162, mqtt_server_ip[1]);
  EEPROM.writeByte(163, mqtt_server_ip[2]);
  EEPROM.writeByte(164, mqtt_server_ip[3]);
  EEPROM.writeUShort(165, MQTT_PORT);
  EEPROM.writeBool(167, ELEVATION);
  EEPROM.writeBool(168, MQTT_LOGIN);
  WriteFixedStringToEeprom(169, 50, MapUrl);
  EEPROM.writeUShort(220, OneTurnLimitSec);
  EEPROM.writeByte(222, highZoneTenths);
  EEPROM.writeByte(223, AZsource);
  EEPROM.writeUShort(224, PulsePerDegree);
  EEPROM.writeUShort(226, BaudRate);
  EEPROM.writeBool(228, AZtwoWire);
  EEPROM.writeBool(229, AZpreamp);
  EEPROM.writeBool(230, ReverseAZ);
  EEPROM.writeBool(231, PWMenable);
  EEPROM.writeBool(232, WebAuthEnabled);
  EEPROM.writeByte(233, 0xA5);
  WriteFixedStringToEeprom(41, 100, WebAuthPassword);
  EEPROM.writeUShort(234, PwmRampSteps);
  WriteFixedStringToEeprom(236, 10, MQTT_USER);
  WriteFixedStringToEeprom(246, 20, MQTT_PASS);
  EEPROM.writeByte(266, MapSource);
  for(int i = 0; i < 6; i++){
    EEPROM.write(267 + i, MapLocator[i]);
  }
  EEPROM.writeUShort(273, MapZoomKm);
  WriteFixedStringToEeprom(275, 50, GraylineNtpServer);
  EEPROM.writeByte(325, GraylineDarkness);
  EEPROM.writeByte(326, MapTheme);
  EEPROM.writeByte(327, PwmMaxDuty);
  EEPROM.writeUShort(328, pwmSlowWindowTenths);
  EEPROM.writeByte(330, PwmTuneAggressiveness);
  WriteFixedStringToEeprom(331, 64, DxcHost);
  EEPROM.writeUShort(395, DxcPort);
  WriteFixedStringToEeprom(397, 16, DxcCallsign);
  EEPROM.writeBool(413, DefaultDegOverlayEnabled);
  EEPROM.writeBool(414, DefaultAntOverlayEnabled);
  EEPROM.writeBool(415, DefaultMapLocGridEnabled);
  EEPROM.writeBool(416, DefaultMapGraylineEnabled);
  EEPROM.writeBool(417, DefaultMapBordersEnabled);
  EEPROM.writeBool(418, DefaultMapDxccEnabled);
  EEPROM.commit();

  digitalWrite(AZtwoWirePin, AZtwoWire);
  digitalWrite(AZpreampPin, AZpreamp);
  ApplyAzimuthSourceMode(false);
  ApplyGraylineNtpConfig();
  return "";
}

void MaybeBeginBrakeLearning(int directionSign, bool &brakeLearningActive, int &brakeLearningDirection, float &brakeStartDistance){
  if(brakeLearningActive || AZsource != 0 || PWMenable == false || AzimuthTarget < 0){
    return;
  }

  float stopToleranceDeg = GetStopToleranceDeg();
  float slowWindowDeg = max(PwmSlowWindowDeg, stopToleranceDeg + 1.0f);
  float rawRemainingDeg = ReadRemainingDistanceDeg(directionSign, ReadRawAzimuthDeg());
  if(rawRemainingDeg <= 0.0f || rawRemainingDeg > slowWindowDeg + 0.8f){
    return;
  }

  brakeLearningActive = true;
  brakeLearningDirection = directionSign;
  brakeStartDistance = rawRemainingDeg;
}

void UpdateAutoSlowWindow(float brakeStartDistance, float finalDistance){
  if(AZsource != 0){
    return;
  }
  float reachedDistance = max(0.0f, brakeStartDistance - max(finalDistance, 0.0f));
  float overshoot = max(0.0f, -finalDistance);
  float undershoot = max(0.0f, finalDistance);
  float settleMargin = GetStopToleranceDeg() + 0.5;
  float aggression = GetPwmTuneAggressionFactor();
  float overshootGain = 2.2f + aggression * 1.1f;
  float undershootGain = 0.45f + aggression * 0.30f;
  float recommendedWindow = reachedDistance + overshoot * overshootGain - undershoot * undershootGain + settleMargin;
  recommendedWindow = ClampFloat(recommendedWindow, 3.0, 60.0);
  float windowError = abs(recommendedWindow - PwmSlowWindowDeg);
  float normalizedError = ClampFloat(windowError / 14.0f, 0.0, 1.0);
  float overshootFactor = ClampFloat(overshoot / 8.0f, 0.0, 1.0);
  float residualFactor = ClampFloat(abs(finalDistance) / 3.0f, 0.0, 1.0);
  float adaptiveBlend = 0.16f + aggression * 0.10f + normalizedError * (0.12f + aggression * 0.05f) + normalizedError * normalizedError * 0.18f + residualFactor * (0.07f + aggression * 0.04f) + overshootFactor * (0.10f + aggression * 0.06f);
  adaptiveBlend = ClampFloat(adaptiveBlend, 0.18f, 0.82f);
  PwmSlowWindowDeg = ClampFloat(PwmSlowWindowDeg * (1.0f - adaptiveBlend) + recommendedWindow * adaptiveBlend, 3.0, 60.0);
  PersistAutoSlowWindowIfNeeded();
}

void RequestStopRamp(bool suppressBrakeLearning){
  if(IsManualStatusValue(Status)){
    RequestManualStopRamp();
    return;
  }
  if(Status<0){
    if(suppressBrakeLearning){
      SkipBrakeLearningOnManualStop = true;
    }
    Status=-3;
  }else if(Status>0){
    if(suppressBrakeLearning){
      SkipBrakeLearningOnManualStop = true;
    }
    Status=3;
  }
}

bool IsManualStatusValue(int statusValue){
  return statusValue == -23 || statusValue == -22 || statusValue == -21 || statusValue == 21 || statusValue == 22 || statusValue == 23;
}

bool IsManualPwmKeyControlEnabled(){
  return AZsource == 0 && PWMenable == true && ACmotor == false;
}

void RequestManualStopRamp(){
  if(Status == -21 || Status == -22 || Status == -23){
    Status = -23;
  }else if(Status == 21 || Status == 22 || Status == 23){
    Status = 23;
  }
}

bool IsPulseAzimuthSource(){
  return AZsource == 1;
}

unsigned int ClampPulseAzimuthTenths(int valueTenths){
  if(valueTenths < 0){
    return 0;
  }
  int maxTenths = int(MaxRotateDegree) * 10;
  if(valueTenths > maxTenths){
    return (unsigned int)maxTenths;
  }
  return (unsigned int)valueTenths;
}

long ClampPulseCount(long pulseCount){
  long maxCount = long(MaxRotateDegree) * long(max(PulsePerDegree, short(1)));
  if(pulseCount < 0){
    return 0;
  }
  if(pulseCount > maxCount){
    return maxCount;
  }
  return pulseCount;
}

void ConfigureSinglePulseCounter(pcnt_unit_t unit, int gpioPin){
  pcnt_config_t pulseCounterConfig = {};
  pulseCounterConfig.pulse_gpio_num = gpioPin;
  pulseCounterConfig.ctrl_gpio_num = PCNT_PIN_NOT_USED;
  pulseCounterConfig.lctrl_mode = PCNT_MODE_KEEP;
  pulseCounterConfig.hctrl_mode = PCNT_MODE_KEEP;
  pulseCounterConfig.pos_mode = PCNT_COUNT_INC;
  pulseCounterConfig.neg_mode = PCNT_COUNT_DIS;
  pulseCounterConfig.counter_h_lim = 32767;
  pulseCounterConfig.counter_l_lim = 0;
  pulseCounterConfig.unit = unit;
  pulseCounterConfig.channel = PCNT_CHANNEL_0;
  pcnt_unit_config(&pulseCounterConfig);
  pcnt_set_filter_value(unit, 1000);
  pcnt_filter_enable(unit);
  pcnt_counter_pause(unit);
  pcnt_counter_clear(unit);
}

void ConfigurePulseCounter(){
  ConfigureSinglePulseCounter(CwPulseCounterUnit, CwInputPin);
  ConfigureSinglePulseCounter(CcwPulseCounterUnit, CcwInputPin);
  PulseCounterConfigured = true;
}

void ApplyAzimuthSourceMode(bool useStoredPulseAzimuth){
  if(PulseCounterConfigured == false){
    return;
  }

  pcnt_counter_pause(CwPulseCounterUnit);
  pcnt_counter_pause(CcwPulseCounterUnit);
  pcnt_counter_clear(CwPulseCounterUnit);
  pcnt_counter_clear(CcwPulseCounterUnit);

  if(IsPulseAzimuthSource()){
    float seedAzimuthDeg = float(Azimuth);
    if(useStoredPulseAzimuth){
      seedAzimuthDeg = float(PulseAzimuthTenths) / 10.0f;
    }
    seedAzimuthDeg = ClampFloat(seedAzimuthDeg, 0.0f, float(MaxRotateDegree));
    PulseCounterValue = ClampPulseCount(lroundf(seedAzimuthDeg * float(max(PulsePerDegree, short(1)))));
    AzimuthRawDeg = seedAzimuthDeg;
    AzimuthControlDeg = seedAzimuthDeg;
    AzimuthNoiseDeg = 0.2f;
    Azimuth = int(round(seedAzimuthDeg));
    pcnt_counter_resume(CwPulseCounterUnit);
    pcnt_counter_resume(CcwPulseCounterUnit);
  }
}

void UpdatePulseAzimuthFromCounter(){
  if(PulseCounterConfigured == false || PulsePerDegree < 1){
    return;
  }

  int16_t cwDelta = 0;
  int16_t ccwDelta = 0;
  pcnt_get_counter_value(CwPulseCounterUnit, &cwDelta);
  pcnt_get_counter_value(CcwPulseCounterUnit, &ccwDelta);
  pcnt_counter_clear(CwPulseCounterUnit);
  pcnt_counter_clear(CcwPulseCounterUnit);

  long signedPulseDelta = long(cwDelta) - long(ccwDelta);
  if(ReverseAZ){
    signedPulseDelta = -signedPulseDelta;
  }
  if(signedPulseDelta != 0){
    PulseCounterValue = ClampPulseCount(PulseCounterValue + signedPulseDelta);
  }

  float pulseAzimuthDeg = float(PulseCounterValue) / float(PulsePerDegree);
  pulseAzimuthDeg = ClampFloat(pulseAzimuthDeg, 0.0f, float(MaxRotateDegree));
  AzimuthRawDeg = pulseAzimuthDeg;
  AzimuthControlDeg = pulseAzimuthDeg;
  AzimuthNoiseDeg = 0.2f;
  Azimuth = int(round(pulseAzimuthDeg));
}

void PersistPulseAzimuthIfNeeded(bool forceSave){
  static unsigned int lastSavedPulseAzimuthTenths = 65535;
  static long lastPulseAzimuthSave = 0;

  if(IsPulseAzimuthSource() == false){
    return;
  }

  unsigned int currentTenths = ClampPulseAzimuthTenths(int(round(AzimuthRawDeg * 10.0f)));
  if(lastSavedPulseAzimuthTenths == 65535){
    lastSavedPulseAzimuthTenths = EEPROM.readUShort(419);
  }

  bool changed = currentTenths != lastSavedPulseAzimuthTenths;
  bool enoughTimeElapsed = millis() - lastPulseAzimuthSave > 60000;
  if(changed && (forceSave || (Status == 0 && enoughTimeElapsed))){
    EEPROM.writeUShort(419, currentTenths);
    EEPROM.commit();
    lastSavedPulseAzimuthTenths = currentTenths;
    PulseAzimuthTenths = currentTenths;
    lastPulseAzimuthSave = millis();
    MqttPubString("PulseAzimuth", String(float(currentTenths) / 10.0f, 1), true);
  }
}

int GetPulseVirtualAdcValue(){
  if(IsPulseAzimuthSource() == false || MaxRotateDegree == 0){
    return int(AzimuthValue);
  }
  return int(lroundf(142.0f + (3013.0f * ClampFloat(AzimuthRawDeg, 0.0f, float(MaxRotateDegree)) / float(MaxRotateDegree))));
}

int GetPulseCounterValue(){
  return int(PulseCounterValue);
}

void PersistAutoSlowWindowIfNeeded(){
  static unsigned int LastSavedTenths = 0;
  unsigned int currentTenths = (unsigned int)round(PwmSlowWindowDeg * 10.0);
  if(LastSavedTenths == 0){
    LastSavedTenths = EEPROM.readUShort(328);
  }
  if(currentTenths != LastSavedTenths && millis()-PwmTuneSaveTimer > 60000){
    EEPROM.writeUShort(328, currentTenths);
    EEPROM.commit();
    LastSavedTenths = currentTenths;
    PwmTuneSaveTimer = millis();
    MqttPubString("PwmSlowWindowDeg", String(PwmSlowWindowDeg, 1), true);
  }
}
//-------------------------------------------------------------------------------------------------------

bool HandleManualPwmStatus(long &PwmTimer){
  if(IsManualPwmKeyControlEnabled() == false || IsManualStatusValue(Status) == false){
    return false;
  }

  switch (Status) {
    case -21: {
      ErrorDetect=0;
      ReverseProcedure(true);
      RunTimer();
      AzimuthTarget=-1;
      UiTargetAzimuth=-1;
      Status=-22;
      PwmTimer=millis();
      return true;
    }
    case -22: {
      ErrorDetect=0;
      ApplyDutySlew(PwmMaxDuty, PwmTimer);
      return true;
    }
    case -23: {
      ApplyDutySlew(0, PwmTimer);
      if(dutyCycle==0){
        ledcWrite(mosfetPWMChannel, 0);
        digitalWrite(BrakePin, LOW);
        delay(24);
        digitalWrite(ReversePin, LOW);
        Status=0;
        AzimuthTarget=-1;
        UiTargetAzimuth=-1;
        AzimuthControlDeg=AzimuthRawDeg;
      }
      return true;
    }
    case 21: {
      ErrorDetect=0;
      ReverseProcedure(false);
      RunTimer();
      AzimuthTarget=-1;
      UiTargetAzimuth=-1;
      Status=22;
      PwmTimer=millis();
      return true;
    }
    case 22: {
      ErrorDetect=0;
      ApplyDutySlew(PwmMaxDuty, PwmTimer);
      return true;
    }
    case 23: {
      ApplyDutySlew(0, PwmTimer);
      if(dutyCycle==0){
        ledcWrite(mosfetPWMChannel, 0);
        digitalWrite(BrakePin, LOW);
        delay(24);
        digitalWrite(ReversePin, LOW);
        Status=0;
        AzimuthTarget=-1;
        UiTargetAzimuth=-1;
        AzimuthControlDeg=AzimuthRawDeg;
      }
      return true;
    }
  }

  return false;
}

//-------------------------------------------------------------------------------------------------------

void RunByStatus(){
  static long PwmTimer = 0;
  static bool OneTimeSend = false;
  static bool BrakeLearningActive = false;
  static int BrakeLearningDirection = 0;
  static float BrakeStartDistance = 0.0;
  DetectEndstopZone();
  EthTest();

  if(HandleManualPwmStatus(PwmTimer)){
    return;
  }

  // }else if( (Azimuth>=0 && Azimuth<=450) ){
    switch (Status) {
      case -3: {
        if(ACmotor==false){
          //DC
          if(PWMenable==true){
            ApplyDutySlew(0, PwmTimer);
            if(dutyCycle==0){
              ledcWrite(mosfetPWMChannel, 0);
              digitalWrite(BrakePin, LOW);
              delay(24);
              digitalWrite(ReversePin, LOW);
              Status=0;
              if(BrakeLearningActive){
                if(!SkipBrakeLearningOnManualStop){
                  UpdateAutoSlowWindow(BrakeStartDistance, ReadRemainingDistanceDeg(BrakeLearningDirection, ReadRawAzimuthDeg()));
                }
                BrakeLearningActive = false;
              }
              SkipBrakeLearningOnManualStop = false;
              AzimuthTarget=-1;
              UiTargetAzimuth=-1;
              AzimuthControlDeg=AzimuthRawDeg;
            }
          }else{
            dutyCycle=0;
            ledcWrite(mosfetPWMChannel, 0);
            digitalWrite(BrakePin, LOW);
            delay(24);
            // ReverseProcedure(false);
            digitalWrite(ReversePin, LOW);
            // digitalWrite(ACcwPin, LOW);
            Status=0;
            BrakeLearningActive = false;
            SkipBrakeLearningOnManualStop = false;
            AzimuthTarget=-1;
            UiTargetAzimuth=-1;
          }
        }else{
          //AC
          digitalWrite(ACcwPin, LOW);
          digitalWrite(ReversePin, LOW);
          delay(24);
          digitalWrite(BrakePin, LOW);
          Status=0;
          BrakeLearningActive = false;
          SkipBrakeLearningOnManualStop = false;
        }
        ; break; }
      case -2: {
        if(PWMenable==true){
          MaybeBeginBrakeLearning(-1, BrakeLearningActive, BrakeLearningDirection, BrakeStartDistance);
          if(TargetReachedInDirection(-1)){
            Status=-3;
          }else if(ShouldForceStopFromPwmStall(-1, dutyCycle, ReadRawAzimuthDeg(), PwmMaxDuty)){
            Status=-3;
            MqttPubString("Debug", "Forced stop after low PWM stall during CCW braking", false);
          }else{
            ApplyDutySlew(ComputeClosedLoopDuty(-1), PwmTimer);
          }
        }else{
          if(abs(AzimuthTarget-Azimuth)<2){
            // MqttPubString("Debug", "RunByStatus-2offPWM|"+String(AzimuthTarget)+"-"+String(Azimuth)+"("+String(abs(AzimuthTarget-Azimuth))+")<2", false);
            Status=-3;
          }
        }
        ; break; }
      case -11: {
        ErrorDetect=0;
        // ReverseProcedure(true);
        if(ACmotor==false){
          //DC
          if(PWMenable==true){
            byte targetDuty = ComputeClosedLoopDuty(-1);
            if(targetDuty==0 || TargetReachedInDirection(-1)){
              Status=-3;
            }else{
              ApplyDutySlew(targetDuty, PwmTimer);
              if(dutyCycle>=targetDuty){
                Status=-2;
              }
            }
          }else{
            ledcWrite(mosfetPWMChannel, 255);
            // digitalWrite(ACcwPin, HIGH);
            Status=-2;
          }
        }else{
          //AC
          ledcWrite(mosfetPWMChannel, 0);
          if(Reverse==false){ // CCW
            digitalWrite(ACcwPin, LOW);
            digitalWrite(ReversePin, HIGH);
          }else{ // CW
            digitalWrite(ACcwPin, HIGH);
            digitalWrite(ReversePin, LOW);
          }
          Status=-2;
        }
        ; break; }
      case -1: {
        ReverseProcedure(true);
        RunTimer();
        BrakeLearningActive = false;
        SkipBrakeLearningOnManualStop = false;
        ShouldForceStopFromPwmStall(0, PwmMaxDuty, ReadRawAzimuthDeg(), PwmMaxDuty);
        Status=-11;
        OneTimeSend = false;
        PwmTimer=millis();
        ; break; }
      case  0: {
        BrakeLearningActive = false;
        SkipBrakeLearningOnManualStop = false;
        LedStatusErr();
        if(OneTimeSend==false){
          MqttPubString("AzimuthStop", String(Azimuth), false);
          OneTimeSend = true;
          TxMqttAzimuthTimer=millis();
        }
        ; break; }
      case 1: {
        ReverseProcedure(false);
        RunTimer();
        BrakeLearningActive = false;
        SkipBrakeLearningOnManualStop = false;
        ShouldForceStopFromPwmStall(0, PwmMaxDuty, ReadRawAzimuthDeg(), PwmMaxDuty);
        Status=11;
        OneTimeSend = false;
        PwmTimer=millis();
        ; break; }
      case  11: {
        ErrorDetect=0;
        // ReverseProcedure(false);
        if(ACmotor==false){
          //DC
          if(PWMenable==true){
            byte targetDuty = ComputeClosedLoopDuty(1);
            if(targetDuty==0 || TargetReachedInDirection(1)){
              Status=3;
            }else{
              ApplyDutySlew(targetDuty, PwmTimer);
              if(dutyCycle>=targetDuty){
                Status=2;
              }
            }
          }else{
            ledcWrite(mosfetPWMChannel, 255);
            // digitalWrite(ACcwPin, HIGH);
            Status=2;
          }
        }else{
          //AC
          ledcWrite(mosfetPWMChannel, 0);
          if(Reverse==false){ // CW
            digitalWrite(ACcwPin, HIGH);
            digitalWrite(ReversePin, LOW);
          }else{ // CCW
            digitalWrite(ACcwPin, LOW);
            digitalWrite(ReversePin, HIGH);
          }
          Status=2;
        }
        ; break; }
      case  2: {
        if(PWMenable==true){
          MaybeBeginBrakeLearning(1, BrakeLearningActive, BrakeLearningDirection, BrakeStartDistance);
          if(TargetReachedInDirection(1)){
            Status=3;
          }else if(ShouldForceStopFromPwmStall(1, dutyCycle, ReadRawAzimuthDeg(), PwmMaxDuty)){
            Status=3;
            MqttPubString("Debug", "Forced stop after low PWM stall during CW braking", false);
          }else{
            ApplyDutySlew(ComputeClosedLoopDuty(1), PwmTimer);
          }
        }else{
          if(abs(AzimuthTarget-Azimuth)<2){
            // MqttPubString("Debug", "RunByStatus2offPWM|"+String(AzimuthTarget)+"-"+String(Azimuth)+"("+String(abs(AzimuthTarget-Azimuth))+")<2", false);
            Status=3;
          }
        }
        ; break; }
      case  3: {
        if(ACmotor==false){
          //DC
          if(PWMenable==true){
            ApplyDutySlew(0, PwmTimer);
            if(dutyCycle==0){
              ledcWrite(mosfetPWMChannel, 0);
              digitalWrite(BrakePin, LOW);
              delay(24);
              digitalWrite(ReversePin, LOW);
              Status=0;
              if(BrakeLearningActive){
                if(!SkipBrakeLearningOnManualStop){
                  UpdateAutoSlowWindow(BrakeStartDistance, ReadRemainingDistanceDeg(BrakeLearningDirection, ReadRawAzimuthDeg()));
                }
                BrakeLearningActive = false;
              }
              SkipBrakeLearningOnManualStop = false;
              AzimuthTarget=-1;
              UiTargetAzimuth=-1;
              AzimuthControlDeg=AzimuthRawDeg;
            }
          }else{
            dutyCycle=0;
            ledcWrite(mosfetPWMChannel, 0);
            digitalWrite(BrakePin, LOW);
            delay(24);
            // ReverseProcedure(false);
            digitalWrite(ReversePin, LOW);
            // digitalWrite(ACcwPin, LOW);
            Status=0;
            BrakeLearningActive = false;
            SkipBrakeLearningOnManualStop = false;
            AzimuthTarget=-1;
            UiTargetAzimuth=-1;
          }
        }else{
          //AC
          digitalWrite(ACcwPin, LOW);
          digitalWrite(ReversePin, LOW);
          delay(24);
          digitalWrite(BrakePin, LOW);
          Status=0;
          BrakeLearningActive = false;
          SkipBrakeLearningOnManualStop = false;
        }
        ; break; }
    }
  // }
}

//-------------------------------------------------------------------------------------------------------
void ReverseProcedure(bool CCW){

  if(CCW==false){ // CW
    if(ACmotor==false){
      //DC
      if(Reverse==false){
        digitalWrite(ReversePin, LOW);
      }else{
        digitalWrite(ReversePin, HIGH);
      }
      delay(24);
      digitalWrite(BrakePin, HIGH);
    }else{
      //AC
      digitalWrite(BrakePin, HIGH);
      delay(24);
    }
  }else{  // CCW
    if(ACmotor==false){
      //DC
      if(Reverse==false){
        digitalWrite(ReversePin, HIGH);
      }else{
        digitalWrite(ReversePin, LOW);
      }
      delay(24);
      digitalWrite(BrakePin, HIGH);
    }else{
      //AC
      digitalWrite(BrakePin, HIGH);
      delay(24);
    }
  }
   delay(12);
}




String LeadingZero(int NumberOfZero, int NR){
  String StrBuf = String((int)round(NR));
  if(StrBuf.length()>NumberOfZero){
    StrBuf="0";
  }
  for (int i=StrBuf.length(); i<NumberOfZero; i++){
    StrBuf="0"+StrBuf;
  }
  return StrBuf;
}

void CLI2(){
  int OUT=2;
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    OUT = 0;
  }
  esp_task_wdt_reset();
  WdtTimer=millis();


/*
https://www.yaesu.com/downloadFile.cfm?FileID=820&FileCatID=155&FileName=GS232A.pdf&FileContentType=application%2Fpdf
https://www.passion-radio.com/index.php?controller=attachment&id_attachment=782
https://www.qsl.net/dh1ngp/onlinehelpft100/Rotator_control_with_Easycomm.htm
https://www.mustbeart.com/software/easycomm.txt

Command		Meaning			Perameters
-------		-------			----------
AZ		Azimuth			number - 1 decimal place
ML		Move Left
MR		Move Right
SA		Stop azimuth moving

EL		Elevation		number - 1 decimal place
UP		Uplink freq		in Hertz
DN		Downlink freq		in Hertz
DM		Downlink Mode		ascii, eg SSB, FM
UM		Uplink Mode		ascii, eg SSB, FM
DR		Downlink Radio		number
UR		Uplink Radio		number
MU		Move Up
MD		Move Down
SE		Stop elevation moving
AO		AOS
LO		LOS
OP		Set output		number
IP		Read an input		number
AN		Read analogue input	number
ST		Set time		YY:MM:DD:HH:MM:SS
VE		Request Version

GS-232B
-------
R 82 Clockwise Rotation
L 76 Counter Clockwise Rotation
A 65 CW/CCW Rotation Stop
  S 83 All Stop
C 67 Antenna Direction Value
M 77 Antenna Direction Setting. MXXX
O 79 Offset Calibration
F 70 Full Scale Calibration

M Time Interval Direction Setting.
  MTTT XXX XXX XXX ---
  (TTT = Step value)
  (XXX = Horizontal Angle)
T Start Command in the time interval direction setting mode.
N Total number of setting angles in “M” mode and traced number of all datas (setting angles)
X1 Rotation Speed 1 (Horizontal) Low
X2 Rotation Speed 2 (Horizontal) Middle 1
X3 Rotation Speed 3 (Horizontal) Middle 2
X4 Rotation Speed 4 (Horizontal) High
*/
long RawTmp = 0;

  // ? H h
  if(incomingByte==63 || incomingByte==72 || incomingByte==104){
    // if(eth_connected == false){
    //   Serial.println("ETH  disconnected");
    // } 
    Prn(OUT, 1, "http://"+String(ETH.localIP()[0])+"."+String(ETH.localIP()[1])+"."+String(ETH.localIP()[2])+"."+String(ETH.localIP()[3]) );
    // Serial.print("MAC ");
    // Serial.println(MACString);
    // Prn(OUT, 1,"");
    Prn(OUT, 1,"key - print web password");
    Prn(OUT, 1,"noauth - disable web authentication");
    Prn(OUT, 1,"R L A S C Mxxx O F - supported GS-232 commands");

  // key
  }else if(incomingByte==107 || incomingByte==75){
    char firstChar = char(incomingByte);
    String tail = ReadSerialCommandTail(2, 300);
    String command = String(firstChar) + tail;
    if(command == "key"){
      PrintWebKey(OUT);
    }

  // noauth / NOAUTH
  }else if(incomingByte==110 || incomingByte==78){
    char firstChar = char(incomingByte);
    String tail = ReadSerialCommandTail(5, 500);
    String command = String(firstChar) + tail;
    if(command == "noauth" || command == "NOAUTH"){
      DisableWebAuth(OUT);
    }

  // R 82 Clockwise Rotation
  }else if(incomingByte==82){
    Status=1; //digitalWrite(BrakePin, HIGH); delay(24);
    // RunTimer();
    Prn(OUT, 1, "CW" );

  // L 76 Counter Clockwise Rotation
  }else if(incomingByte==76){
    Status=-1; //digitalWrite(BrakePin, HIGH); delay(24);
    // RunTimer();
    Prn(OUT, 1, "CCW" );

  // A 65 CW/CCW Rotation Stop
  //   S 83 All Stop
  }else if(incomingByte==65||incomingByte==83){
    RequestStopRamp(true);
    Prn(OUT, 1, "Rotation Stop" );

  // C 67 Antenna Direction Value
  }else if(incomingByte==67){
    Prn(OUT, 0, "+0" );
    if(Azimuth+StartAzimuth < 100){
      Prn(OUT, 0, "0" );
      if(Azimuth+StartAzimuth < 10){
        Prn(OUT, 0, "0" );
      }
    }
    Prn(OUT, 1, String(Azimuth+StartAzimuth) );

  // M 77 Antenna Direction Setting. MXXX
  }else if(incomingByte==77){
    InputByte[0]=0;
    incomingByte = 0;
    bool br=false;
    // Prn(OUT, 0,"> ");

    while(br==false) {
      if(Serial.available()){
        incomingByte=Serial.read();
        if(incomingByte==13 || incomingByte==10){ // CR or LF
          br=true;
          // Prn(OUT, 1,"");
        }else{
          // Serial.write(incomingByte);
          if(incomingByte!=10 && incomingByte!=13){
            if(incomingByte==127){
              if(InputByte[0] > 0){
                InputByte[0]--;
              }
            }else{
              InputByte[InputByte[0]+1]=incomingByte;
              InputByte[0]++;
            }
          }
        }
        if(InputByte[0]==3){
          br=true;
          // Prn(OUT, 1," too long");
        }
      }
    }
    int intBuf=0;
    int mult=1;
    for (int j=InputByte[0]; j>0; j--){
      if(InputByte[j]>=48 && InputByte[j]<=57 || InputByte[j]==46 || InputByte[j]==44 ){ // numbers only + . ,
        intBuf = intBuf + ((InputByte[j]-48)*mult);
        mult = mult*10;
        if(InputByte[j]==46 || InputByte[j]==44){ //reset at . ,
          intBuf=0;
          mult=1;
        }
      }
    }
    if(intBuf>=0 && intBuf<=MaxRotateDegree){
      Prn(OUT, 0,"Rotate to ");
      Prn(OUT, 0, String(intBuf) );
      Prn(OUT, 1,"°");

      AzimuthTarget = intBuf;
      if(AzimuthTarget>359){
        AzimuthTarget = AzimuthTarget - 360;
      }
      MqttPubString("AzimuthTarget", String(AzimuthTarget), false);
      RotCalculate();
    }

  // O 79 Offset Calibration
  }else if(incomingByte==79){
    Prn(OUT, 1,"Rotate to full CCW and press [y/n]");
    bool br = false;
    while(br==false) {
      if(Serial.available()){
        incomingByte=Serial.read();
        if(incomingByte==89 || incomingByte==121){
          // RawTmp = 0;
          // for (int i=0; i<10; i++){
          //   RawTmp = RawTmp + readADC_Cal(analogRead(AzimuthPin));
          //   delay(10);
          // }
          // CcwRaw = RawTmp / 10;
          CcwRaw = AzimuthValue;
          EEPROM.writeUShort(31, CcwRaw);
          EEPROM.commit();
          Prn(OUT, 1,"Offset Calibration to "+String(float(CcwRaw)/1000)+"V" );
          MqttPubString("CcwRaw", String(CcwRaw), true);
          br=true;
        }else if(incomingByte!=13 && incomingByte!=10){
          br=true;
        }
      }
    }

  // F 70 Full Scale Calibration
  }else if(incomingByte==70){
    Prn(OUT, 1,"Rotate to full CW and press [y/n]");
    bool br = false;
    while(br==false) {
      if(Serial.available()){
        incomingByte=Serial.read();
        if(incomingByte==89 || incomingByte==121){
          // RawTmp = 0;
          // for (int i=0; i<10; i++){
          //   RawTmp = RawTmp + readADC_Cal(analogRead(AzimuthPin));
          //   delay(10);
          // }
          // CwRaw = RawTmp / 10;
          CwRaw = AzimuthValue;
          EEPROM.writeUShort(33, CwRaw);
          EEPROM.commit();
          Prn(OUT, 1,"Full Scale Calibration to "+String(float(CwRaw)/1000)+"V" );
          MqttPubString("CwRaw", String(CwRaw), true);
          br=true;
        }else if(incomingByte!=13 && incomingByte!=10){
          br=true;
        }
      }
    }

  // CR/LF
  }else if(incomingByte==13||incomingByte==10){
    // Prn(OUT, 1,"");

  // anykey
  }else{
    // Prn(OUT, 0," [");
    // Prn(OUT, 0, String(incomingByte) ); //, DEC);
    // Prn(OUT, 1,"] unknown command");
  }
  incomingByte=0;
}
void Enter(){
  int OUT=0;

  InputByte[0]=0;
  incomingByte = 0;
  bool br=false;
  Prn(OUT, 0,"> ");

  while(br==false) {
    if(Serial.available()){
      incomingByte=Serial.read();
      if(incomingByte==13 || incomingByte==59){ // CR or ;
        br=true;
        Prn(OUT, 1,"");
      }else{
        Serial.write(incomingByte);
        if(incomingByte!=10 && incomingByte!=13){
          if(incomingByte==127){
            if(InputByte[0] > 0){
              InputByte[0]--;
            }
          }else{
            InputByte[InputByte[0]+1]=incomingByte;
            InputByte[0]++;
          }
        }
      }
      if(InputByte[0]==20){
        br=true;
        Prn(OUT, 1," too long");
      }
    }
  }

  // Serial.println();
  // for (int i=1; i<InputByte[0]+1; i++){
    // Serial.write(InputByte[i]);
  // }
  // Serial.println();

  // Prn(OUT, 1, "out"+String(CompareInt) );
}

//-------------------------------------------------------------------------------------------------------
void EnterChar(int OUT){
  incomingByte = 0;
  Prn(OUT, 0," > ");
  while (Serial.available() == 0) {
    // Wait
  }
  incomingByte = Serial.read();
  Prn(OUT, 1, String(char(incomingByte)) );
}

void Prn(int OUT, int LN, String STR){
  Serial.print(STR);
  if(LN==1){
    Serial.println();
  }
}

//-------------------------------------------------------------------------------------------------------
void ListCommands(int OUT){
  // digitalWrite(EnablePin,1);
  if(OUT==0){
    Prn(OUT, 1,"");
    Prn(OUT, 1,"");
    Prn(OUT, 1," =============================================================");
    Prn(OUT, 1," Please copy and save the IP address and MAC address");
    Prn(OUT, 1,"");
      Prn(OUT, 1, "   "+String(ETH.localIP()[0])+"."+String(ETH.localIP()[1])+"."+String(ETH.localIP()[2])+"."+String(ETH.localIP()[3]) );
      Serial.print("   ");
      Serial.println(MACString);
    Prn(OUT, 1,"");
    Prn(OUT, 1,"   Use command 'key' to print the current web password");
    Prn(OUT, 1,"");
    Prn(OUT, 1," Then disconnect the USB, and log in using web");
    Prn(OUT, 1," More information https://remoteqth.com/w/doku.php?id=3d_print_wx_station#second_step_connect_remotely_via_ip");
    Prn(OUT, 1," =============================================================");
    Prn(OUT, 1,"");
  }
  else{
    #if defined(ETHERNET)
      Prn(OUT, 1,"");
      Prn(OUT, 1,"------------  Rotator station status  ------------");
      Prn(OUT, 0,"  http://");
      Prn(OUT, 1, String(ETH.localIP()[0])+"."+String(ETH.localIP()[1])+"."+String(ETH.localIP()[2])+"."+String(ETH.localIP()[3]) );
      Prn(OUT, 0,"  ETH: MAC ");
      Prn(OUT, 0, MACString +", " );
      // Prn(OUT, 0, String(ETH.macAddress()[0], HEX)+":"+String(ETH.macAddress()[1], HEX)+":"+String(ETH.macAddress()[2], HEX)+":"+String(ETH.macAddress()[3], HEX)+":"+String(ETH.macAddress()[4], HEX)+":"+String(ETH.macAddress()[5], HEX)+", " );
      Prn(OUT, 0, String(ETH.linkSpeed()) );
      Prn(OUT, 0,"Mbps");
      if (ETH.fullDuplex()) {
        Prn(OUT, 0,", FULL_DUPLEX ");
      }
    #else
      Prn(OUT, 0,"  ETHERNET OFF ");
    #endif
    #if defined(WIFI)
      Prn(OUT, 1,"  =================================");
      Prn(OUT, 0,"  http://");
      Prn(OUT, 1, String(WiFi.localIP()[0])+"."+String(WiFi.localIP()[1])+"."+String(WiFi.localIP()[2])+"."+String(WiFi.localIP()[3]) );
      Prn(OUT, 0,"  dBm: ");
      Prn(OUT, 1, String(WiFi.RSSI()) );
    #else
      Prn(OUT, 1,"  WIFI: OFF");
    #endif

    if(OUT==0){
      Prn(OUT, 1,"  Use command 'key' to print the current web password");
    }
    // Prn(OUT, 1, "  ChipID: "+ChipidHex);

    Prn(OUT, 0,"  NTP UTC:");
    Prn(OUT, 1, UtcTime(1));
    Prn(OUT, 0,"  Uptime: ");
    if(millis() < 60000){
      Prn(OUT, 0, String(millis()/1000) );
      Prn(OUT, 1," second");
    }else if(millis() > 60000 && millis() < 3600000){
      Prn(OUT, 0, String(millis()/60000) );
      Prn(OUT, 1," minutes");
    }else if(millis() > 3600000 && millis() < 86400000){
      Prn(OUT, 0, String(millis()/3600000) );
      Prn(OUT, 1," hours");
    }else{
      Prn(OUT, 0, String(millis()/86400000) );
      Prn(OUT, 1," days");
    }

    Prn(OUT, 0,"  MqttSubscribe: "+String(mqtt_server_ip[0])+"."+String(mqtt_server_ip[1])+"."+String(mqtt_server_ip[2])+"."+String(mqtt_server_ip[3])+":"+String(MQTT_PORT)+"/");
      String topic = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/sub";
      const char *cstr = topic.c_str();
      if(mqttClient.subscribe(cstr)==true){
        Prn(OUT, 1, String(cstr));
      }else{
        Prn(OUT, 1, "FALSE");
      }

    Prn(OUT, 0,"  Firmware: ");
    Prn(OUT, 1, String(REV));
    // Prn(OUT, 0," | Hardware: ");
    // Prn(OUT, 1, String(HWREV));


    // Prn(OUT, 0,"  micro SD card present: ");
    // uint8_t cardType = SD_MMC.cardType();
    //
    // if(cardType == CARD_NONE){
    //     Prn(OUT, 1,"none ");
    //     // return;
    // }else if(cardType == CARD_MMC){
    //     Prn(OUT, 1,"MMC ");
    // } else if(cardType == CARD_SD){
    //     Prn(OUT, 1,"SDSC ");
    // } else if(cardType == CARD_SDHC){
    //     Prn(OUT, 1,"SDHC ");
    // } else {
    //     Prn(OUT, 1,"unknown ");
    // }
    Prn(OUT, 1,"------------------  Menu  -------------------");
    // Prn(OUT, 1,"  You can change source, with send character:");
    // if(TxUdpBuffer[2]=='m'){
    //   Prn(OUT, 0,"     [m]");
    // }else{
    //   Prn(OUT, 0,"      m ");
    // }
    // Prn(OUT, 1,"- IP switch master");
    // if(TxUdpBuffer[2]=='r'){
    //   Prn(OUT, 0,"     [r]");
    // }else{
    //   Prn(OUT, 0,"      r ");
    // }
    // Prn(OUT, 1,"- Band decoder");
    // if(TxUdpBuffer[2]=='o'){
    //   Prn(OUT, 0,"     [o]");
    // }else{
    //   Prn(OUT, 0,"      o ");
    // }
    // Prn(OUT, 1,"- Open Interface III");
    // if(TxUdpBuffer[2]=='n'){
    //   Prn(OUT, 0,"     [n]");
    // }else{
    //   Prn(OUT, 0,"      n ");
    // }
    // Prn(OUT, 1,"- none");
    // Prn(OUT, 1,"");
    Prn(OUT, 1,"      ?  list status and commands");
    Prn(OUT, 0,"      m  Altitude [");
    Prn(OUT, 0, String(Altitude)+" m]");
    if(Altitude==0){
      Prn(OUT, 0, " PLEASE SET ALTITUDE");
    }
    Prn(OUT, 1, "");

    Prn(OUT, 0,"      *  terminal debug ");
      if(EnableSerialDebug==0){
        Prn(OUT, 1,"[OFF]");
      }else if(EnableSerialDebug==1){
        Prn(OUT, 1,"[ON]");
      }else if(EnableSerialDebug==2){
        Prn(OUT, 1,"[ON-frenetic]");
      }
    Prn(OUT, 0,"      #  network ID prefix [");
    byte ID = 0;
    bitClear(ID, 0); // ->
    bitClear(ID, 1);
    bitClear(ID, 2);
    bitClear(ID, 3);
    ID = ID >> 4;
    Prn(OUT, 0, String(ID, HEX) );
    Prn(OUT, 0,"] hex");
    Prn(OUT, 1,"");

    ID = 0;
    bitClear(ID, 4);
    bitClear(ID, 5);
    bitClear(ID, 6);
    bitClear(ID, 7); // <-
    Prn(OUT, 0,"         +network ID sufix [");
    Prn(OUT, 0, String(ID, HEX) );
    Prn(OUT, 0,"] hex");
    Prn(OUT, 1,"");
    Prn(OUT, 1, "      +  change MQTT broker IP | "+String(mqtt_server_ip[0])+"."+String(mqtt_server_ip[1])+"."+String(mqtt_server_ip[2])+"."+String(mqtt_server_ip[3])+":"+String(MQTT_PORT));
    // Prn(OUT, 0, String(mqtt_server_ip));
    // Prn(OUT, 0, ":");
    // Prn(OUT, 1, String(MQTT_PORT));
    Prn(OUT, 1,"      x  TX repeat time ["+String(MeasureTimer[1]/60000)+" min]");
    Prn(OUT, 1,"      L  change location | "+YOUR_CALL);
    Prn(OUT, 1,"      &  send broadcast packet");
    Prn(OUT, 1,"      E  erase whole eeprom (web password also)");
    // Prn(OUT, 1,"      C  eeprom commit");
    Prn(OUT, 1,"      /  list directory");
    Prn(OUT, 1,"      R  read log file");
    Prn(OUT, 1,"      e  list EEPROM");
    Prn(OUT, 1,"      2  I2C scanner");
    Prn(OUT, 1,"      .  reset timer and send measure");
    Prn(OUT, 1,"      W  erase wind speed max memory");
    Prn(OUT, 1,"      key  print web password");
    Prn(OUT, 1,"      noauth  disable web authentication");
    Prn(OUT, 1,"      @  restart device");
    // Prn(OUT, 1,"---------------------------------------------");
    Prn(OUT, 0, " > " );
  }
  // digitalWrite(EnablePin,0);
}

String ReadSerialCommandTail(byte maxLen, unsigned long timeoutMs){
  String tail = "";
  unsigned long started = millis();
  while(tail.length() < maxLen && millis() - started < timeoutMs){
    if(Serial.available()){
      char c = Serial.read();
      if(c == '\r' || c == '\n' || c == ';'){
        break;
      }
      tail += c;
    }
  }
  return tail;
}

void PrintWebKey(int OUT){
  Prn(OUT, 1,"");
  Prn(OUT, 1,"Web authentication");
  Prn(OUT, 1,"  user: admin");
  Prn(OUT, 0,"  password: ");
  if(WebAuthPassword.length() > 0){
    Prn(OUT, 0, WebAuthPassword);
  }else{
    Prn(OUT, 0, "(not set)");
  }
  Prn(OUT, 1,"");
  Prn(OUT, 1,"");
}

void DisableWebAuth(int OUT){
  WebAuthEnabled = false;
  EEPROM.writeBool(232, WebAuthEnabled);
  EEPROM.commit();
  Prn(OUT, 1,"Web authentication disabled.");
  Prn(OUT, 1,"Use Setup > Web authentication to enable it again.");
}

void http(){
  // listen for incoming clients
  WiFiClient webClient = server.available();
  if (webClient) {
    if(EnableSerialDebug>0){
      Serial.println("WIFI New webClient");
    }
    memset(linebuf,0,sizeof(linebuf));
    charcount=0;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (webClient.connected()) {
      if (webClient.available()) {
        char c = webClient.read();
        HTTP_req += c;
        // if(EnableSerialDebug>0){
        //   Serial.write(c);
        // }
        //read char by char HTTP request
        linebuf[charcount]=c;
        if (charcount<sizeof(linebuf)-1) charcount++;
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          int firstSpace = HTTP_req.indexOf(' ');
          int secondSpace = HTTP_req.indexOf(' ', firstSpace + 1);
          String method = "GET";
          String uri = "/";
          if(firstSpace > 0 && secondSpace > firstSpace){
            method = HTTP_req.substring(0, firstSpace);
            uri = HTTP_req.substring(firstSpace + 1, secondSpace);
          }
          if(uri == "/dxcws"){
            DxcHandleWebSocketUpgrade(webClient, HTTP_req, method, uri);
            HTTP_req = "";
            return;
          }
          String authorization = ExtractHttpHeader(HTTP_req, "Authorization");
          if(!CheckDigestAuthorization(authorization, method, uri)){
            SendHttpDigestChallenge(webClient);
            if(EnableSerialDebug>0){
              Serial.print(HTTP_req);
            }
            HTTP_req = "";
            break;
          }
          if(uri == "/dxc.html"){
            File dxcFile = SPIFFS.open("/dxc.html", "r");
            if(!dxcFile){
              webClient.println(F("HTTP/1.1 404 Not Found"));
              webClient.println(F("Content-Type: text/plain"));
              webClient.println(F("Connection: close"));
              webClient.println();
              webClient.println(F("Missing /dxc.html in SPIFFS"));
            }else{
              webClient.println(F("HTTP/1.1 200 OK"));
              webClient.println(F("Content-Type: text/html; charset=utf-8"));
              webClient.println(F("Connection: close"));
              webClient.println();
              uint8_t fileBuffer[512];
              while(dxcFile.available()){
                int chunk = dxcFile.read(fileBuffer, sizeof(fileBuffer));
                if(chunk > 0){
                  webClient.write(fileBuffer, chunk);
                }
              }
              dxcFile.close();
            }
            HTTP_req = "";
            break;
          }
          // send a standard http response header

          // send a standard http response header
          webClient.println(F("HTTP/1.1 200 OK"));
          webClient.println(F("Content-Type: text/html"));
          webClient.println(F("Connection: close"));  // the connection will be closed after completion of the response
          webClient.println();
          webClient.println(F("  <!DOCTYPE html>"));
          webClient.println(F("  <html>"));
          webClient.println(F("      <head>"));
          webClient.println(F("          <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\"/>"));
          webClient.println(F("          <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"));
          // webClient.println(F("          <meta http-equiv=\"refresh\" content=\"10\">"));
          webClient.println(F("          <link rel=\"stylesheet\" type=\"text/css\" href=\"https://remoteqth.com/mqtt-wall/style.css\">"));
          // TITLE
          webClient.print(F("           <title>IP rotator "));
          webClient.print(YOUR_CALL);
          webClient.println(F("</title>"));
          // END TITLE
          webClient.println(F("          <link rel=\"apple-touch-icon\" sizes=\"180x180\" href=\"style/apple-touch-icon.png\">"));
          webClient.println(F("          <link rel=\"mask-icon\" href=\"style/safari-pinned-tab.svg\" color=\"#5bbad5\">"));
          webClient.println(F("          <link rel=\"icon\" type=\"image/png\" href=\"style/favicon-32x32.png\" sizes=\"32x32\">"));
          webClient.println(F("          <link rel=\"icon\" type=\"image/png\" href=\"style/favicon-16x16.png\" sizes=\"16x16\">"));
          webClient.println(F("          <link rel=\"manifest\" href=\"style/manifest.json\">"));
          webClient.println(F("          <link rel=\"shortcut icon\" href=\"style/favicon.ico\">"));
          webClient.println(F("          <meta name='apple-mobile-web-app-capable' content='yes'>"));
          webClient.println(F("          <meta name='mobile-web-app-capable' content='yes'>"));
          webClient.println(F("          <meta name=\"msapplication-config\" content=\"style/browserconfig.xml\">"));
          webClient.println(F("          <meta name=\"theme-color\" content=\"#111827\">"));
          webClient.println(F("          <style type=\"text/css\">"));
          webClient.println(F("            :root { color-scheme: dark; }"));
          webClient.println(F("            body { background: radial-gradient(circle at top, #1f2937 0%, #111827 48%, #030712 100%) !important; color: #e5e7eb !important; }"));
          webClient.println(F("            a { color: #fbbf24; }"));
          webClient.println(F("            a:hover { color: #fdba74; }"));
          webClient.println(F("            #frame { background: rgba(17, 24, 39, 0.88) !important; color: #e5e7eb !important; border: 1px solid rgba(148, 163, 184, 0.18); border-radius: 18px; box-shadow: 0 24px 60px rgba(0, 0, 0, 0.45); }"));
          webClient.println(F("            #header { position: static !important; top: auto !important; z-index: auto !important; width: auto !important; margin: 0 0 14px 0; padding: 12px 14px !important; background: linear-gradient(180deg, rgba(51, 65, 85, 0.96) 0%, rgba(30, 41, 59, 0.96) 100%) !important; border: 1px solid rgba(148, 163, 184, 0.24); border-radius: 14px; box-shadow: 0 16px 32px rgba(0, 0, 0, 0.22); }"));
          webClient.println(F("            #footer { background: transparent !important; }"));
          webClient.println(F("            #header #topic-box { background: rgba(15, 23, 42, 0.82) !important; border: 1px solid rgba(148, 163, 184, 0.22); border-radius: 10px; padding: 4px 8px; }"));
          webClient.println(F("            .messages { margin-top: 0 !important; padding-top: 0 !important; background: rgba(15, 23, 42, 0.72) !important; border: 1px solid rgba(148, 163, 184, 0.14); border-radius: 14px; overflow: hidden; }"));
          webClient.println(F("            .messages article, .messages .message, .messages li { background: rgba(30, 41, 59, 0.78) !important; color: #e5e7eb !important; border-color: rgba(148, 163, 184, 0.18) !important; }"));
          webClient.println(F("            .messages .message { position: relative; overflow: hidden; --ipr-flash-start: 0px; }"));
          webClient.println(F("            .messages .message header { color: #f8fafc !important; font-weight: 400; }"));
          webClient.println(F("            .messages .message header h2 { color: #f8fafc !important; font-weight: 400; }"));
          webClient.println(F("            .messages .message header .mark { background: rgba(148, 163, 184, 0.28) !important; color: #e5e7eb !important; }"));
          webClient.println(F("            .messages .message header .mark.retain { background: #dc2626 !important; color: #fff !important; }"));
          webClient.println(F("            .messages .message header .mark.qos[data-qos=\"1\"] { background: #475569 !important; color: #fff !important; }"));
          webClient.println(F("            .messages .message header .mark.qos[data-qos=\"2\"] { background: #0f172a !important; color: #fff !important; }"));
          webClient.println(F("            .messages .message header, .messages .message p, .messages .message code { position: relative; z-index: 1; font-variant-numeric: tabular-nums; }"));
          webClient.println(F("            .messages .message header { background: rgba(30, 41, 59, 0.78) !important; }"));
          webClient.println(F("            .messages .message p { color: #f8fafc !important; background-color: rgba(45, 58, 78, 0.98) !important; background-image: none !important; border: 1px solid rgba(148, 163, 184, 0.24); box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.05); transition: background-color 0.16s ease, border-color 0.16s ease, box-shadow 0.16s ease; }"));
          webClient.println(F("            .messages .message.ipr-flash::after { content: \"\"; position: absolute; top: 0; right: 0; bottom: 0; left: var(--ipr-flash-start); pointer-events: none; z-index: 0; animation: iprRowFlash 0.85s ease-out; }"));
          webClient.println(F("            .messages .message p.ipr-flash { animation: iprValueTextFlash 0.85s ease-out; }"));
          webClient.println(F("            @keyframes iprRowFlash {"));
          webClient.println(F("              0% { background-color: rgba(255, 255, 255, 0.26); }"));
          webClient.println(F("              35% { background-color: rgba(255, 255, 255, 0.14); }"));
          webClient.println(F("              100% { background-color: rgba(255, 255, 255, 0); }"));
          webClient.println(F("            }"));
          webClient.println(F("            @keyframes iprValueTextFlash {"));
          webClient.println(F("              0% { color: #ffffff !important; border-color: rgba(255, 255, 255, 0.55); box-shadow: 0 0 0 1px rgba(255, 255, 255, 0.12); }"));
          webClient.println(F("              35% { color: #ffffff !important; border-color: rgba(255, 255, 255, 0.34); box-shadow: 0 0 0 1px rgba(255, 255, 255, 0.08); }"));
          webClient.println(F("              100% { color: #f8fafc !important; border-color: rgba(148, 163, 184, 0.24); box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.05); }"));
          webClient.println(F("            }"));
          webClient.println(F("            .status, .status a, .status em, .status span, #footer, #footer p, #footer a { color: #d1d5db !important; }"));
          webClient.println(F("            code { background: rgba(15, 23, 42, 0.95) !important; border-radius: 6px; padding: 2px 6px; }"));
          webClient.println(F("            #topic { background: transparent !important; color: #f9fafb !important; border: none !important; border-radius: 8px; font-weight: 700; letter-spacing: 0.03em; }"));
          webClient.println(F("            #topic::placeholder { color: #94a3b8; }"));
          webClient.println(F("            button { box-shadow: 0 12px 24px rgba(0, 0, 0, 0.28); }"));
          webClient.println(F("            #firmware-actions { display:none; max-width: 760px; margin: 14px auto 0 auto; padding: 18px 20px 20px 20px; text-align:center; background: linear-gradient(180deg, rgba(251, 191, 36, 0.18) 0%, rgba(249, 115, 22, 0.18) 100%); border: 1px solid rgba(251, 191, 36, 0.34); border-radius: 16px; box-shadow: 0 18px 40px rgba(0, 0, 0, 0.30); }"));
          webClient.println(F("            #firmware-actions.firmware-actions-current { background: rgba(15, 23, 42, 0.18); border-color: rgba(148, 163, 184, 0.08); box-shadow: none; }"));
          webClient.println(F("            #firmware-update-help { display:none; color:#ffedd5 !important; font-size:100%; line-height:1.6; margin:0 auto; max-width:640px; }"));
          webClient.println(F("            #firmware-actions.firmware-actions-current #firmware-update-help { color:#9ca3af !important; }"));
          webClient.println(F("            .firmware-actions-row { display:flex; flex-wrap:wrap; justify-content:center; gap:12px; margin-top:16px; }"));
          webClient.println(F("            .firmware-cta, .firmware-cta:link, .firmware-cta:visited, .firmware-cta:hover, .firmware-cta:active, #firmware-download-btn, #firmware-download-btn:link, #firmware-download-btn:visited, #firmware-download-btn:hover, #firmware-download-btn:active, #firmware-upload-btn, #firmware-upload-btn:link, #firmware-upload-btn:visited, #firmware-upload-btn:hover, #firmware-upload-btn:active { display:none; color:#000 !important; text-decoration:none; }"));
          webClient.println(F("            .firmware-cta { background:#fb923c; padding:10px 22px; border:1px solid rgba(251, 146, 60, 0.55); border-radius:6px; font-weight:400; font-size:110%; box-shadow: 0 12px 24px rgba(124, 45, 18, 0.22); transition: background-color 0.2s ease, border-color 0.2s ease, color 0.2s ease, box-shadow 0.2s ease; }"));
          webClient.println(F("            .firmware-cta:hover, #firmware-download-btn:hover, #firmware-upload-btn:hover { background:#fdba74; color:#000 !important; }"));
          webClient.println(F("            #firmware-actions.firmware-actions-current .firmware-cta, #firmware-actions.firmware-actions-current .firmware-cta:link, #firmware-actions.firmware-actions-current .firmware-cta:visited, #firmware-actions.firmware-actions-current .firmware-cta:hover, #firmware-actions.firmware-actions-current .firmware-cta:active { background: rgba(148, 163, 184, 0.16); border-color: rgba(148, 163, 184, 0.08); color:#9ca3af !important; box-shadow:none; }"));
          webClient.println(F("          </style>"));
          webClient.println(F("          <script type=\"text/javascript\">"));
          webClient.println(F("          var config = {"));
          webClient.println(F("              server: {"));
          webClient.print(F("                  uri: \"ws://"));
          webClient.print(mqtt_server_ip[0]);
          webClient.print(F("."));
          webClient.print(mqtt_server_ip[1]);
          webClient.print(F("."));
          webClient.print(mqtt_server_ip[2]);
          webClient.print(F("."));
          webClient.print(mqtt_server_ip[3]);
          webClient.println(":1884/\",");
          if(MQTT_LOGIN==true){
            webClient.print(F("                  username: \""));
            webClient.print(String(MQTT_USER));
            webClient.println(F("\","));
            webClient.print(F("                  password: \""));
            webClient.print(String(MQTT_PASS));
            webClient.println(F("\""));
          }
          webClient.println(F("              },"));
          // TOPIC
          webClient.print(F("              defaultTopic: \""));
          webClient.print(YOUR_CALL);
          webClient.print(F("/"));
          webClient.print(NET_ID);
          webClient.println(F("/ROT/#\","));
          // END TOPIC
          webClient.println(F("              showCounter: true,"));
          webClient.println(F("              alphabeticalSort: true,"));
          webClient.println(F("              qos: 0"));
          webClient.println(F("          };"));
          #if defined(OTAWEB)
            webClient.print(F("          var FirmwareRev = \""));
            webClient.print(REV);
            webClient.println(F("\";"));
            webClient.println(F("          var LatestReleaseTag = \"\";"));
            webClient.println(F("          function normalizeVersionDigits(versionText){"));
            webClient.println(F("            var digits = String(versionText || \"\").replace(/[^0-9]/g, \"\");"));
            webClient.println(F("            return digits.length ? digits : \"\";"));
            webClient.println(F("          }"));
            webClient.println(F("          function updateFirmwareActions(){"));
            webClient.println(F("            var wrap = document.getElementById(\"firmware-actions\");"));
            webClient.println(F("            var help = document.getElementById(\"firmware-update-help\");"));
            webClient.println(F("            var download = document.getElementById(\"firmware-download-btn\");"));
            webClient.println(F("            var upload = document.getElementById(\"firmware-upload-btn\");"));
            webClient.println(F("            if(!wrap || !help || !download || !upload){ return; }"));
            webClient.println(F("            wrap.style.display = \"block\";"));
            webClient.println(F("            wrap.classList.remove(\"firmware-actions-current\");"));
            webClient.println(F("            help.style.display = \"none\";"));
            webClient.println(F("            download.style.display = \"none\";"));
            webClient.println(F("            upload.style.display = \"none\";"));
            webClient.println(F("            if(!FirmwareRev || !LatestReleaseTag){"));
            webClient.println(F("              wrap.style.display = \"none\";"));
            webClient.println(F("              return;"));
            webClient.println(F("            }"));
            webClient.println(F("            var currentDigits = normalizeVersionDigits(FirmwareRev);"));
            webClient.println(F("            var latestDigits = normalizeVersionDigits(LatestReleaseTag);"));
            webClient.println(F("            if(!currentDigits || !latestDigits){"));
            webClient.println(F("              wrap.style.display = \"none\";"));
            webClient.println(F("              return;"));
            webClient.println(F("            }"));
            webClient.println(F("            if(Number(latestDigits) > Number(currentDigits)){"));
            webClient.println(F("              help.innerHTML = \"Download the latest two firmware files from the release page, then upload them on the web update page in the correct order: first firmware, then filesystem.\";"));
            webClient.println(F("              help.style.display = \"block\";"));
            webClient.println(F("              download.style.display = \"inline-block\";"));
            webClient.println(F("              upload.style.display = \"inline-block\";"));
            webClient.println(F("            }else{"));
            webClient.println(F("              help.innerHTML = \"Firmware is up to date\";"));
            webClient.println(F("              help.style.display = \"block\";"));
            webClient.println(F("              download.style.display = \"inline-block\";"));
            webClient.println(F("              upload.style.display = \"inline-block\";"));
            webClient.println(F("              wrap.classList.add(\"firmware-actions-current\");"));
            webClient.println(F("            }"));
            webClient.println(F("          }"));
            webClient.println(F("          function checkLatestRelease(){"));
            webClient.println(F("            if(!FirmwareRev){ return; }"));
            webClient.println(F("            var rhttp = new XMLHttpRequest();"));
            webClient.println(F("            rhttp.onreadystatechange = function() {"));
            webClient.println(F("              if (this.readyState == 4 && this.status == 200) {"));
            webClient.println(F("                try{"));
            webClient.println(F("                  var data = JSON.parse(this.responseText);"));
            webClient.println(F("                  LatestReleaseTag = String(data.tag_name || \"\");"));
            webClient.println(F("                  updateFirmwareActions();"));
            webClient.println(F("                }catch(e){}"));
            webClient.println(F("              }"));
            webClient.println(F("            };"));
            webClient.println(F("            rhttp.open(\"GET\", \"https://api.github.com/repos/ok1hra/IP-rotator/releases/latest\", true);"));
            webClient.println(F("            rhttp.send();"));
            webClient.println(F("          }"));
            webClient.println(F("          window.addEventListener(\"load\", function(){ checkLatestRelease(); });"));
          #endif
          webClient.println(F("          </script>"));
          // END TOPIC
          webClient.println(F("      </head>"));
          webClient.println(F("      <body>"));
          webClient.print(F("          <div id=\"frame\" "));
          webClient.println(F(">"));
          webClient.println(F("              <div id=\"footer\">"));
          webClient.println(F("                  <p class=\"status\" style=\"font-size: 150%;\">"));
          // STATUS
          webClient.print(F("Uptime: "));
          if(millis() < 60000){
            webClient.print(millis()/1000);
            webClient.print(F(" seconds"));
          }else if(millis() > 60000 && millis() < 3600000){
            webClient.print(millis()/60000);
            webClient.print(F(" minutes"));
          }else if(millis() > 3600000 && millis() < 86400000){
            webClient.print(millis()/3600000);
            webClient.print(F(" hours"));
          }else{
            webClient.print(millis()/86400000);
            webClient.print(F(" days"));
          }
          webClient.print(F(" | FW:&nbsp;"));
          webClient.println(REV);
          webClient.print(F("&nbsp;| HW:&nbsp;"));
          webClient.println(HardwareRev);
          webClient.print(F("&nbsp;| eth&nbsp;mac:&nbsp;"));
          webClient.print(MACString);
          webClient.println();

          webClient.print(F("&nbsp;| dhcp:&nbsp;"));
          if(DHCP_ENABLE==1){
            webClient.print(F("ON"));
          }else{
            webClient.print(F("OFF"));
          }
          webClient.print(F("&nbsp;| ip:&nbsp;"));
          webClient.println(ETH.localIP());
          // webClient.print(F(" | utc from ntp: "));
          // webClient.println(F("timeClient.getFormattedTime()"));
          // webClient.println(F("<br>MQTT subscribe command: $ mosquitto_sub -v -h mqttstage.prusa -t prusa-debug/prusafil/extrusionline/+/#"));
          webClient.print(F(" | MQTT&nbsp;"));
          if(MQTT_ENABLE==true){
            webClient.print(F("broker&nbsp;ip:&nbsp;"));
            webClient.print(mqtt_server_ip[0]);
            webClient.print(F("."));
            webClient.print(mqtt_server_ip[1]);
            webClient.print(F("."));
            webClient.print(mqtt_server_ip[2]);
            webClient.print(F("."));
            webClient.print(mqtt_server_ip[3]);
            webClient.print(F(":"));
            webClient.print(MQTT_PORT);
          }else{
            webClient.print(F("<span style='color: #F00; font-weight: bold;'>DISABLE</span>"));
          }
          // END STATUS
          webClient.println(F("              </p>"));
          #if defined(OTAWEB)
            webClient.println(F("              <div id=\"firmware-actions\">"));
            webClient.println(F("                  <div id=\"firmware-update-help\">Download the latest two firmware files from the release page, then upload them on the web update page in the correct order: first firmware, then filesystem.</div>"));
            webClient.println(F("                  <div class=\"firmware-actions-row\">"));
            webClient.println(F("                      <a id=\"firmware-download-btn\" class=\"firmware-cta\" href=\"https://github.com/ok1hra/IP-rotator/releases/latest\" target=\"_blank\">Download release</a>"));
            webClient.print(F("                  <a id=\"firmware-upload-btn\" href=\"http://"));
            webClient.print(ETH.localIP());
            webClient.println(F(":82/update\" target=\"_blank\" class=\"firmware-cta\">Upload firmware</a>"));
            webClient.println(F("                  </div>"));
            webClient.println(F("              </div>"));
          #endif
          webClient.print(F("              <div style=\"text-align:center;\">"));
          webClient.print(F("                  <a href=\"http://"));
          webClient.println(ETH.localIP());
          if(ELEVATION==false){
            webClient.print(F(":88\" onclick=\"window.open( this.href, this.href, 'width=620,height=750,left=0,top=0,menubar=no,location=no,status=no' ); return false;\"><button style='color:#fff; background-color:#060; padding:10px 22px; margin:15px 8px 0 8px; border:none; border-radius:6px; font-weight:bold; font-size:110%;'>Azimuth Map Control</button></a>"));
          }else{
            webClient.print(F(":88\" onclick=\"window.open( this.href, this.href, 'width=620,height=610,left=0,top=0,menubar=no,location=no,status=no' ); return false;\"><button style='color:#fff; background-color:#060; padding:10px 22px; margin:15px 8px 0 8px; border:none; border-radius:6px; font-weight:bold; font-size:110%;'>Elevation Map Control</button></a>"));
          }
          webClient.println(F("              </div>"));
          webClient.println(F("              </div>"));
          webClient.println(F("              <div id=\"header\">"));
          webClient.println(F("                  <div id=\"topic-box\">"));
          webClient.println(F("                      <input type=\"text\" id=\"topic\" value=\"\" title=\"Topic to subscribe\">"));
          webClient.println(F("                  </div>"));
          webClient.println(F("              </div>"));
          webClient.println(F("              <div id=\"toast\"></div>"));
          webClient.println(F("              <section class=\"messages\"></section>"));
          webClient.println(F("              <div id=\"footer\">"));
          webClient.println(F("                  <p class=\"status\">"));
          webClient.println(F("                      Client <code id=\"status-client\" title=\"Client ID\">?</code> is "));
          webClient.println(F("                      <code id=\"status-state\" class=\"connecting\"><em>&bull;</em> <span>connecting...</span></code> to "));
          webClient.println(F("                      <code id=\"status-host\">?</code>"));
          webClient.println(F("                      <em>via</em> MQTT Wall 0.3.0 (<a href=\"https://github.com/bastlirna/mqtt-wall\">github</a>)"));
          webClient.println(F("                      | <a href=\"https://remoteqth.com/wiki/\" target=\"_blank\">ROT Wiki</a>."));
          webClient.println(F("                  </p>"));
          webClient.println(F("              </div>"));
          webClient.println(F("          </div>"));
          webClient.println(F("          <script type=\"text/javascript\" src=\"https://code.jquery.com/jquery-2.1.4.min.js\"></script>"));
          webClient.println(F("          <script type=\"text/javascript\" src=\"https://code.jquery.com/color/jquery.color-2.1.2.min.js\"></script>"));
          webClient.println(F("          <script type=\"text/javascript\" src=\"https://cdnjs.cloudflare.com/ajax/libs/paho-mqtt/1.0.1/mqttws31.min.js\"></script>"));
          webClient.println(F("          <script type=\"text/javascript\" src=\"https://remoteqth.com/mqtt-wall/wall.js\"></script>"));
          webClient.println(F("          <script type=\"text/javascript\">"));
          webClient.println(F("            (function(){"));
          webClient.println(F("              function flashValue(node){"));
          webClient.println(F("                if(!node || !node.classList){ return; }"));
          webClient.println(F("                var message = node.closest ? node.closest(\".messages .message\") : null;"));
          webClient.println(F("                if(message){"));
          webClient.println(F("                  var header = message.querySelector(\"header\");"));
          webClient.println(F("                  var start = header ? Math.max(0, Math.ceil(header.getBoundingClientRect().width)) : 0;"));
          webClient.println(F("                  message.style.setProperty(\"--ipr-flash-start\", start + \"px\");"));
          webClient.println(F("                  message.classList.remove(\"ipr-flash\");"));
          webClient.println(F("                }"));
          webClient.println(F("                node.classList.remove(\"ipr-flash\");"));
          webClient.println(F("                void node.offsetWidth;"));
          webClient.println(F("                if(message){"));
          webClient.println(F("                  void message.offsetWidth;"));
          webClient.println(F("                  message.classList.add(\"ipr-flash\");"));
          webClient.println(F("                }"));
          webClient.println(F("                node.classList.add(\"ipr-flash\");"));
          webClient.println(F("              }"));
          webClient.println(F("              function resolvePayloadNode(node){"));
          webClient.println(F("                if(!node){ return null; }"));
          webClient.println(F("                if(node.nodeType === 3){ node = node.parentElement; }"));
          webClient.println(F("                if(!node || !node.closest){ return null; }"));
          webClient.println(F("                return node.matches(\".messages .message p\") ? node : node.closest(\".messages .message p\");"));
          webClient.println(F("              }"));
          webClient.println(F("              function watchMessages(){"));
          webClient.println(F("                var host = document.querySelector(\".messages\");"));
          webClient.println(F("                if(!host){ window.setTimeout(watchMessages, 250); return; }"));
          webClient.println(F("                var lastValues = new WeakMap();"));
          webClient.println(F("                function markIfChanged(node){"));
          webClient.println(F("                  var payload = resolvePayloadNode(node);"));
          webClient.println(F("                  if(!payload){ return; }"));
          webClient.println(F("                  var next = payload.textContent;"));
          webClient.println(F("                  if(!next){ return; }"));
          webClient.println(F("                  var prev = lastValues.get(payload);"));
          webClient.println(F("                  if(prev === undefined){"));
          webClient.println(F("                    lastValues.set(payload, next);"));
          webClient.println(F("                    return;"));
          webClient.println(F("                  }"));
          webClient.println(F("                  if(prev !== next){"));
          webClient.println(F("                    lastValues.set(payload, next);"));
          webClient.println(F("                    flashValue(payload);"));
          webClient.println(F("                  }"));
          webClient.println(F("                }"));
          webClient.println(F("                host.querySelectorAll(\".message p\").forEach(function(payload){"));
          webClient.println(F("                  lastValues.set(payload, payload.textContent);"));
          webClient.println(F("                });"));
          webClient.println(F("                var observer = new MutationObserver(function(mutations){"));
          webClient.println(F("                  mutations.forEach(function(mutation){"));
          webClient.println(F("                    if(mutation.type === \"characterData\"){"));
          webClient.println(F("                      markIfChanged(mutation.target);"));
          webClient.println(F("                      return;"));
          webClient.println(F("                    }"));
          webClient.println(F("                    if(mutation.type === \"childList\"){"));
          webClient.println(F("                      if(mutation.target){ markIfChanged(mutation.target); }"));
          webClient.println(F("                      Array.prototype.forEach.call(mutation.addedNodes, function(added){"));
          webClient.println(F("                        var payload = resolvePayloadNode(added);"));
          webClient.println(F("                        if(payload){"));
          webClient.println(F("                          lastValues.set(payload, payload.textContent);"));
          webClient.println(F("                          flashValue(payload);"));
          webClient.println(F("                        }"));
          webClient.println(F("                      });"));
          webClient.println(F("                    }"));
          webClient.println(F("                  });"));
          webClient.println(F("                });"));
          webClient.println(F("                observer.observe(host, { childList: true, subtree: true, characterData: true });"));
          webClient.println(F("              }"));
          webClient.println(F("              if(document.readyState === \"loading\"){"));
          webClient.println(F("                document.addEventListener(\"DOMContentLoaded\", watchMessages);"));
          webClient.println(F("              }else{"));
          webClient.println(F("                watchMessages();"));
          webClient.println(F("              }"));
          webClient.println(F("            })();"));
          webClient.println(F("          </script>"));
          webClient.println(F("      </body>"));
          webClient.println(F("  </html>"));

          if(EnableSerialDebug>0){
            Serial.print(HTTP_req);
          }
          HTTP_req = "";

          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          // if (strstr(linebuf,"GET /h0 ") > 0){digitalWrite(GPIOS[0], HIGH);}else if (strstr(linebuf,"GET /l0 ") > 0){digitalWrite(GPIOS[0], LOW);}
          // else if (strstr(linebuf,"GET /h1 ") > 0){digitalWrite(GPIOS[1], HIGH);}else if (strstr(linebuf,"GET /l1 ") > 0){digitalWrite(GPIOS[1], LOW);}

          // you're starting a new line
          currentLineIsBlank = true;
          memset(linebuf,0,sizeof(linebuf));
          charcount=0;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    webClient.stop();
   if(EnableSerialDebug>0){
     Serial.println("WIFI webClient disconnected");
     MeasureTimer[0]=millis()+5000-MeasureTimer[1];
   }
  }
}
//-------------------------------------------------------------------------------------------------------

void EthEvent(WiFiEvent_t event)
{
  switch (event) {
    // case SYSTEM_EVENT_ETH_START:
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH  Started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;
    // case SYSTEM_EVENT_ETH_CONNECTED:
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH  link up");
      break;
    // case SYSTEM_EVENT_ETH_GOT_IP:
    case ARDUINO_EVENT_ETH_GOT_IP:
      MACString = ETH.macAddress();
      MACString.toCharArray( MACchar, 18 );
      Serial.print("ETH  MAC: ");
      Serial.println(MACString);
      // Serial.println("===============================");
      // Serial.print("   IPv4: ");
      // Serial.println(ETH.localIP());
      // Serial.println("===============================");
      if (ETH.fullDuplex()) {
        Serial.print("FULL_DUPLEX, ");
      }
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;

      #if defined(MQTT)
        if(MQTT_ENABLE == true){
          Serial.print("EthEvent-mqtt ");
          mqttClient.setServer(mqtt_server_ip, MQTT_PORT);
          mqttClient.setCallback(MqttRx);
          lastMqttReconnectAttempt = 0;
          // Check if this is a cold start
          // If so then read in YOUR_CALL from EEPROM
          if (YOUR_CALL.isEmpty()){
            // EEPROM YOUR_CALL
            if(EEPROM.read(141)==0xff){
              YOUR_CALL=MACString;
              YOUR_CALL.remove(0, 12);
              }else{
                YOUR_CALL = ReadFixedStringFromEeprom(141, 20);
              }
          }

          char charbuf[50];
           // // memcpy( charbuf, ETH.macAddress(), 6);
           // ETH.macAddress().toCharArray(charbuf, 18);
           // // charbuf[6] = 0;
          if(MQTT_LOGIN == true){
            if (mqttClient.connect(MACchar,MQTT_USER.c_str(),MQTT_PASS.c_str())){
              Prn(0, 1, String(MACchar));
              mqttReconnect();
              AfterMQTTconnect();
              Prn(0, 1, "http://"+String(ETH.localIP()[0])+"."+String(ETH.localIP()[1])+"."+String(ETH.localIP()[2])+"."+String(ETH.localIP()[3]) );
              if(BaudRate!=115200){
                MqttPubString("USB-BaudRate", String(BaudRate), true);
                Serial.println("Baudrate change to "+String(BaudRate)+"...");
                Serial.flush();
                // Serial.end();
                delay(1000);
                Serial.begin(BaudRate);
                delay(500);
                Serial.println();
                Serial.println();
                Serial.println("New Baudrate "+String(BaudRate));
                Prn(0, 1, "http://"+String(ETH.localIP()[0])+"."+String(ETH.localIP()[1])+"."+String(ETH.localIP()[2])+"."+String(ETH.localIP()[3]) );
              }
            }
          }else{
            if (mqttClient.connect(MACchar)){
              Prn(0, 1, String(MACchar));
              mqttReconnect();
              AfterMQTTconnect();
              Prn(0, 1, "http://"+String(ETH.localIP()[0])+"."+String(ETH.localIP()[1])+"."+String(ETH.localIP()[2])+"."+String(ETH.localIP()[3]) );
              if(BaudRate!=115200){
                MqttPubString("USB-BaudRate", String(BaudRate), true);
                Serial.println("Baudrate change to "+String(BaudRate)+"...");
                Serial.flush();
                // Serial.end();
                delay(1000);
                Serial.begin(BaudRate);
                delay(500);
                Serial.println();
                Serial.println();
                Serial.println("New Baudrate "+String(BaudRate));
                Prn(0, 1, "http://"+String(ETH.localIP()[0])+"."+String(ETH.localIP()[1])+"."+String(ETH.localIP()[2])+"."+String(ETH.localIP()[3]) );
              }
            }
          }
        }
      #endif
      // ListCommands(0);
      break;

    // case SYSTEM_EVENT_ETH_DISCONNECTED:
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH  Disconnected");
      eth_connected = false;
      break;
    // case SYSTEM_EVENT_ETH_STOP:
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH  Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}
//-------------------------------------------------------------------------------------------------------
void Mqtt(){
  if (millis()-MqttStatusTimer[0]>MqttStatusTimer[1] && MQTT_ENABLE == true && eth_connected==1){
    if(!mqttClient.connected()){
      long now = millis();
      if (now - lastMqttReconnectAttempt > 5000) {
        lastMqttReconnectAttempt = now;
        if(EnableSerialDebug>0){
          Prn(3, 1, "Attempt to MQTT reconnect | "+String(millis()/1000) );
        }
        if (mqttReconnect()) {
          lastMqttReconnectAttempt = 0;
        }
      }
    }else{
      // Client connected
      mqttClient.loop();
    }
    MqttStatusTimer[0]=millis();
  }
}

//-------------------------------------------------------------------------------------------------------

bool mqttReconnect() {
  // Prn(3, 0, "MQTT");
  char charbuf[50];
  // // memcpy( charbuf, ETH.macAddress(), 6);
  // ETH.macAddress().toCharArray(charbuf, 18);
  // charbuf[6] = 0;
  if(MQTT_LOGIN == true){
    if (mqttClient.connect(MACchar,MQTT_USER.c_str(),MQTT_PASS.c_str())){
      if(EnableSerialDebug>0){
        Prn(3, 0, "mqttReconnect-connected");
      }
      reSubscribe();
    }
  }else{
    if (mqttClient.connect(MACchar)) {
      if(EnableSerialDebug>0){
        Prn(3, 0, "mqttReconnect-connected");
      }
      // IPAddress IPlocalAddr = ETH.localIP();                           // get
      // String IPlocalAddrString = String(IPlocalAddr[0]) + "." + String(IPlocalAddr[1]) + "." + String(IPlocalAddr[2]) + "." + String(IPlocalAddr[3]);   // to string
      // MqttPubStringQC(1, "IP", IPlocalAddrString, true);
      reSubscribe();
    }
  }
  return mqttClient.connected();
}

//------------------------------------------------------------------------------------
void reSubscribe(){
    String topic = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/Target";
    const char *cstr = topic.c_str();
    if(mqttClient.subscribe(cstr)==true){
      if(EnableSerialDebug>0){
        Prn(3, 1, " > subscribe "+String(cstr));
      }
    }
    topic = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/get";
    const char *cstr1 = topic.c_str();
    if(mqttClient.subscribe(cstr1)==true){
      if(EnableSerialDebug>0){
        Prn(3, 1, " > subscribe "+String(cstr1));
      }
    }
    topic = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/stop";
    const char *cstr2 = topic.c_str();
    if(mqttClient.subscribe(cstr2)==true){
      if(EnableSerialDebug>0){
        Prn(3, 1, " > subscribe "+String(cstr2));
      }
    }
    topic = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/RxAzimuth";
    const char *cstr3 = topic.c_str();
    if(mqttClient.subscribe(cstr3)==true){
      if(EnableSerialDebug>0){
        Prn(3, 1, " > subscribe "+String(cstr3));
      }
    }
}

//------------------------------------------------------------------------------------
void MqttRx(char *topic, byte *payload, unsigned int length) {
  String CheckTopicBase;
  CheckTopicBase.reserve(100);
  byte* p = (byte*)malloc(length);
  memcpy(p,payload,length);
  // static bool HeardBeatStatus;
  if(EnableSerialDebug>0){
    Prn(3, 0, "RX MQTT ");
  }

    // RxAzimuth
    CheckTopicBase = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/RxAzimuth";
    if ( CheckTopicBase.equals( String(topic) ) ){
      RxAzimuth = 0;
      unsigned long exp = 1;
      for (int i = length-1; i >=0 ; i--) {
        // Numbers only
        if(p[i]>=48 && p[i]<=58){
          RxAzimuth = RxAzimuth + (p[i]-48)*exp;
          exp = exp*10;
        }
      }
      if(AZsource == 2){ //MQTT
        Azimuth=RxAzimuth;
      }
        Prn(3, 1, "/RxAzimuth " + String(RxAzimuth));
    }

    CheckTopicBase = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/stop";
    if ( CheckTopicBase.equals( String(topic) )){
      if(EnableSerialDebug>0){
        Prn(3, 1, "/stop ");
      }
      RequestStopRamp(true);
    }

    CheckTopicBase = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/get";
    if ( CheckTopicBase.equals( String(topic) )){
      MqttPubString("Azimuth", String(Azimuth), false);
      if(Status==0){
        MqttPubString("AzimuthStop", String(Azimuth), false);
        MqttPubString("StartAzimuth", String(StartAzimuth), false);
        MqttPubString("Name", String(RotName), false);
      }
      TxMqttAzimuthTimer=millis();
      if(EnableSerialDebug>0){
        Prn(3, 1, "/get ");
      }
    }

    // Target
    CheckTopicBase = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/Target";
    if ( CheckTopicBase.equals( String(topic) ) ){
      AzimuthTarget = 0;
      unsigned long exp = 1;
      for (int i = length-1; i >=0 ; i--) {
        // Numbers only
        if(p[i]>=48 && p[i]<=58){
          AzimuthTarget = AzimuthTarget + (p[i]-48)*exp;
          exp = exp*10;
        }
      }
      UiTargetAzimuth = AzimuthTarget + StartAzimuth;
      if(UiTargetAzimuth > 359){
        UiTargetAzimuth = UiTargetAzimuth - 360;
      }
      RotCalculate();
    }

} // MqttRx END

//-------------------------------------------------------------------------------------------------------
void AfterMQTTconnect(){
  #if defined(MQTT)
  //    if (mqttClient.connect("esp32gwClient", MQTT_USER, MQTT_PASS)) {          // public IP addres to MQTT
        IPAddress IPlocalAddr = ETH.localIP();                           // get
        String IPlocalAddrString = String(IPlocalAddr[0]) + "." + String(IPlocalAddr[1]) + "." + String(IPlocalAddr[2]) + "." + String(IPlocalAddr[3]);   // to string
        IPlocalAddrString.toCharArray( mqttTX, 50 );                          // to array
        String path2 = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/ip";
        path2.toCharArray( mqttPath, 100 );
        mqttClient.publish(mqttPath, mqttTX, true);
          Serial.print("MQTT-TX ");
          Serial.print(mqttPath);
          Serial.print(" ");
          Serial.println(mqttTX);

        // String MAClocalAddrString = ETH.macAddress();   // to string
        // MAClocalAddrString.toCharArray( mqttTX, 50 );                          // to array
        path2 = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/mac";
        path2.toCharArray( mqttPath, 100 );
        mqttClient.publish(mqttPath, MACchar, true);
          Serial.print("MQTT-TX ");
          Serial.print(mqttPath);
          Serial.print(" ");
          Serial.println(MACchar);

        // MqttPubString("StartAzimuth", String(StartAzimuth), true);
        // MqttPubString("Name", String(RotName), true);
        MqttPubString("PwmMaxDuty", String(PwmMaxDuty), true);
        MqttPubString("PwmSlowWindowDeg", String(PwmSlowWindowDeg, 1), true);
        MqttPubString("PwmTuneAggressiveness", String(PwmTuneAggressiveness + 1), true);
        MqttPubString("StopToleranceDeg", String(GetStopToleranceDeg(), 2), true);

        // String pcbString = String(HWREV);   // to string
        // pcbString.toCharArray( mqttTX, 2 );                          // to array
        // path2 = String(YOUR_CALL) + "/WX/pcb";
        // path2.toCharArray( mqttPath, 100 );
        //   mqttClient.publish(mqttPath, mqttTX, true);
            // Serial.print("MQTT-TX ");
            // Serial.print(mqttPath);
            // Serial.print(" ");
            // Serial.println(mqttTX);

    // MeasureTimer[0]=2800000;
    MeasureTimer[0]=millis()-MeasureTimer[1];

  #endif
}
//-----------------------------------------------------------------------------------
void MqttPubString(String TOPIC, String DATA, bool RETAIN){
  char charbuf[50];
   // // memcpy( charbuf, mac, 6);
   // ETH.macAddress().toCharArray(charbuf, 10);
   // charbuf[6] = 0;
  // if(EnableEthernet==1 && MQTT_ENABLE==1 && EthLinkStatus==1 && mqttClient.connected()==true){
  if(mqttClient.connected()==true){
    if(MQTT_LOGIN == true){
      if (mqttClient.connect(MACchar,MQTT_USER.c_str(),MQTT_PASS.c_str())){
        String topic = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/"+TOPIC;
        topic.toCharArray( mqttPath, 50 );
        DATA.toCharArray( mqttTX, 50 );
        mqttClient.publish(mqttPath, mqttTX, RETAIN);
      }
    }else{
      if (mqttClient.connect(MACchar)) {
        String topic = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/"+TOPIC;
        topic.toCharArray( mqttPath, 50 );
        DATA.toCharArray( mqttTX, 50 );
        mqttClient.publish(mqttPath, mqttTX, RETAIN);
      }
    }
  }
}
//-------------------------------------------------------------------------------------------------------
String UtcTime(int format){
  tm timeinfo;
  char buf[50]; //50 chars should be enough
  if (eth_connected==false) {
    strcpy(buf, "n/a");
  }else{
    if(!getLocalTime(&timeinfo)){
      strcpy(buf, "n/a");
    }else{
      if(format==1){
        strftime(buf, sizeof(buf), "%Y-%b-%d %H:%M:%S", &timeinfo);
      }else if(format==2){
        strftime(buf, sizeof(buf), "%d", &timeinfo);
      }else if(format==3){
        strftime(buf, sizeof(buf), "%Y", &timeinfo);
      }
    }
  }
  // Serial.println(buf);
  return String(buf);
}

void InitWebAuth(){
  WebDigestNonce = DigestRandomHex();
  WebDigestOpaque = DigestRandomHex();
}

String Md5Hex(const String& value){
  MD5Builder md5;
  md5.begin();
  md5.add(value);
  md5.calculate();
  return md5.toString();
}

String DigestRandomHex(){
  char buffer[33];
  for(int n=0; n<4; n++){
    sprintf(buffer + (n*8), "%08x", esp_random());
  }
  buffer[32] = '\0';
  return String(buffer);
}

String ExtractHttpHeader(const String& request, const String& headerName){
  String needle = "\n" + headerName + ":";
  int start = request.indexOf(needle);
  if(start < 0){
    needle = "\n" + headerName;
    needle.toLowerCase();
    String lowerRequest = request;
    lowerRequest.toLowerCase();
    start = lowerRequest.indexOf(needle + ":");
    if(start < 0){
      return "";
    }
  }
  start = request.indexOf(':', start);
  if(start < 0){
    return "";
  }
  start++;
  int end = request.indexOf('\n', start);
  if(end < 0){
    end = request.length();
  }
  String value = request.substring(start, end);
  value.trim();
  return value;
}

String ExtractDigestParam(const String& authHeader, const String& name, char delimiter){
  String quotedNeedle = name + "=\"";
  int start = authHeader.indexOf(quotedNeedle);
  if(start >= 0){
    start += quotedNeedle.length();
    int end = authHeader.indexOf('"', start);
    if(end < 0){
      return "";
    }
    return authHeader.substring(start, end);
  }
  String rawNeedle = name + "=";
  start = authHeader.indexOf(rawNeedle);
  if(start < 0){
    return "";
  }
  start += rawNeedle.length();
  int end = authHeader.indexOf(delimiter, start);
  if(end < 0){
    end = authHeader.length();
  }
  String value = authHeader.substring(start, end);
  value.trim();
  return value;
}

bool CheckDigestAuthorization(const String& authHeader, const String& method, const String& uri){
  if(!WebAuthEnabled){
    return true;
  }
  if(!authHeader.startsWith("Digest ")){
    return false;
  }
  String username = ExtractDigestParam(authHeader, "username", ',');
  String realm = ExtractDigestParam(authHeader, "realm", ',');
  String nonce = ExtractDigestParam(authHeader, "nonce", ',');
  String requestUri = ExtractDigestParam(authHeader, "uri", ',');
  String response = ExtractDigestParam(authHeader, "response", ',');
  String opaque = ExtractDigestParam(authHeader, "opaque", ',');
  String qop = ExtractDigestParam(authHeader, "qop", ',');
  String nc = ExtractDigestParam(authHeader, "nc", ',');
  String cnonce = ExtractDigestParam(authHeader, "cnonce", ',');

  if(username != String(WEB_AUTH_USER) || realm != String(WEB_AUTH_REALM) || nonce != WebDigestNonce || opaque != WebDigestOpaque){
    return false;
  }
  if(requestUri.length() == 0 || response.length() == 0){
    return false;
  }
  String h1 = Md5Hex(String(WEB_AUTH_USER) + ":" + WEB_AUTH_REALM + ":" + WebAuthPassword);
  String h2 = Md5Hex(method + ":" + requestUri);
  String expected;
  if(qop == "auth"){
    if(nc.length() == 0 || cnonce.length() == 0){
      return false;
    }
    expected = Md5Hex(h1 + ":" + nonce + ":" + nc + ":" + cnonce + ":auth:" + h2);
  }else{
    expected = Md5Hex(h1 + ":" + nonce + ":" + h2);
  }
  return response == expected;
}

void SendHttpDigestChallenge(WiFiClient& webClient){
  webClient.println(F("HTTP/1.1 401 Unauthorized"));
  webClient.print(F("WWW-Authenticate: Digest realm=\""));
  webClient.print(WEB_AUTH_REALM);
  webClient.print(F("\", qop=\"auth\", nonce=\""));
  webClient.print(WebDigestNonce);
  webClient.print(F("\", opaque=\""));
  webClient.print(WebDigestOpaque);
  webClient.println(F("\""));
  webClient.println(F("Content-Type: text/plain"));
  webClient.println(F("Connection: close"));
  webClient.println();
  webClient.println(F("Authentication required"));
}

bool AjaxAuthOk(){
  if(!WebAuthEnabled){
    return true;
  }
  return ajaxserver.authenticate(WEB_AUTH_USER, WebAuthPassword.c_str());
}

bool RequireAjaxAuth(){
  if(AjaxAuthOk()){
    return true;
  }
  ajaxserver.requestAuthentication(DIGEST_AUTH, WEB_AUTH_REALM, "Authentication required");
  return false;
}

void RegisterAjaxRoute(const char* uri, WebServer::THandlerFunction handler){
  ajaxserver.on(uri, [handler](){
    if(!RequireAjaxAuth()){
      return;
    }
    handler();
  });
}

void RegisterAjaxRoute(const char* uri, HTTPMethod method, WebServer::THandlerFunction handler){
  ajaxserver.on(uri, method, [handler](){
    if(!RequireAjaxAuth()){
      return;
    }
    handler();
  });
}

void RegisterAjaxUploadRoute(const char* uri, HTTPMethod method, WebServer::THandlerFunction handler, WebServer::THandlerFunction uploadHandler){
  ajaxserver.on(uri, method, [handler](){
    if(!RequireAjaxAuth()){
      return;
    }
    handler();
  }, [uploadHandler](){
    if(!AjaxAuthOk()){
      return;
    }
    uploadHandler();
  });
}

// ajax rx
void handlePostRot() {
 // String s = MAIN_page; //Read HTML contents
 String str = ajaxserver.arg("ROT");
 if(str.length()<1){
   ajaxserver.send(400, "text/plain", "ROTATE REQUEST FAILED");
   return;
 }
 int requestedAzimuth = str.toInt();
 if(requestedAzimuth < 0 || requestedAzimuth > 359){
   ajaxserver.send(400, "text/plain", "ROTATE REQUEST FAILED");
   return;
 }
 if(Status==0){
   AzimuthTarget = requestedAzimuth - StartAzimuth;
   if(AzimuthTarget<0){
       AzimuthTarget = 360+AzimuthTarget;
   }
   UiTargetAzimuth = requestedAzimuth;
   MqttPubString("AzimuthTarget", String(AzimuthTarget), false);
   RotCalculate();
   ajaxserver.send(200, "text/plain", "ROTATE REQUEST ACCEPTED");
 }else{
   RequestStopRamp(true);
   ajaxserver.send(409, "text/plain", "ROTATE REQUEST FAILED");
  }
}

void handleSet() {

  String yourcallERR= "";
  String rotidERR= "";
  String rotnameERR= "";
  String startazimuthERR= "";
  String startazimuthSTYLE= "";
  String startazimuthDisable= "";
  String maxrotatedegreeERR= "";
  String antradiationangleERR= "";
  String oneturnlimitsecERR= "";
  String pulseperdegreeERR= "";
  String pulseperdegreeSTYLE= "";
  String pwmenableSTYLE= "";
  String twowireSTYLE= "";
  String pulseperdegreeDisable= "";
  String pwmenableDisable= "";
  String twowireDisable= "";
  String mapurlERR= "";
  String mqttERR= "";
  String mqttportERR= "";
  String edstoplowzoneERR= "";
  String edstoplowzoneSTYLE= "";
  String edstoplowzoneDisable= "";
  String edstophighzoneERR= "";
  String edstophighzoneSTYLE= "";
  String edstophighzoneDisable= "";
  String edstopsCHECKED= "";
  String edstopsSTYLE= "";
  String acmotorCHECKED= "";
  String motorSELECT0= "";
  String motorSELECT1= "";
  String pwmSELECT0= "";
  String pwmSELECT1= "";
  String sourceSELECT0= "";
  String sourceSELECT1= "";
  String sourceSELECT2= "";
  String baudSELECT0= "";
  String baudSELECT1= "";
  String baudSELECT2= "";
  String baudSELECT3= "";
  String baudSELECT4= "";
  String twowireSELECT0= "";
  String twowireSELECT1= "";
  String preampSELECT0= "";
  String preampSELECT1= "";
  String pwmrampstepsERR= "";
  String pwmrampstepsSTYLE= "";
  String pwmrampstepsDisable= "";
  String pwmtuneaggrERR= "";
  String pwmtuneaggrSTYLE= "";
  String pwmtuneaggrDisable= "";
  String pwmtuneaggrSELECT0= "";
  String pwmtuneaggrSELECT1= "";
  String pwmtuneaggrSELECT2= "";
  String pwmtuneaggrSELECT3= "";
  String pwmtuneaggrSELECT4= "";
  String mqtt_loginSTYLE= "";
  String mqtt_loginCHECKED= "";
  String elevationCHECKED= "";
  String mqtt_userSTYLE= "";
  String mqtt_userERR= "";
  String mqtt_passSTYLE= "";
  String mqtt_passERR= "";
  String webpassERR= "";
  String mqtt_loginDisable= "";
  String dxc_hostERR= "";
  String dxc_portERR= "";
  String dxc_callsignERR= "";
  String mapsourceERR= "";
  String maplocatorERR= "";
  String mapzoomkmERR= "";
  String graylinentpERR= "";
  String graylinedarknessERR= "";
  String mapthemeERR= "";
  String mapdefaultdegCHECKED= "";
  String mapdefaultantCHECKED= "";
  String mapdefaultlocgridCHECKED= "";
  String mapdefaultgraylineCHECKED= "";
  String mapdefaultbordersCHECKED= "";
  String mapdefaultdxccCHECKED= "";
  String mapSourceSELECT0= "";
  String mapSourceSELECT1= "";
  String mapUrlRowStyle= "";
  String mapLocatorRowStyle= "";
  String mapZoomRowStyle= "";
  String mapThemeRowStyle= "";
  String graylineNtpRowStyle= "";
  String graylineDarknessRowStyle= "";
  String mapThemeSELECT0= "";
  String mapThemeSELECT1= "";
  String mapThemeSELECT2= "";
  String mapThemeSELECT3= "";
  String mapThemeSELECT4= "";
  String mapThemeSELECT5= "";
  String webauthCHECKED= "";
  byte previousAzSource = AZsource;
  short previousPulsePerDegree = PulsePerDegree;
  unsigned int previousMaxRotateDegree = MaxRotateDegree;

  if ( ajaxserver.hasArg("yourcall") == false \
    && ajaxserver.hasArg("rotid") == false \
    && ajaxserver.hasArg("rotname") == false \
    && ajaxserver.hasArg("startazimuth") == false \
    && ajaxserver.hasArg("maxrotatedegree") == false \
    && ajaxserver.hasArg("mapurl") == false \
    && ajaxserver.hasArg("antradiationangle") == false \
    && ajaxserver.hasArg("edstoplowzone") == false \
    && ajaxserver.hasArg("edstophighzone") == false \
    && ajaxserver.hasArg("pwmrampsteps") == false \
    && ajaxserver.hasArg("pwmtuneaggr") == false \
    && ajaxserver.hasArg("mapsource") == false \
    && ajaxserver.hasArg("maplocator") == false \
    && ajaxserver.hasArg("mapzoomkm") == false \
    && ajaxserver.hasArg("maptheme") == false \
    && ajaxserver.hasArg("graylinentp") == false \
    && ajaxserver.hasArg("graylinedarkness") == false \
    && ajaxserver.hasArg("dxchost") == false \
    && ajaxserver.hasArg("dxcport") == false \
    && ajaxserver.hasArg("dxccall") == false \
    && ajaxserver.hasArg("webauth") == false \
    && ajaxserver.hasArg("webpass") == false \
  ) {
    // MqttPubString("Debug", "Form not valid", false);
  }else{
    // MqttPubString("Debug", "Form valid", false);

    // YOUR_CALL
    if ( ajaxserver.arg("yourcall").length()<1 || ajaxserver.arg("yourcall").length()>20){
      yourcallERR= " Out of range 1-20 characters";
    }else{
      String str = String(ajaxserver.arg("yourcall"));
      if(YOUR_CALL == str){
        yourcallERR="";
      }else{
        yourcallERR=" Warning: MQTT topic has changed.";
        YOUR_CALL = String(ajaxserver.arg("yourcall"));

        int str_len = str.length();
        char char_array[str_len+1];
        str.toCharArray(char_array, str_len+1);
        for (int i=0; i<20; i++){
          if(i < str_len){
            EEPROM.write(141+i, char_array[i]);
          }else{
            EEPROM.write(141+i, 0xff);
          }
        }
        // EEPROM.commit();
      }
    }

    // NET_ID
    if ( ajaxserver.arg("rotid").length()<1 || ajaxserver.arg("rotid").length()>2){
      rotidERR= " Out of range 1-2 characters";
    }else{
      String str = String(ajaxserver.arg("rotid"));
      if(NET_ID == str){
        rotidERR="";
      }else{
        rotidERR=" Warning: MQTT topic has changed.";
        NET_ID = String(ajaxserver.arg("rotid"));

        int str_len = str.length();
        char char_array[str_len+1];
        str.toCharArray(char_array, str_len+1);
        for (int i=0; i<2; i++){
          if(i < str_len){
            EEPROM.write(i, char_array[i]);
          }else{
            EEPROM.write(i, 0xff);
          }
        }
        // EEPROM.commit();
      }
    }

    // RotName
    if ( ajaxserver.arg("rotname").length()<1 || ajaxserver.arg("rotname").length()>20){
      rotnameERR= " Out of range 1-20 characters";
    }else{
      String str = String(ajaxserver.arg("rotname"));
      if(RotName == str){
        rotnameERR="";
      }else{
        rotnameERR="";
        RotName = String(ajaxserver.arg("rotname"));

        int str_len = str.length();
        char char_array[str_len+1];
        str.toCharArray(char_array, str_len+1);
        for (int i=0; i<20; i++){
          if(i < str_len){
            EEPROM.write(2+i, char_array[i]);
          }else{
            EEPROM.write(2+i, 0xff);
          }
        }
        // EEPROM.commit();
        MqttPubString("Name", String(RotName), true);
      }
    }

    // 236-245 - MQTT_USER
    if ( ajaxserver.arg("mqttuser").length()<1 || ajaxserver.arg("mqttuser").length()>10){
      if(MQTT_LOGIN==true){
        mqtt_userERR= " Out of range 1-10 characters";
      }else{
        mqtt_userERR= "";
      }
    }else{
      String str = String(ajaxserver.arg("mqttuser"));
      if(MQTT_USER == str){
        mqtt_userERR="";
      }else{
        mqtt_userERR=" Must restart after change!";
        MQTT_USER = String(ajaxserver.arg("mqttuser"));

        int str_len = str.length();
        char char_array[str_len+1];
        str.toCharArray(char_array, str_len+1);
        for (int i=0; i<10; i++){
          if(i < str_len){
            EEPROM.write(236+i, char_array[i]);
          }else{
            EEPROM.write(236+i, 0xff);
          }
        }
        MqttPubString("MQTTuser", String(MQTT_USER), true);
      }
    }

    // 246-265 - MQTT_PASS
    if ( ajaxserver.arg("mqttpass").length()<1 || ajaxserver.arg("mqttpass").length()>20){
      if(MQTT_LOGIN==true){
        mqtt_passERR= " Out of range 1-20 characters";
      }else{
        mqtt_passERR= "";
      }
    }else{
      String str = String(ajaxserver.arg("mqttpass"));
      if(MQTT_PASS == str){
        mqtt_passERR="";
      }else{
        mqtt_passERR=" Must restart after change!";
        MQTT_PASS = String(ajaxserver.arg("mqttpass"));

        int str_len = str.length();
        char char_array[str_len+1];
        str.toCharArray(char_array, str_len+1);
        for (int i=0; i<20; i++){
          if(i < str_len){
            EEPROM.write(246+i, char_array[i]);
          }else{
            EEPROM.write(246+i, 0xff);
          }
        }
      }
    }

    // 41-140 - WebAuthPassword
    if ( ajaxserver.arg("webpass").length()>100){
      webpassERR= " Out of range 0-100 characters";
    }else if(!IsSafeWebAuthPasswordValue(ajaxserver.arg("webpass"))){
      webpassERR= " Unsupported chars: avoid quotes, <, > and &";
    }else{
      String str = String(ajaxserver.arg("webpass"));
      if(str.length() < 1 && ajaxserver.arg("webauth").toInt()==1){
        webpassERR= " Password is required when web authentication is enabled";
      }else if(WebAuthPassword == str){
        webpassERR="";
      }else{
        webpassERR="";
        WebAuthPassword = str;
        EEPROM.writeByte(233, 0xA5);
        WriteFixedStringToEeprom(41, 100, WebAuthPassword);
      }
    }

    String newDxcHost = String(ajaxserver.arg("dxchost"));
    String newDxcCallsign = String(ajaxserver.arg("dxccall"));
    newDxcHost.trim();
    newDxcCallsign.trim();
    newDxcCallsign.toUpperCase();

    if(newDxcHost.length() > 63){
      dxc_hostERR = " Out of range 0-63 characters";
    }else{
      dxc_hostERR = "";
      if(DxcHost != newDxcHost){
        DxcHost = newDxcHost;
        WriteFixedStringToEeprom(331, 64, DxcHost);
      }
    }

    if ( ajaxserver.arg("dxcport").length()<1 || ajaxserver.arg("dxcport").toInt()<1 || ajaxserver.arg("dxcport").toInt()>65535){
      dxc_portERR = " Out of range number 1-65535";
    }else{
      dxc_portERR = "";
      uint16_t newDxcPort = uint16_t(ajaxserver.arg("dxcport").toInt());
      if(DxcPort != newDxcPort){
        DxcPort = newDxcPort;
        EEPROM.writeUShort(395, DxcPort);
      }
    }

    if(newDxcHost.length() > 0 && (newDxcCallsign.length() < 1 || newDxcCallsign.length() > 16)){
      dxc_callsignERR = " Out of range 1-16 characters";
    }else if(newDxcHost.length() == 0 && newDxcCallsign.length() > 16){
      dxc_callsignERR = " Out of range 0-16 characters";
    }else{
      dxc_callsignERR = "";
      if(DxcCallsign != newDxcCallsign){
        DxcCallsign = newDxcCallsign;
        WriteFixedStringToEeprom(397, 16, DxcCallsign);
      }
    }

    // StartAzimuth
    if(ELEVATION==false){
      if ( ajaxserver.arg("startazimuth").length()<1 || ajaxserver.arg("startazimuth").toInt()<0 || ajaxserver.arg("startazimuth").toInt()>359){
        startazimuthERR= " Out of range number 0-359";
      }else{
        if(StartAzimuth == ajaxserver.arg("startazimuth").toInt()){
          startazimuthERR="";
        }else{
          startazimuthERR="";
          StartAzimuth = ajaxserver.arg("startazimuth").toInt();
          EEPROM.writeUShort(23, StartAzimuth);
          // EEPROM.commit();
          MqttPubString("StartAzimuth", String(StartAzimuth), true);
        }
      }
    }else{
      if (ajaxserver.arg("startazimuth").toInt()!=270){
        // startazimuthERR= " If enable Elevation, must be set to 270";
      }
    }

    // MaxRotateDegree
    if(ELEVATION==false){
      if ( ajaxserver.arg("maxrotatedegree").length()<1 || ajaxserver.arg("maxrotatedegree").toInt()<0 || ajaxserver.arg("maxrotatedegree").toInt()>719){
        maxrotatedegreeERR= " Out of range number 0-719";
      }else{
        if(MaxRotateDegree == ajaxserver.arg("maxrotatedegree").toInt()){
          maxrotatedegreeERR="";
        }else{
          maxrotatedegreeERR="";
          MaxRotateDegree = ajaxserver.arg("maxrotatedegree").toInt();
          EEPROM.writeUShort(25, MaxRotateDegree);
          // EEPROM.commit();
          MqttPubString("MaxRotateDegree", String(MaxRotateDegree), true);
        }
      }
    }else{
      if ( ajaxserver.arg("maxrotatedegree").length()<1 || ajaxserver.arg("maxrotatedegree").toInt()<0 || ajaxserver.arg("maxrotatedegree").toInt()>180){
        maxrotatedegreeERR= " Out of range number 0-180 (with enable Elevation)";
      }else{
        if(MaxRotateDegree == ajaxserver.arg("maxrotatedegree").toInt()){
          maxrotatedegreeERR="";
        }else{
          maxrotatedegreeERR="";
          MaxRotateDegree = ajaxserver.arg("maxrotatedegree").toInt();
          EEPROM.writeUShort(25, MaxRotateDegree);
          // EEPROM.commit();
          MqttPubString("MaxRotateDegree", String(MaxRotateDegree), true);
        }
      }
    }

    // AntRadiationAngle
    if ( ajaxserver.arg("antradiationangle").length()<1 || ajaxserver.arg("antradiationangle").toInt()<0 || ajaxserver.arg("antradiationangle").toInt()>180){
      antradiationangleERR= " Out of range number 1-180";
    }else{
      if(AntRadiationAngle == ajaxserver.arg("antradiationangle").toInt()){
        antradiationangleERR="";
      }else{
        antradiationangleERR="";
        AntRadiationAngle = ajaxserver.arg("antradiationangle").toInt();
        EEPROM.writeUShort(27, AntRadiationAngle);
        // EEPROM.commit();
        MqttPubString("AntRadiationAngle", String(AntRadiationAngle), true);
      }
    }

    // MapUrl
    if ( ajaxserver.arg("mapurl").length()<1 || ajaxserver.arg("mapurl").length()>50){
      mapurlERR= " Out of range 1-50 characters";
    }else{
      String str = String(ajaxserver.arg("mapurl"));
      if(MapUrl == str){
        mapurlERR="";
      }else{
        mapurlERR="";
        MapUrl = String(ajaxserver.arg("mapurl"));

        int str_len = str.length();
        char char_array[str_len+1];
        str.toCharArray(char_array, str_len+1);
        for (int i=0; i<50; i++){
          if(i < str_len){
            EEPROM.write(169+i, char_array[i]);
          }else{
            EEPROM.write(169+i, 0xff);
          }
        }
        // EEPROM.commit();
      }
    }

    // MapSource
    if (ajaxserver.arg("mapsource").length()<1){
      mapsourceERR= " Missing value";
    }else{
      int NewMapSource = ajaxserver.arg("mapsource").toInt();
      if(NewMapSource<0 || NewMapSource>1){
        mapsourceERR= " Invalid value";
      }else{
        mapsourceERR= "";
        if(MapSource != byte(NewMapSource)){
          MapSource = byte(NewMapSource);
          EEPROM.writeByte(266, MapSource);
        }
      }
    }

    // MapLocator
    String NewMapLocator = String(ajaxserver.arg("maplocator"));
    NewMapLocator.trim();
    NewMapLocator.toUpperCase();
    if ( NewMapLocator.length()!=6
      || NewMapLocator[0]<'A' || NewMapLocator[0]>'R'
      || NewMapLocator[1]<'A' || NewMapLocator[1]>'R'
      || NewMapLocator[2]<'0' || NewMapLocator[2]>'9'
      || NewMapLocator[3]<'0' || NewMapLocator[3]>'9'
      || NewMapLocator[4]<'A' || NewMapLocator[4]>'X'
      || NewMapLocator[5]<'A' || NewMapLocator[5]>'X'
    ){
      maplocatorERR= " Use 6 chars, for example JO60UC";
    }else{
      maplocatorERR= "";
      if(MapLocator != NewMapLocator){
        MapLocator = NewMapLocator;
        for (int i=0; i<6; i++){
          EEPROM.write(267+i, MapLocator[i]);
        }
      }
    }

    // MapZoomKm
    if ( ajaxserver.arg("mapzoomkm").length()<1 || ajaxserver.arg("mapzoomkm").toInt()<1000 || ajaxserver.arg("mapzoomkm").toInt()>20000){
      mapzoomkmERR= " Out of range number 1000-20000";
    }else{
      if(MapZoomKm == ajaxserver.arg("mapzoomkm").toInt()){
        mapzoomkmERR="";
      }else{
        mapzoomkmERR="";
        MapZoomKm = ajaxserver.arg("mapzoomkm").toInt();
        EEPROM.writeUShort(273, MapZoomKm);
      }
    }

    // MapTheme
    if (ajaxserver.arg("maptheme").length()<1){
      mapthemeERR = " Missing value";
    }else{
      int NewMapTheme = ajaxserver.arg("maptheme").toInt();
      if(NewMapTheme < 0 || NewMapTheme > 5){
        mapthemeERR = " Invalid value";
      }else{
        mapthemeERR = "";
        if(MapTheme != byte(NewMapTheme)){
          MapTheme = byte(NewMapTheme);
          EEPROM.writeByte(326, MapTheme);
        }
      }
    }

    // GraylineNtpServer
    String NewGraylineNtpServer = String(ajaxserver.arg("graylinentp"));
    NewGraylineNtpServer.trim();
    if (NewGraylineNtpServer.length()<1 || NewGraylineNtpServer.length()>50){
      graylinentpERR = " Out of range 1-50 characters";
    }else{
      graylinentpERR = "";
      if(GraylineNtpServer != NewGraylineNtpServer){
        GraylineNtpServer = NewGraylineNtpServer;
        int str_len = GraylineNtpServer.length();
        char char_array[str_len+1];
        GraylineNtpServer.toCharArray(char_array, str_len+1);
        for (int i=0; i<50; i++){
          if(i < str_len){
            EEPROM.write(275+i, char_array[i]);
          }else{
            EEPROM.write(275+i, 0xff);
          }
        }
        ApplyGraylineNtpConfig();
      }
    }

    // GraylineDarkness
    if (ajaxserver.arg("graylinedarkness").length()<1 || ajaxserver.arg("graylinedarkness").toInt()<0 || ajaxserver.arg("graylinedarkness").toInt()>100){
      graylinedarknessERR = " Out of range number 0-100";
    }else{
      graylinedarknessERR = "";
      if(GraylineDarkness != byte(ajaxserver.arg("graylinedarkness").toInt())){
        GraylineDarkness = byte(ajaxserver.arg("graylinedarkness").toInt());
        EEPROM.writeByte(325, GraylineDarkness);
      }
    }

    // Control page default map/display toggles
    bool newDefaultDegOverlayEnabled = ajaxserver.arg("mapdefaultdeg").toInt() == 1;
    bool newDefaultAntOverlayEnabled = ajaxserver.arg("mapdefaultant").toInt() == 1;
    bool newDefaultMapLocGridEnabled = ajaxserver.arg("mapdefaultlocgrid").toInt() == 1;
    bool newDefaultMapGraylineEnabled = ajaxserver.arg("mapdefaultgrayline").toInt() == 1;
    bool newDefaultMapBordersEnabled = ajaxserver.arg("mapdefaultborders").toInt() == 1;
    bool newDefaultMapDxccEnabled = ajaxserver.arg("mapdefaultdxcc").toInt() == 1;
    if(DefaultDegOverlayEnabled != newDefaultDegOverlayEnabled){
      DefaultDegOverlayEnabled = newDefaultDegOverlayEnabled;
      EEPROM.writeBool(413, DefaultDegOverlayEnabled);
    }
    if(DefaultAntOverlayEnabled != newDefaultAntOverlayEnabled){
      DefaultAntOverlayEnabled = newDefaultAntOverlayEnabled;
      EEPROM.writeBool(414, DefaultAntOverlayEnabled);
    }
    if(DefaultMapLocGridEnabled != newDefaultMapLocGridEnabled){
      DefaultMapLocGridEnabled = newDefaultMapLocGridEnabled;
      EEPROM.writeBool(415, DefaultMapLocGridEnabled);
    }
    if(DefaultMapGraylineEnabled != newDefaultMapGraylineEnabled){
      DefaultMapGraylineEnabled = newDefaultMapGraylineEnabled;
      EEPROM.writeBool(416, DefaultMapGraylineEnabled);
    }
    if(DefaultMapBordersEnabled != newDefaultMapBordersEnabled){
      DefaultMapBordersEnabled = newDefaultMapBordersEnabled;
      EEPROM.writeBool(417, DefaultMapBordersEnabled);
    }
    if(DefaultMapDxccEnabled != newDefaultMapDxccEnabled){
      DefaultMapDxccEnabled = newDefaultMapDxccEnabled;
      EEPROM.writeBool(418, DefaultMapDxccEnabled);
    }

    // 223 AZsource
    switch (ajaxserver.arg("source").toInt()) {
      case 0: {AZsource = 0;
        EEPROM.writeByte(223, 0);
        MqttPubString("AZsource", "Potentiometer", true);
        break; }
      case 1: {AZsource = 1;
        EEPROM.writeByte(223, 1);
        MqttPubString("AZsource", "CW/CCW pulse", true);
        if(Endstop == false){
          Endstop = true;
          EEPROM.writeBool(29, Endstop);
          MqttPubString("EndstopUse", String(Endstop), true);
        }
        break; }
      case 2: {AZsource = 2;
        EEPROM.writeByte(223, 2);
        MqttPubString("AZsource", "MQTT", true);
        if(Endstop == false){
          Endstop = true;
          EEPROM.writeBool(29, Endstop);
          MqttPubString("EndstopUse", String(Endstop), true);
        }
        break; }
    }
    // if(ajaxserver.arg("source").toInt()==0 && AZsource==true){
    //   AZsource = false;
    //   EEPROM.writeBool(223, 0);
    //   MqttPubString("AZsource", "Potentiometer", true);
    // }else if(ajaxserver.arg("source").toInt()==1 && AZsource==false){
    //   AZsource = true;
    //   EEPROM.writeBool(223, 1);
    //   MqttPubString("AZsource", "CW/CCW pulse", true);
    //   if(Endstop == false){
    //     Endstop = true;
    //     EEPROM.writeBool(29, Endstop);
    //     MqttPubString("EndstopUse", String(Endstop), true);
    //   }
    // }

    // 224-225 PulsePerDegree
    if ( ajaxserver.arg("pulseperdegree").length()<1 || ajaxserver.arg("pulseperdegree").toInt()<1 || ajaxserver.arg("pulseperdegree").toInt()>100){
      // pulseperdegreeERR= " Out of range number 1-100";
    }else{
      if(PulsePerDegree == ajaxserver.arg("pulseperdegree").toInt()){
        pulseperdegreeERR="";
      }else{
        pulseperdegreeERR="";
        PulsePerDegree = ajaxserver.arg("pulseperdegree").toInt();
        EEPROM.writeUShort(224, PulsePerDegree);
        // EEPROM.commit();
        MqttPubString("PulsePerDegree", String(PulsePerDegree), true);
      }
    }
    if(previousAzSource != AZsource || (AZsource == 1 && (previousPulsePerDegree != PulsePerDegree || previousMaxRotateDegree != MaxRotateDegree))){
      ApplyAzimuthSourceMode(false);
    }

    // 29  - Endstop
    if(ajaxserver.arg("edstops").toInt()==1 && Endstop==false){
      Endstop = true;
      EEPROM.writeBool(29, Endstop);
      // EEPROM.commit();
      MqttPubString("EndstopUse", String(Endstop), true);
    }else if(ajaxserver.arg("edstops").toInt()!=1 && Endstop==true){
      if(AZsource == 1){ //pulse
        Endstop=true;
      }else{  // potentiometer
        Endstop = false;
      }
      EEPROM.writeBool(29, Endstop);
      // EEPROM.commit();
      MqttPubString("EndstopUse", String(Endstop), true);
    }

    // 228 AZtwoWire
    if(ajaxserver.arg("twowire").toInt()==1 && AZtwoWire==false){
      AZtwoWire = true;
      digitalWrite(AZtwoWirePin, AZtwoWire);
      EEPROM.writeBool(228, AZtwoWire);
      MqttPubString("AZpotentiometer", "2-wire", true);
    }else if(ajaxserver.arg("twowire").toInt()!=1 && AZtwoWire==true){
      AZtwoWire = false;
      digitalWrite(AZtwoWirePin, AZtwoWire);
      EEPROM.writeBool(228, AZtwoWire);
      MqttPubString("AZpotentiometer", "3-wire", true);
    }

    // 229 AZpreamp
    if(ajaxserver.arg("preamp").toInt()==1 && AZpreamp==false){
      AZpreamp = true;
      digitalWrite(AZpreampPin, AZpreamp);
      EEPROM.writeBool(229, AZpreamp);
      MqttPubString("AZpreamp", "ON", true);
    }else if(ajaxserver.arg("preamp").toInt()!=1 && AZpreamp==true){
      AZpreamp = false;
      digitalWrite(AZpreampPin, AZpreamp);
      EEPROM.writeBool(229, AZpreamp);
      MqttPubString("AZpreamp", "OFF", true);
    }

    // 36 - NoEndstopLowZone
    if ( ajaxserver.arg("edstoplowzone").length()<1 || ajaxserver.arg("edstoplowzone").toInt()<2 || ajaxserver.arg("edstoplowzone").toInt()>15){
      // edstoplowzoneERR= " Out of range number 2-15";
    }else{
      if(NoEndstopLowZone == float(ajaxserver.arg("edstoplowzone").toInt())/10 ) {
        edstoplowzoneERR="";
      }else{
        edstoplowzoneERR="";
        NoEndstopLowZone = float(ajaxserver.arg("edstoplowzone").toInt())/10;
        // NoEndstopHighZone = 3.3 - NoEndstopLowZone;
        // NoEndstopLowZone = NoEndstopLowZone;
        EEPROM.writeByte(36, int(NoEndstopLowZone*10));
        // EEPROM.commit();
        MqttPubString("NoEndstopLowZone", String(NoEndstopLowZone), true);
      }
    }

    // 222 - NoEndstopHighZone
    if ( ajaxserver.arg("edstophighzone").length()<1 || ajaxserver.arg("edstophighzone").toInt()<16 || ajaxserver.arg("edstophighzone").toInt()>31){
      // edstophighzoneERR= " Out of range number 16-31";
    }else{
      if(NoEndstopHighZone == float(ajaxserver.arg("edstophighzone").toInt())/10 ) {
        edstophighzoneERR="";
      }else{
        edstophighzoneERR="";
        NoEndstopHighZone = float(ajaxserver.arg("edstophighzone").toInt())/10;
        // NoEndstopHighZone = 3.3 - NoEndstopLowZone;
        // NoEndstopLowZone = NoEndstopLowZone;
        EEPROM.writeByte(222, int(NoEndstopHighZone*10));
        // EEPROM.commit();
        MqttPubString("NoEndstopHighZone", String(NoEndstopHighZone), true);
      }
    }

    // 220 OneTurnLimitSec
    if ( ajaxserver.arg("oneturnlimitsec").length()<2 || ajaxserver.arg("oneturnlimitsec").toInt()<20 || ajaxserver.arg("oneturnlimitsec").toInt()>600){
      oneturnlimitsecERR= " Out of range number 20-600";
    }else{
      if(OneTurnLimitSec == ajaxserver.arg("oneturnlimitsec").toInt()){
        oneturnlimitsecERR="";
      }else{
        oneturnlimitsecERR="";
        OneTurnLimitSec = ajaxserver.arg("oneturnlimitsec").toInt();
        EEPROM.writeUShort(220, OneTurnLimitSec);
        // EEPROM.commit();
        MqttPubString("OneTurnLimitSec", String(OneTurnLimitSec), true);
      }
    }

    // 231 - PWMenable = true;
    if(ajaxserver.arg("pwmenable").toInt()==0 && PWMenable==true){
      PWMenable = false;
      EEPROM.writeBool(231, 0);
      MqttPubString("PWMenable", "OFF", true);
    }else if(ajaxserver.arg("pwmenable").toInt()==1 && PWMenable==false){
      PWMenable = true;
      EEPROM.writeBool(231, 1);
      MqttPubString("PWMenable", "ON", true);
    }
        
    // motor
    // MqttPubString("Debug Motor", String(ajaxserver.arg("motor")), false);
    // MqttPubString("Debug Motor2", String(ajaxserver.hasArg("motor")), false);
    if(ajaxserver.arg("motor").toInt()==0 && ACmotor==true){
      ACmotor = false;
      EEPROM.writeBool(30, 0);
      MqttPubString("Motor", "DC", true);
    }else if(ajaxserver.arg("motor").toInt()==1 && ACmotor==false){
      ACmotor = true;
      EEPROM.writeBool(30, 1);
      MqttPubString("Motor", "AC", true);
      PWMenable = false;
      EEPROM.writeBool(231, 0);
      MqttPubString("PWMenable", "OFF", true);
    }

    // 234-235 PwmRampSteps UShort
    if (ACmotor==false && PWMenable==true){
      if(ajaxserver.arg("pwmrampsteps").length()<1 || ajaxserver.arg("pwmrampsteps").toInt()<1 || ajaxserver.arg("pwmrampsteps").toInt()>200){
        pwmrampstepsERR= " Out of range number 1-200";
      }else{
        if(PwmRampSteps == ajaxserver.arg("pwmrampsteps").toInt()){
          pwmrampstepsERR="";
        }else{
          pwmrampstepsERR="";
          PwmRampSteps = ajaxserver.arg("pwmrampsteps").toInt();
          EEPROM.writeUShort(234, PwmRampSteps);
          // EEPROM.commit();
          MqttPubString("PwmRampSteps", String(PwmRampSteps), true);
        }
      }

      if(ajaxserver.arg("pwmtuneaggr").length()<1 || ajaxserver.arg("pwmtuneaggr").toInt()<1 || ajaxserver.arg("pwmtuneaggr").toInt()>5){
        pwmtuneaggrERR= " Select level 1-5";
      }else{
        byte newTuneAggressiveness = byte(ajaxserver.arg("pwmtuneaggr").toInt() - 1);
        if(PwmTuneAggressiveness == newTuneAggressiveness){
          pwmtuneaggrERR="";
        }else{
          pwmtuneaggrERR="";
          PwmTuneAggressiveness = newTuneAggressiveness;
          EEPROM.writeByte(330, PwmTuneAggressiveness);
          MqttPubString("PwmTuneAggressiveness", String(PwmTuneAggressiveness + 1), true);
        }
      }
    }

    // 226-227 BaudRate
    static int BaudRateTmp=115200;
    switch (ajaxserver.arg("baud").toInt()) {
      case 0: {BaudRateTmp= 1200; break; }
      case 1: {BaudRateTmp= 2400; break; }
      case 2: {BaudRateTmp= 4800; break; }
      case 3: {BaudRateTmp= 9600; break; }
      case 4: {BaudRateTmp= 115200; break; }
    }
    if(BaudRateTmp!=BaudRate){
      BaudRate=BaudRateTmp;
      EEPROM.writeUShort(226, BaudRate);
      MqttPubString("USB-BaudRate", String(BaudRate), true);
      Serial.println("Baudrate change to "+String(BaudRate)+"...");
      Serial.flush();
      // Serial.end();
      delay(1000);
      Serial.begin(BaudRate);
      delay(500);
      Serial.println();
      Serial.println();
      Serial.println("New Baudrate "+String(BaudRate));
    }

    // 161-164 - MQTT broker IP
    if ( ajaxserver.arg("mqttip0").length()<1 || ajaxserver.arg("mqttip0").toInt()>255){
      mqttERR= " Out of range number 0-255";
    }else{
      if(mqtt_server_ip[0] == byte(ajaxserver.arg("mqttip0").toInt()) ){
        mqttERR="";
      }else{
        mqttERR=" Warning: MQTT broker IP has changed.";
        mqtt_server_ip[0] = byte(ajaxserver.arg("mqttip0").toInt()) ;
        EEPROM.writeByte(161, mqtt_server_ip[0]);
      }
    }

    if ( ajaxserver.arg("mqttip1").length()<1 || ajaxserver.arg("mqttip1").toInt()>255){
      mqttERR= " Out of range number 0-255";
    }else{
      if(mqtt_server_ip[1] == byte(ajaxserver.arg("mqttip1").toInt()) ){
        mqttERR="";
      }else{
        mqttERR=" Warning: MQTT broker IP has changed.";
        mqtt_server_ip[1] = byte(ajaxserver.arg("mqttip1").toInt()) ;
        EEPROM.writeByte(162, mqtt_server_ip[1]);
      }
    }

    if ( ajaxserver.arg("mqttip2").length()<1 || ajaxserver.arg("mqttip2").toInt()>255){
      mqttERR= " Out of range number 0-255";
    }else{
      if(mqtt_server_ip[2] == byte(ajaxserver.arg("mqttip2").toInt()) ){
        mqttERR="";
      }else{
        mqttERR=" Warning: MQTT broker IP has changed.";
        mqtt_server_ip[2] = byte(ajaxserver.arg("mqttip2").toInt()) ;
        EEPROM.writeByte(163, mqtt_server_ip[2]);
      }
    }

    if ( ajaxserver.arg("mqttip3").length()<1 || ajaxserver.arg("mqttip3").toInt()>255){
      mqttERR= " Out of range number 0-255";
    }else{
      if(mqtt_server_ip[3] == byte(ajaxserver.arg("mqttip3").toInt()) ){
        mqttERR="";
      }else{
        mqttERR=" Warning: MQTT broker IP has changed.";
        mqtt_server_ip[3] = byte(ajaxserver.arg("mqttip3").toInt()) ;
        EEPROM.writeByte(164, mqtt_server_ip[3]);
      }
    }

    // 165-166 - MQTT_PORT
    if ( ajaxserver.arg("mqttport").length()<1 || ajaxserver.arg("mqttport").toInt()<1 || ajaxserver.arg("mqttport").toInt()>65535){
      mqttportERR= " Out of range number 1-65535";
    }else{
      if(MQTT_PORT == ajaxserver.arg("mqttport").toInt()){
        mqttportERR="";
      }else{
        mqttportERR=" Warning: MQTT broker PORT has changed.";
        MQTT_PORT = ajaxserver.arg("mqttport").toInt();
        EEPROM.writeUShort(165, MQTT_PORT);
      }
    }

    // 168 - MQTT_LOGIN
    if(ajaxserver.arg("mqtt_login").toInt()==1 && MQTT_LOGIN==false){
      MQTT_LOGIN = true;
      EEPROM.writeBool(168, MQTT_LOGIN);
      MqttPubString("MQTToginEnable", String(MQTT_LOGIN), true);
    }else if(ajaxserver.arg("mqtt_login").toInt()!=1 && MQTT_LOGIN==true){
      MQTT_LOGIN = false;
      EEPROM.writeBool(168, MQTT_LOGIN);
      MqttPubString("MQTToginEnable", String(MQTT_LOGIN), true);
    }

    // 232 - WebAuthEnabled
    if(ajaxserver.arg("webauth").toInt()==1 && webpassERR.length() > 0){
      WebAuthEnabled = false;
      EEPROM.writeBool(232, WebAuthEnabled);
    }else if(ajaxserver.arg("webauth").toInt()==1 && WebAuthPassword.length()<1){
      webpassERR= " Password is required when web authentication is enabled";
      WebAuthEnabled = false;
      EEPROM.writeBool(232, WebAuthEnabled);
    }else if(ajaxserver.arg("webauth").toInt()==1 && WebAuthEnabled==false){
      WebAuthEnabled = true;
      EEPROM.writeBool(232, WebAuthEnabled);
    }else if(ajaxserver.arg("webauth").toInt()!=1 && WebAuthEnabled==true){
      WebAuthEnabled = false;
      EEPROM.writeBool(232, WebAuthEnabled);
    }

    // 167 - ELEVATION
    if(ajaxserver.arg("elevation").toInt()==1 && ELEVATION==false){
      ELEVATION = true;
        StartAzimuth = 270;
        EEPROM.writeUShort(23, StartAzimuth);
        MqttPubString("StartAzimuth", String(StartAzimuth), true);
        MaxRotateDegree = 180;
        EEPROM.writeUShort(25, MaxRotateDegree);
        MqttPubString("MaxRotateDegree", String(MaxRotateDegree), true);
      EEPROM.writeBool(167, ELEVATION);
      MqttPubString("ElevationEnable", String(ELEVATION), true);
    }else if(ajaxserver.arg("elevation").toInt()!=1 && ELEVATION==true){
      ELEVATION = false;
      EEPROM.writeBool(167, ELEVATION);
      MqttPubString("ElevationEnable", String(ELEVATION), true);
    }

    if(ajaxserver.hasArg("dxchost") || ajaxserver.hasArg("dxcport") || ajaxserver.hasArg("dxccall")){
      DxcRequestReconnect();
    }

    EEPROM.commit();
  } // else form valid


sourceSELECT0= "";
sourceSELECT1= "";
sourceSELECT2= "";
switch (AZsource) {
  case 0: {
    sourceSELECT0= " selected";
    sourceSELECT1= "";
    sourceSELECT2= "";
    pulseperdegreeDisable=" disabled";
    pulseperdegreeSTYLE=" style='text-decoration: line-through; color: #555;'";
    twowireDisable="";
    twowireSTYLE=""; 
    break;   
    }
  case 1: {
    sourceSELECT0= "";
    sourceSELECT1= " selected";
    sourceSELECT2= "";
    pulseperdegreeDisable="";
    pulseperdegreeSTYLE="";
    twowireSTYLE=" style='text-decoration: line-through; color: #555;'";
    twowireDisable=" disabled";
    break;
    }
  case 2: {
    sourceSELECT0= "";
    sourceSELECT1= "";
    sourceSELECT2= " selected";
    pulseperdegreeDisable=" disabled";
    pulseperdegreeSTYLE=" style='text-decoration: line-through; color: #555;'";
    twowireSTYLE=" style='text-decoration: line-through; color: #555;'";
    twowireDisable=" disabled";
    break;
    }
}

if(MapSource==0){
  mapSourceSELECT0= " selected";
  mapSourceSELECT1= "";
  mapUrlRowStyle= "";
  mapLocatorRowStyle= " style='display:none;'";
  mapZoomRowStyle= " style='display:none;'";
  mapThemeRowStyle= " style='display:none;'";
  graylineNtpRowStyle= " style='display:none;'";
  graylineDarknessRowStyle= " style='display:none;'";
}else{
  mapSourceSELECT0= "";
  mapSourceSELECT1= " selected";
  mapUrlRowStyle= " style='display:none;'";
  mapLocatorRowStyle= "";
  mapZoomRowStyle= "";
  mapThemeRowStyle= "";
  graylineNtpRowStyle= "";
  graylineDarknessRowStyle= "";
}

if(MapTheme==0){
  mapThemeSELECT0 = " selected";
  mapThemeSELECT1 = "";
  mapThemeSELECT2 = "";
  mapThemeSELECT3 = "";
  mapThemeSELECT4 = "";
  mapThemeSELECT5 = "";
}else if(MapTheme==1){
  mapThemeSELECT0 = "";
  mapThemeSELECT1 = " selected";
  mapThemeSELECT2 = "";
  mapThemeSELECT3 = "";
  mapThemeSELECT4 = "";
  mapThemeSELECT5 = "";
}else if(MapTheme==2){
  mapThemeSELECT0 = "";
  mapThemeSELECT1 = "";
  mapThemeSELECT2 = " selected";
  mapThemeSELECT3 = "";
  mapThemeSELECT4 = "";
  mapThemeSELECT5 = "";
}else if(MapTheme==3){
  mapThemeSELECT0 = "";
  mapThemeSELECT1 = "";
  mapThemeSELECT2 = "";
  mapThemeSELECT3 = " selected";
  mapThemeSELECT4 = "";
  mapThemeSELECT5 = "";
}else if(MapTheme==4){
  mapThemeSELECT0 = "";
  mapThemeSELECT1 = "";
  mapThemeSELECT2 = "";
  mapThemeSELECT3 = "";
  mapThemeSELECT4 = " selected";
  mapThemeSELECT5 = "";
}else{
  mapThemeSELECT0 = "";
  mapThemeSELECT1 = "";
  mapThemeSELECT2 = "";
  mapThemeSELECT3 = "";
  mapThemeSELECT4 = "";
  mapThemeSELECT5 = " selected";
}

if(DefaultDegOverlayEnabled==true){
  mapdefaultdegCHECKED = "checked";
}
if(DefaultAntOverlayEnabled==true){
  mapdefaultantCHECKED = "checked";
}
if(DefaultMapLocGridEnabled==true){
  mapdefaultlocgridCHECKED = "checked";
}
if(DefaultMapGraylineEnabled==true){
  mapdefaultgraylineCHECKED = "checked";
}
if(DefaultMapBordersEnabled==true){
  mapdefaultbordersCHECKED = "checked";
}
if(DefaultMapDxccEnabled==true){
  mapdefaultdxccCHECKED = "checked";
}

// if(AZsource==true){
//   sourceSELECT0= "";
//   sourceSELECT1= " selected";
//   pulseperdegreeDisable="";
//   pulseperdegreeSTYLE="";
//   twowireSTYLE=" style='text-decoration: line-through; color: #555;'";
//   twowireDisable=" disabled";
// }else{
//   sourceSELECT0= " selected";
//   sourceSELECT1= "";
//   pulseperdegreeDisable=" disabled";
//   pulseperdegreeSTYLE=" style='text-decoration: line-through; color: #555;'";
//   twowireDisable="";
//   twowireSTYLE="";
// }

if(AZtwoWire==true){
  twowireSELECT0= "";
  twowireSELECT1= " selected";
}else{
  twowireSELECT0= " selected";
  twowireSELECT1= "";
}

if(AZpreamp==true){
  preampSELECT0= "";
  preampSELECT1= " selected";
}else{
  preampSELECT0= " selected";
  preampSELECT1= "";
}

if(Endstop==true){
  edstopsCHECKED= "checked";
  edstopsSTYLE="";
  edstoplowzoneDisable=" disabled";
  edstophighzoneDisable=" disabled";
  edstoplowzoneSTYLE=" style='text-decoration: line-through; color: #555;'";
  edstophighzoneSTYLE=" style='text-decoration: line-through; color: #555;'";
}else{
  edstoplowzoneSTYLE=" style='color: orange;'";
  edstophighzoneSTYLE=" style='color: orange;'";
  edstopsCHECKED= "";
  // edstopsSTYLE=" style='text-decoration: line-through; color: #555;'";
}

if(ELEVATION==true){
  elevationCHECKED= "checked";
  startazimuthSTYLE=" style='text-decoration: line-through; color: #555;'";
  startazimuthDisable=" disabled";
}else{
  elevationCHECKED= "";
  startazimuthSTYLE="";
  startazimuthDisable="";
}

if(MQTT_LOGIN==true){
  mqtt_loginCHECKED= "checked";
  mqtt_loginSTYLE="";
}else{
  mqtt_loginCHECKED= "";
  mqtt_loginSTYLE=" style='text-decoration: line-through; color: #555;'";
}

if(WebAuthEnabled==true){
  webauthCHECKED= "checked";
}else{
  webauthCHECKED= "";
}

if(MQTT_LOGIN==true){
  mqtt_loginCHECKED= "checked";
  mqtt_loginDisable="";
  mqtt_userSTYLE="";
  mqtt_passSTYLE="";
}else{
  mqtt_loginCHECKED= "";
  mqtt_loginDisable=" disabled";
  mqtt_userSTYLE=" style='text-decoration: line-through; color: #555;'";
  mqtt_passSTYLE=" style='text-decoration: line-through; color: #555;'";
}

baudSELECT0= "";
baudSELECT1= "";
baudSELECT2= "";
baudSELECT3= "";
baudSELECT4= "";
switch (BaudRate) {
  case 1200: {baudSELECT0= " selected"; break; }
  case 2400: {baudSELECT1= " selected"; break; }
  case 4800: {baudSELECT2= " selected"; break; }
  case 9600: {baudSELECT3= " selected"; break; }
  case 115200: {baudSELECT4= " selected"; break; }
}

if(ACmotor==true){
  motorSELECT0= "";
  motorSELECT1= " selected";
  pwmenableSTYLE=" style='text-decoration: line-through; color: #555;'";
  pwmenableDisable=" disabled";
    pwmrampstepsSTYLE=" style='text-decoration: line-through; color: #555;'";
    pwmrampstepsDisable=" disabled";
    pwmtuneaggrSTYLE=" style='text-decoration: line-through; color: #555;'";
    pwmtuneaggrDisable=" disabled";
}else{
  motorSELECT0= " selected";
  motorSELECT1= "";
  pwmenableSTYLE="";
  pwmenableDisable="";

  if(PWMenable==true){
    pwmSELECT0= "";
    pwmSELECT1= " selected";
    pwmrampstepsSTYLE= "";
    pwmrampstepsDisable= "";
    pwmtuneaggrSTYLE= "";
    pwmtuneaggrDisable= "";
  }else{
    pwmSELECT0= " selected";
    pwmSELECT1= "";
    pwmrampstepsSTYLE=" style='text-decoration: line-through; color: #555;'";
    pwmrampstepsDisable=" disabled";
    pwmtuneaggrSTYLE=" style='text-decoration: line-through; color: #555;'";
    pwmtuneaggrDisable=" disabled";
  }
}

switch (PwmTuneAggressiveness) {
  case 0: {pwmtuneaggrSELECT0= " selected"; break; }
  case 1: {pwmtuneaggrSELECT1= " selected"; break; }
  case 2: {pwmtuneaggrSELECT2= " selected"; break; }
  case 3: {pwmtuneaggrSELECT3= " selected"; break; }
  default: {pwmtuneaggrSELECT4= " selected"; break; }
}


  String HtmlSrc = "<!DOCTYPE html><html><head><title>SETUP</title>\n";
  HtmlSrc +="<meta http-equiv='Content-Type' content='text/html; charset=UTF-8'>\n";
  // <meta http-equiv = 'refresh' content = '600; url = /'>\n";
  HtmlSrc +="<style type='text/css'> @font-face {font-family: 'Roboto Condensed'; src: url('/RC-R.ttf') format('truetype'); font-weight: 400; font-style: normal; font-display: swap;} @font-face {font-family: 'Roboto Condensed'; src: url('/RC-B.ttf') format('truetype'); font-weight: 700; font-style: normal; font-display: swap;} button#go {background-color: #ccc; padding: 5px 20px 5px 20px; border: none; -webkit-border-radius: 5px; -moz-border-radius: 5px; border-radius: 5px;} button#go:hover {background-color: orange;} table, th, td {color: #fff; border-collapse: collapse; border:0px } .tdr {color: #0c0; height: 40px; text-align: right; vertical-align: middle; padding-right: 15px} html,body {background-color: #333; text-color: #ccc; font-family: 'Roboto Condensed',sans-serif,Arial,Tahoma,Verdana;} body {margin: 0; padding: 0 18px 28px 18px;} a:hover {color: #fff;} a { color: #ccc; text-decoration: underline;} ";
  HtmlSrc +=".b {border-top: 1px dotted #666;} .tooltip-text {visibility: hidden; position: absolute; z-index: 9999; width: 300px; max-width: calc(100vw - 24px); box-sizing: border-box; color: white; font-size: 12px; background-color: #DE3163; border-radius: 10px; padding: 10px 15px 10px 15px; box-shadow: 0 10px 25px rgba(0,0,0,0.35); } .hover-text:hover .tooltip-text, .hover-text:focus-within .tooltip-text { visibility: visible; } #right { top: -30px; left: 200%; } #top { top: -60px; left: -150%; } #left { top: -8px; right: 120%;}";
  HtmlSrc +=".hover-text {position: relative; z-index: 2; background: #888; padding: 5px 12px; margin: 5px; font-size: 15px; border-radius: 100%; color: #FFF; display: inline-block; text-align: center; } .setup-wrap {max-width: 980px; margin: 0 auto;} .setup-form {color: #ccc; margin: 0; text-align: center;} .setup-section {position: relative; background: #3b3b3b; border: 1px solid #555; border-radius: 14px; margin: 0 0 10px 0; overflow: visible;} .setup-summary {cursor: pointer; list-style: none; padding: 10px 14px; text-align: left; font-size: 20px; color: #ddd; background: #444; transition: background-color 0.15s ease, color 0.15s ease;} .setup-summary:hover, .setup-summary:focus {background: orange; color: #111;} .setup-summary::-webkit-details-marker {display: none;} .setup-summary:after {content: '\\25be'; float: right; color: #aaa;} .setup-summary:hover:after, .setup-summary:focus:after {color: #111;} .setup-section[open] .setup-summary {background: #d8921a; color: #111;} .setup-section[open] .setup-summary:after {content: '\\25b4'; color: #111;} .setup-table {width: 100%; table-layout: fixed;} .setup-table td {padding-top: 2px; padding-bottom: 2px;} .setup-table .tdr {width: 50%; box-sizing: border-box;} .setup-table td:last-child {width: 50%; text-align: left; box-sizing: border-box;} .setup-actions {text-align: center; margin-top: 18px;} .setup-note {color: #666; text-align: center; margin-top: 18px;} .backup-box {padding: 16px 18px 18px 18px; text-align: left; color: #ddd;} .backup-box p {margin: 0 0 14px 0; color: #bbb;} .backup-actions {display: flex; flex-wrap: wrap; gap: 10px; align-items: center; margin-bottom: 12px;} .backup-upload {display: flex; flex-wrap: wrap; gap: 10px; align-items: center;} .backup-upload input[type='file'] {max-width: 100%;} .backup-status {display: none; margin-top: 10px; padding: 10px 12px; border-radius: 8px; background: #2f2f2f; color: #ddd;} .backup-status.is-ok {display: block; background: #16351d; color: #d5ffd8;} .backup-status.is-error {display: block; background: #4a1f1f; color: #ffd9d9;}</style>\n";
  HtmlSrc +="</head><body>\n";
  HtmlSrc +="<H1 style='color: #666; text-align: center;'>Setup<br><span style='font-size: 50%;'>(MAC ";
  HtmlSrc +=MACString;
  HtmlSrc +="|FW ";
  HtmlSrc +=REV;
  HtmlSrc +="|HW ";
  HtmlSrc +=String(HardwareRev);
  HtmlSrc +=")</span><span style='color: #333;'>";
  HtmlSrc +=String(HWidValue);
  HtmlSrc +="</span></H1><div class='setup-wrap'><form action='/set' method='post' class='setup-form'>\n";
  HtmlSrc +="<details class='setup-section'><summary class='setup-summary'>Station and rotor</summary><table class='setup-table'>\n";
  HtmlSrc +="<tr class='b'><td class='tdr'><label for='yourcall'>Your callsign:</label></td><td><input type='text' id='yourcall' name='yourcall' size='10' value='";
  HtmlSrc += YOUR_CALL;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += yourcallERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 200px;'>Used as part of an MQTT topic</span></td></tr>\n<tr><td class='tdr'><label for='rotid'>Rotator ID:</label></td><td><input type='text' id='rotid' name='rotid' size='2' value='";
  HtmlSrc += NET_ID;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += rotidERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 300px;'>[1-2 chars]<br>Multiple rotators with the same TOPIC must have different IDs<br>Second part of MQTT topic</span></span></td></tr>\n<tr><td class='tdr'><label for='rotname'>Rotator name:</label></td><td><input type='text' id='rotname' name='rotname' size='20' value='";
  HtmlSrc += RotName;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += rotnameERR;
  HtmlSrc +="</span></td></tr>\n";
    HtmlSrc +="<tr class='b'><td class='tdr'><label for='elevation'>Use only for Elevation:</label></td><td><input type='checkbox' id='elevation' name='elevation' value='1' ${postData.elevation?'checked':''} ";
    HtmlSrc += elevationCHECKED;
    HtmlSrc +="><span class='hover-text'>?<span class='tooltip-text' id='top'>Will be SET<br>Start Azimuth to 270<br>Max Rotate Degree to 180 (may be change to 0-180)<br><br>For map use<br>https://remoteqth.com/xplanet/SKY.jpg</span></span></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='startazimuth'><span";
  HtmlSrc += startazimuthSTYLE;
  HtmlSrc += ">Start CCW azimuth:</span></label></td><td><input type='text' id='startazimuth' name='startazimuth' size='3' value='";
  HtmlSrc += StartAzimuth;
  HtmlSrc +="' ";
  HtmlSrc += startazimuthDisable;
  HtmlSrc +=">&deg; <span style='color:red;'>";
  HtmlSrc += startazimuthERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 100px;'>Allowed range<br>[0-359&deg;]</span></span></td></tr>\n<tr><td class='tdr'><label for='maxrotatedegree'>Rotation range in degrees:</label></td><td><input type='text' id='maxrotatedegree' name='maxrotatedegree' size='3' value='";
  HtmlSrc += MaxRotateDegree;
  HtmlSrc +="'>&deg; <span style='color:red;'>";
  HtmlSrc += maxrotatedegreeERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 100px;'>Range from CCW to CW endstop in degrees</span></span></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='antradiationangle'>Antenna radiation angle in degrees:</label></td><td><input type='text' id='antradiationangle' name='antradiationangle' size='3' value='";
  HtmlSrc += AntRadiationAngle;
  HtmlSrc +="'>&deg; <span style='color:red;'>";
  HtmlSrc += antradiationangleERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 100px;'>Allowed range<br>[1-180&deg;]</span></span></td></tr>\n";
  HtmlSrc +="</table></details>\n";

  HtmlSrc +="<details class='setup-section'><summary class='setup-summary'>Map and display</summary><table class='setup-table'>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='mapsource'>Map source:</label></td><td><select id='mapsource' name='mapsource' onchange='toggleMapSourceRows()'><option value='0'";
  HtmlSrc += mapSourceSELECT0;
  HtmlSrc +=">URL bitmap</option><option value='1'";
  HtmlSrc += mapSourceSELECT1;
  HtmlSrc +=">Vector map</option></select><span style='color:red;'>";
  HtmlSrc += mapsourceERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 240px;'>Vector map uses stored continent outlines, locator center and grayline with UTC time from the selected NTP server. Zoom is changed live with the bar below the map.</span></span></td></tr>\n";
  HtmlSrc +="<tr id='mapUrlRow'";
  HtmlSrc += mapUrlRowStyle;
  HtmlSrc +="><td class='tdr'><label for='mapurl'>Background azimuth map URL:</label></td><td><input type='text' id='mapurl' name='mapurl' size='30' value='";
  HtmlSrc += MapUrl;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += mapurlERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='left'>DXCC generated every quarter hour is available at https://remoteqth.com/xplanet/. If you need another, please contact OK1HRA, or run own services.</span></span> <a href='https://remoteqth.com/xplanet/' target='_blank'>Available list</a></td></tr>\n";
  HtmlSrc +="<tr id='mapLocatorRow'";
  HtmlSrc += mapLocatorRowStyle;
  HtmlSrc +="><td class='tdr'><label for='maplocator'>Map center locator:</label></td><td><input type='text' id='maplocator' name='maplocator' size='8' maxlength='6' value='";
  HtmlSrc += MapLocator;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += maplocatorERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 180px;'>Maidenhead locator in 6-char format, for example JO60UC</span></span></td></tr>\n";
  HtmlSrc +="<input type='hidden' id='mapzoomkm' name='mapzoomkm' value='";
  HtmlSrc += MapZoomKm;
  HtmlSrc +="'>\n";
  HtmlSrc +="<tr id='mapThemeRow'";
  HtmlSrc += mapThemeRowStyle;
  HtmlSrc +="><td class='tdr'><label for='maptheme'>Vector map theme:</label></td><td><select id='maptheme' name='maptheme'><option value='0'";
  HtmlSrc += mapThemeSELECT0;
  HtmlSrc +=">Calm marine</option><option value='1'";
  HtmlSrc += mapThemeSELECT1;
  HtmlSrc +=">Night radar</option><option value='2'";
  HtmlSrc += mapThemeSELECT2;
  HtmlSrc +=">Warm atlas</option><option value='3'";
  HtmlSrc += mapThemeSELECT3;
  HtmlSrc +=">Amber terminal</option><option value='4'";
  HtmlSrc += mapThemeSELECT4;
  HtmlSrc +=">Night vision</option><option value='5'";
  HtmlSrc += mapThemeSELECT5;
  HtmlSrc +=">Sky blue</option></select><span style='color:red;'>";
  HtmlSrc += mapthemeERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 360px;'>Calm marine is relaxed for long watching. Night radar is technical with clearer contrast. Warm atlas is softer indoors. Amber terminal feels like classic radio gear. Night vision is vivid green instrumentation. Sky blue is brighter, airy and map-like.</span></span></td></tr>\n";
  HtmlSrc +="<tr id='graylineNtpRow'";
  HtmlSrc += graylineNtpRowStyle;
  HtmlSrc +="><td class='tdr'><label for='graylinentp'>NTP server for grayline:</label></td><td><input type='text' id='graylinentp' name='graylinentp' size='24' value='";
  HtmlSrc += GraylineNtpServer;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += graylinentpERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 210px;'>Used to get UTC date and time for the grayline overlay. Default pool.ntp.org.</span></span></td></tr>\n";
  HtmlSrc +="<tr id='graylineDarknessRow'";
  HtmlSrc += graylineDarknessRowStyle;
  HtmlSrc +="><td class='tdr'><label for='graylinedarkness'>Grayline darkness:</label></td><td><input type='text' id='graylinedarkness' name='graylinedarkness' size='4' value='";
  HtmlSrc += GraylineDarkness;
  HtmlSrc +="'>&nbsp;%<span style='color:red;'>";
  HtmlSrc += graylinedarknessERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 190px;'>0 means invisible overlay, 100 means darkest night mask.</span></span></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='mapdefaultdeg'>Default DEG overlay:</label></td><td><input type='checkbox' id='mapdefaultdeg' name='mapdefaultdeg' value='1' ";
  HtmlSrc += mapdefaultdegCHECKED;
  HtmlSrc +="></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='mapdefaultant'>Default ANT overlay:</label></td><td><input type='checkbox' id='mapdefaultant' name='mapdefaultant' value='1' ";
  HtmlSrc += mapdefaultantCHECKED;
  HtmlSrc +="></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='mapdefaultlocgrid'>Default locator grid:</label></td><td><input type='checkbox' id='mapdefaultlocgrid' name='mapdefaultlocgrid' value='1' ";
  HtmlSrc += mapdefaultlocgridCHECKED;
  HtmlSrc +="></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='mapdefaultgrayline'>Default grayline:</label></td><td><input type='checkbox' id='mapdefaultgrayline' name='mapdefaultgrayline' value='1' ";
  HtmlSrc += mapdefaultgraylineCHECKED;
  HtmlSrc +="></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='mapdefaultborders'>Default state borders:</label></td><td><input type='checkbox' id='mapdefaultborders' name='mapdefaultborders' value='1' ";
  HtmlSrc += mapdefaultbordersCHECKED;
  HtmlSrc +="></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='mapdefaultdxcc'>Default DXCC prefixes:</label></td><td><input type='checkbox' id='mapdefaultdxcc' name='mapdefaultdxcc' value='1' ";
  HtmlSrc += mapdefaultdxccCHECKED;
  HtmlSrc +="><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 250px;'>These values define how the control page toggles are pre-set after the page is opened or refreshed.</span></span></td></tr>\n";
  HtmlSrc +="</table></details>\n";

  HtmlSrc +="<details class='setup-section'><summary class='setup-summary'>Sensors and limits</summary><table class='setup-table'>\n";
  HtmlSrc +="<tr class='b'><td class='tdr'><label for='source'>Azimuth source:</label></td><td><select name='source' id='source'><option value='0'";
  HtmlSrc += sourceSELECT0;
  HtmlSrc +=">Potentiometer</option><option value='1'";
  HtmlSrc += sourceSELECT1;
  HtmlSrc +=">CW/CCW pulse</option><option value='2'";
  HtmlSrc += sourceSELECT2;
  HtmlSrc +=">MQTT</option></select><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 320px;'>CW/CCW pulse uses the 3.5mm jack connector inputs on GPIO36 and GPIO5.<br>In pulse mode the manual KEY input on these pins is disabled.<br>MQTT rx on topic " + String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/RxAzimuth</span></span></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='pulseperdegree'><span";
  HtmlSrc += pulseperdegreeSTYLE;
  HtmlSrc +=">Pulse count per degree:</span></label></td><td><input type='text' id='pulseperdegree' name='pulseperdegree' size='3' value='";
  HtmlSrc += PulsePerDegree;
  HtmlSrc +="'";
  HtmlSrc += pulseperdegreeDisable;
  HtmlSrc +="><span style='color:red;'>";
  HtmlSrc += pulseperdegreeERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 100px;'>Allowed range<br>[1-100]</span></span></td></tr>\n";

  HtmlSrc +="<tr><td class='tdr'><label for='twowire'><span";
  HtmlSrc += twowireSTYLE;
  HtmlSrc +=">Azimuth potentiometer:</span></label></td><td><select name='twowire' id='twowire'";
  HtmlSrc += twowireDisable;
  HtmlSrc +="><option value='0'";
  HtmlSrc += twowireSELECT0;
  HtmlSrc +=">3 Wire</option><option value='1'";
  HtmlSrc += twowireSELECT1;
  HtmlSrc +=">2 Wire</option></select><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 150px;'>2 wire use 9mA CC source<br>3 wire use 9V CV source</span></span>";
  if(AZtwoWire==true && AZpreamp==true){
    HtmlSrc +="<br><span style='color: red;'>Recommend using a 3-wire potentiometer with the preamplifier ON</span>";
  }
  HtmlSrc +="</td></tr>\n";

  HtmlSrc +="<tr><td class='tdr'><label for='preamp'><span";
  HtmlSrc += twowireSTYLE;
  HtmlSrc +=">Azimuth gain/shift op-amp:</span></label></td><td><select name='preamp' id='preamp'";
  HtmlSrc += twowireDisable;
  HtmlSrc +="><option value='0'";
  HtmlSrc += preampSELECT0;
  HtmlSrc +=">OFF</option><option value='1'";
  HtmlSrc += preampSELECT1;
  HtmlSrc +=">ON</option></select><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 200px;'>For potentiometer use one turn from any<br>Need manualy preset with two trimmer<br>More in Wiki page</span></span></td></tr>\n";

  // if(AZsource==false){ // potentiometer
    HtmlSrc +="<tr class='b'><td class='tdr'><label for='edstops'><span";
    HtmlSrc += edstopsSTYLE;
    HtmlSrc +=">Hardware endstops INSTALLED:</span></label></td><td><input type='checkbox' id='edstops' name='edstops' value='1' ${postData.edstops?'checked':''} ";
    HtmlSrc += edstopsCHECKED;
    HtmlSrc +="><span class='hover-text'>?<span class='tooltip-text' id='top'>If disabled, it reduces the range of the potentiometer by the forbidden zone on edges</span></span></td></tr>\n";
      HtmlSrc +="<tr><td class='tdr'><label for='edstoplowzone'><span";
      HtmlSrc += edstoplowzoneSTYLE;
      HtmlSrc +=">CCW forbidden zone<br>(software endstops):</span></label></td><td><input type='text' id='edstoplowzone' name='edstoplowzone' size='3' value='";
      HtmlSrc += int(NoEndstopLowZone*10);
      HtmlSrc +="'";
      HtmlSrc += edstoplowzoneDisable;
      HtmlSrc +="> tenths of a Volt <span style='color:red;'>";
      HtmlSrc += edstoplowzoneERR;
      HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 150px;'>Allowed range<br>[2-15] tenths of a Volt</span></span></td></tr>\n";

      HtmlSrc +="<tr><td class='tdr'><label for='edstophighzone'><span";
      HtmlSrc += edstophighzoneSTYLE;
      HtmlSrc +=">CW forbidden zone<br>(software endstops):</span></label></td><td><input type='text' id='edstophighzone' name='edstophighzone' size='3' value='";
      HtmlSrc += int(NoEndstopHighZone*10);
      HtmlSrc +="'";
      HtmlSrc += edstophighzoneDisable;
      HtmlSrc +="> tenths of a Volt <span style='color:red;'>";
      HtmlSrc += edstophighzoneERR;
      HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 150px;'>Allowed range<br>[16-31] tenths of a Volt</span></span></td></tr>\n";
  // }
  HtmlSrc +="</table></details>\n";

  HtmlSrc +="<details class='setup-section'><summary class='setup-summary'>Motor and drive</summary><table class='setup-table'>\n";
  HtmlSrc +="<tr class='b'><td class='tdr'><label for='oneturnlimitsec'>Watchdog speed:</label></td><td><input type='text' id='oneturnlimitsec' name='oneturnlimitsec' size='3' value='";
  HtmlSrc += OneTurnLimitSec;
  HtmlSrc +="'> seconds per one turn <span style='color:red;'>";
  HtmlSrc += oneturnlimitsecERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='left' style='width: 300px;'>Allowed range [20-600sec]<br>Lower speed limit activating the watchdog<br>Use a number 50% higher than the actual speed of your rotator</span></span></td></tr>\n";

  HtmlSrc +="<tr><td class='tdr'><label for='acmotor'>Motor supply:</label></td><td><select name='motor' id='motor'><option value='0'";
  HtmlSrc += motorSELECT0;
  HtmlSrc +=">DC</option><option value='1'";
  HtmlSrc += motorSELECT1;
  HtmlSrc +=">AC</option></select><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 150px;'>DC with optional PWM<br>AC activates another relay sequence</span></span></td></tr>\n";

  HtmlSrc +="<tr><td class='tdr'><label for='pwmenable'><span";
  HtmlSrc += pwmenableSTYLE;
  HtmlSrc += ">DC PWM control:</label></td><td><select name='pwmenable' id='pwmenable' ";
  HtmlSrc += pwmenableDisable;
  HtmlSrc +="><option value='0'";
  HtmlSrc += pwmSELECT0;
  HtmlSrc +=">OFF</option><option value='1'";
  HtmlSrc += pwmSELECT1;
  HtmlSrc +=">ON</option></select><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 200px;'>If disable, mosfet must be bridged,<br>or replace by jumper<br>More in Wiki page</span></span></span></td></tr>\n";

    HtmlSrc +="<tr><td class='tdr'><label for='pwmrampsteps'><span";
    HtmlSrc += pwmrampstepsSTYLE;
    HtmlSrc += ">PWM slew interval:</label></td><td><input type='text' id='pwmrampsteps' name='pwmrampsteps' size='3' value='";
    HtmlSrc += PwmRampSteps;
    HtmlSrc +="' ";
    HtmlSrc += pwmrampstepsDisable;
    HtmlSrc +="> " + FormatPwmTotalRampTime(PwmRampSteps, PwmMaxDuty) + " <span style='color:red;'>";
    HtmlSrc += pwmrampstepsERR;
    HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='left' style='width: 320px;'>Allowed range [1-200] ms for one PWM step<br>The value after the field shows the estimated total ramp-up time from 0 to the current PWM max duty<br>Lower value = faster response<br>Higher value = softer start and stop<br>Braking distance is learned automatically from real stops.</span></span></span></td></tr>\n";

    HtmlSrc +="<tr><td class='tdr'><label for='pwmtuneaggr'><span";
    HtmlSrc += pwmtuneaggrSTYLE;
    HtmlSrc += ">Brake learning aggressiveness:</span></label></td><td><select id='pwmtuneaggr' name='pwmtuneaggr'";
    HtmlSrc += pwmtuneaggrDisable;
    HtmlSrc +="><option value='1'";
    HtmlSrc += pwmtuneaggrSELECT0;
    HtmlSrc +=">1 Very soft</option><option value='2'";
    HtmlSrc += pwmtuneaggrSELECT1;
    HtmlSrc +=">2 Soft</option><option value='3'";
    HtmlSrc += pwmtuneaggrSELECT2;
    HtmlSrc +=">3 Medium</option><option value='4'";
    HtmlSrc += pwmtuneaggrSELECT3;
    HtmlSrc +=">4 Strong</option><option value='5'";
    HtmlSrc += pwmtuneaggrSELECT4;
    HtmlSrc +=">5 Very strong</option></select><span style='color:red;'>";
    HtmlSrc += pwmtuneaggrERR;
    HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='left' style='width: 360px;'>Controls how fast the learned braking window reacts to overshoot and undershoot.<br>Level 3 is the intended center point near zero error.<br>Level 4 should already be able to slightly undershoot on some stops.<br>Level 5 is intentionally over-aggressive.</span></span></td></tr>\n";
  HtmlSrc +="</table></details>\n";

  HtmlSrc +="<details class='setup-section'><summary class='setup-summary'>Serial and MQTT</summary><table class='setup-table'>\n";
  HtmlSrc +="<tr class='b'><td class='tdr'><label for='baud'>USB serial BAUDRATE:</label></td><td><select name='baud' id='baud'><option value='0'";
  HtmlSrc += baudSELECT0;
  HtmlSrc +=">1200</option><option value='1'";
  HtmlSrc += baudSELECT1;
  HtmlSrc +=">2400</option><option value='2'";
  HtmlSrc += baudSELECT2;
  HtmlSrc +=">4800</option><option value='3'";
  HtmlSrc += baudSELECT3;
  HtmlSrc +=">9600</option><option value='4'";
  HtmlSrc += baudSELECT4;
  HtmlSrc +=">115200</option></select><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 150px;'>Use for GS-232 protocol<br>Must restart after change</span></span></td></tr>\n";

  HtmlSrc +="<tr class='b'><td class='tdr'><label for='mqttip0'>MQTT broker IP:</label></td><td>";
  HtmlSrc +="<input type='text' id='mqttip0' name='mqttip0' size='1' value='" + String(mqtt_server_ip[0]) + "'>&nbsp;.&nbsp;<input type='text' id='mqttip1' name='mqttip1' size='1' value='" + String(mqtt_server_ip[1]) + "'>&nbsp;.&nbsp;<input type='text' id='mqttip2' name='mqttip2' size='1' value='" + String(mqtt_server_ip[2]) + "'>&nbsp;.&nbsp;<input type='text' id='mqttip3' name='mqttip3' size='1' value='" + String(mqtt_server_ip[3]) + "'>";
  HtmlSrc +="<span style='color:red;'>";
  HtmlSrc += mqttERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 250px;'>Default public broker 54.38.157.134<br>If the first digit is zero, MQTT is disabled</span></span></td></tr>\n";

  HtmlSrc +="<tr><td class='tdr'><label for='mqttport'>MQTT broker PORT:</label></td><td>";
  HtmlSrc +="<input type='text' id='mqttport' name='mqttport' size='2' value='" + String(MQTT_PORT) + "'>\n";
  HtmlSrc +="<span style='color:red;'>";
  HtmlSrc += mqttportERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 150px;'>Default public broker port 1883</span></span></td></tr>\n";

  HtmlSrc +="<tr><td class='tdr'><label for='mqtt_login'>Enable MQTT PASSWORD:</label></td><td><input type='checkbox' id='mqtt_login' name='mqtt_login' value='1' ${postData.mqtt_login?'checked':''} ";
  HtmlSrc += mqtt_loginCHECKED;
  HtmlSrc +="><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 150px;'>Enable login for<br>connect to MQTT broker<br>WARNING, does not support encryption!</span></span></td></tr>\n";
    HtmlSrc +="<tr><td class='tdr'><label for='mqttuser'><span";
    HtmlSrc += mqtt_userSTYLE;
    HtmlSrc += ">MQTT Login:</span></label></td><td><input type='text' id='mqttuser' name='mqttuser' size='10' value='";
    HtmlSrc += MQTT_USER;
    HtmlSrc +="' ";
    HtmlSrc += mqtt_loginDisable;
    HtmlSrc +="><span style='color:red;'>";
    HtmlSrc += mqtt_userERR;
    HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 150px;'>Login Name max 10 character, for connect to MQTT broker</span></span></td></tr>\n";

  HtmlSrc +="<tr><td class='tdr'><label for='mqttpass'><span";
  HtmlSrc += mqtt_passSTYLE;
  HtmlSrc += ">MQTT Password:</span></label></td><td><input type='password' id='mqttpass' name='mqttpass' size='20' value='";
  HtmlSrc += MQTT_PASS;
  HtmlSrc +="' ";
  HtmlSrc += mqtt_loginDisable;
  HtmlSrc +="><span style='color:red;'>";
  HtmlSrc += mqtt_passERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 150px;'>Login Password max 20 character, for connect to MQTT broker</span></span></td></tr>\n";
  HtmlSrc +="</table></details>\n";
  HtmlSrc +="<details class='setup-section'><summary class='setup-summary'>DX Cluster</summary><table class='setup-table'>\n";
  HtmlSrc +="<tr class='b'><td class='tdr'><label for='dxchost'>DX cluster IP/host:</label></td><td><input type='text' id='dxchost' name='dxchost' size='24' value='";
  HtmlSrc += DxcHost;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += dxc_hostERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 220px;'>Leave empty to disable DXC. The ESP32 opens a Telnet TCP connection to this host.</span></span></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='dxcport'>DX cluster PORT:</label></td><td><input type='text' id='dxcport' name='dxcport' size='6' value='";
  HtmlSrc += String(DxcPort);
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += dxc_portERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 180px;'>Typical Telnet port is often 23 or 7300.</span></span></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='dxccall'>DX cluster callsign:</label></td><td><input type='text' id='dxccall' name='dxccall' size='16' maxlength='16' value='";
  HtmlSrc += DxcCallsign;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += dxc_callsignERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 210px;'>Sent to the DX cluster after Telnet connects. Required when DXC host is filled.</span></span></td></tr>\n";
  HtmlSrc +="</table></details>\n";
  HtmlSrc +="<details class='setup-section'><summary class='setup-summary'>Web authentication</summary><table class='setup-table'>\n";
  HtmlSrc +="<tr class='b'><td class='tdr'><label for='webauth'>Enable web authentication:</label></td><td><input type='checkbox' id='webauth' name='webauth' value='1' ";
  HtmlSrc += webauthCHECKED;
  HtmlSrc +="><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 260px;'>Protects the web pages, control API and OTA update with HTTP Digest authentication. HTTPS is not available, so use this only on a trusted local network.</span></span></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'>Web login user:</td><td><input type='text' size='10' value='";
  HtmlSrc += WEB_AUTH_USER;
  HtmlSrc +="' readonly></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='webpass'>Web login password:</label></td><td><input type='password' id='webpass' name='webpass' size='24' maxlength='100' value='";
  HtmlSrc += WebAuthPassword;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += webpassERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 220px;'>Set your own web password here. The current password is also available in the USB serial console with command <strong>key</strong>.</span></span></td></tr>\n";
  HtmlSrc +="</table></details>\n";
  HtmlSrc +="<details class='setup-section'><summary class='setup-summary'>Backup and restore</summary><div class='backup-box'>";
  HtmlSrc +="<p>Download the full rotator configuration as a JSON backup, or upload it later to restore settings.</p>";
  HtmlSrc +="<div class='backup-actions'><a href='/backup/config'><button type='button' id='go'>Download backup</button></a></div>";
  HtmlSrc +="<div class='backup-upload'><input type='file' id='backupFile' accept='.json,application/json'><button type='button' id='go' onclick='uploadConfigBackup()'>Upload backup</button></div>";
  HtmlSrc +="<div id='backupStatus' class='backup-status'></div>";
  HtmlSrc +="</div></details>\n";
  HtmlSrc +="<div class='setup-actions'><button id='go'>&#10004; Change</button></form>&nbsp; ";
  HtmlSrc +="<a href='/cal' onclick=\"window.open( this.href, this.href, 'width=700,height=1150,left=0,top=0,menubar=no,location=no,status=no' ); return false;\"><button id='go'>Calibrate &#8618;</button></a></div>";
  HtmlSrc +="<p class='setup-note'>After change, refresh all other page for apply changes.<br><a href='https://remoteqth.com/w/doku.php?id=simple_rotator_interface_v' target='_blank'>More on Wiki &#10138;</a></p>";
  HtmlSrc +="</div>";
  HtmlSrc +="<script>function toggleMapSourceRows(){var s=document.getElementById('mapsource').value;document.getElementById('mapUrlRow').style.display=(s==='0')?'table-row':'none';document.getElementById('mapLocatorRow').style.display=(s==='1')?'table-row':'none';document.getElementById('mapThemeRow').style.display=(s==='1')?'table-row':'none';document.getElementById('graylineNtpRow').style.display=(s==='1')?'table-row':'none';document.getElementById('graylineDarknessRow').style.display=(s==='1')?'table-row':'none';}function setBackupStatus(message,isError){var box=document.getElementById('backupStatus');if(!box){return;}box.textContent=message;box.className='backup-status '+(isError?'is-error':'is-ok');}function uploadConfigBackup(){var input=document.getElementById('backupFile');if(!input||!input.files||!input.files.length){setBackupStatus('Select a backup JSON file first.',true);return;}var formData=new FormData();formData.append('file',input.files[0]);setBackupStatus('Uploading backup...',false);fetch('/backup/config',{method:'POST',body:formData}).then(function(response){return response.text().then(function(text){if(!response.ok){throw new Error(text||'Upload failed');}return text;});}).then(function(text){setBackupStatus(text||'Backup restored.',false);}).catch(function(error){setBackupStatus(error.message||'Upload failed',true);});}var activeTooltipHost=null;function resetTooltip(trigger){var tip=trigger?trigger.querySelector('.tooltip-text'):null;if(!tip){return;}tip.style.left='';tip.style.right='';tip.style.top='';tip.style.bottom='';tip.style.position='';}function positionTooltip(trigger){var tip=trigger?trigger.querySelector('.tooltip-text'):null;if(!tip){return;}tip.style.position='fixed';tip.style.left='0px';tip.style.right='auto';tip.style.top='0px';tip.style.bottom='auto';var triggerRect=trigger.getBoundingClientRect();var tipRect=tip.getBoundingClientRect();var gap=12;var left=triggerRect.left+((triggerRect.width-tipRect.width)/2);var top=triggerRect.top-tipRect.height-gap;if(tip.id==='left'){left=triggerRect.left-tipRect.width-gap;top=triggerRect.top+((triggerRect.height-tipRect.height)/2);}else if(tip.id==='right'){left=triggerRect.right+gap;top=triggerRect.top+((triggerRect.height-tipRect.height)/2);}if(left<12){left=12;}if(left+tipRect.width>window.innerWidth-12){left=window.innerWidth-tipRect.width-12;}if(top<12){top=(tip.id==='top')?(triggerRect.bottom+gap):12;}if(top+tipRect.height>window.innerHeight-12){top=window.innerHeight-tipRect.height-12;}if(top<12){top=12;}tip.style.left=Math.round(left)+'px';tip.style.top=Math.round(top)+'px';}function bindTooltips(){var triggers=document.querySelectorAll('.hover-text');for(var i=0;i<triggers.length;i++){triggers[i].addEventListener('mouseenter',function(){activeTooltipHost=this;positionTooltip(this);});triggers[i].addEventListener('focusin',function(){activeTooltipHost=this;positionTooltip(this);});triggers[i].addEventListener('mouseleave',function(){if(activeTooltipHost===this){activeTooltipHost=null;}resetTooltip(this);});triggers[i].addEventListener('focusout',function(){if(activeTooltipHost===this){activeTooltipHost=null;}resetTooltip(this);});}}window.addEventListener('resize',function(){if(activeTooltipHost){positionTooltip(activeTooltipHost);}});window.addEventListener('scroll',function(){if(activeTooltipHost){positionTooltip(activeTooltipHost);}},true);toggleMapSourceRows();bindTooltips();</script>";
  HtmlSrc +="</body></html>\n";

  ajaxserver.send(200, "text/html", HtmlSrc); //Send web page
}

void handleCal() {

  if ( ajaxserver.hasArg("stop")==1 ){
    RequestStopRamp(true);
  }

  if ( ajaxserver.hasArg("cw")==1 ){
    Status=1; //digitalWrite(BrakePin, HIGH); delay(24);
    AzimuthTarget=MaxRotateDegree;
    // RunTimer();
  }

  if ( ajaxserver.hasArg("ccw")==1 ){
    Status=-1; //digitalWrite(BrakePin, HIGH); delay(24);
    AzimuthTarget=0;
    // RunTimer();
  }

  if ( ajaxserver.hasArg("reverse")==1 ){
    Reverse = !Reverse;
    EEPROM.writeBool(35, Reverse);
    EEPROM.commit();
    MqttPubString("ReverseControl", String(Reverse), true);
  }

  if ( ajaxserver.hasArg("reverseaz")==1 ){
    ReverseAZ = !ReverseAZ;
    EEPROM.writeBool(230, ReverseAZ);
    EEPROM.commit();
    MqttPubString("ReverseAzimuth", String(ReverseAZ), true);
  }

  if ( ajaxserver.hasArg("clear")==1 ){
    if(AZsource == 0){
      CcwRaw=142;
      CwRaw = 3155;
      EEPROM.writeUShort(31, CcwRaw);
      EEPROM.writeUShort(33, CwRaw);
      EEPROM.commit();
      MqttPubString("CcwRaw", String(CcwRaw), true);
      MqttPubString("CwRaw", String(CwRaw), true);
    }
  }

  long RawTmp = 0;

  // 31-32 CcwRaw
  if ( ajaxserver.hasArg("setccw")==1 ){
    if(AZsource == 0){
    // RawTmp = 0;
    // for (int i=0; i<10; i++){
    //   RawTmp = RawTmp + readADC_Cal(analogRead(AzimuthPin));
    //   delay(10);
    // }
    // CcwRaw = RawTmp / 10;
    CcwRaw = AzimuthValue;
    EEPROM.writeUShort(31, CcwRaw);
    EEPROM.commit();
    MqttPubString("CcwRaw", String(CcwRaw), true);
    }
  }

  // 33-34  CwRaw
  if ( ajaxserver.hasArg("setcw")==1 ){
    if(AZsource == 0){
    // RawTmp = 0;
    // for (int i=0; i<10; i++){
    //   RawTmp = RawTmp + readADC_Cal(analogRead(AzimuthPin));
    //   delay(10);
    // }
    // CwRaw = RawTmp / 10;
    CwRaw = AzimuthValue;
    EEPROM.writeUShort(33, CwRaw);
    EEPROM.commit();
    MqttPubString("CwRaw", String(CwRaw), true);
    }
  }

    // MqttPubString("Debug setcw", String(ajaxserver.arg("setcw")), false);
    // MqttPubString("Debug setcw 2", String(ajaxserver.hasArg("setcw")), false);

  String ReverseCOLOR= "";
  String ReverseSTATUS= "";
  if(Reverse==true){
    // ReverseCOLOR= " style='background-color: #c00; color: #FFF;'";
    ReverseCOLOR= " class='red'";
    ReverseSTATUS= "ON";
  }else{
    ReverseCOLOR= "";
    ReverseSTATUS= "OFF";
  }

  String ReverseAzCOLOR= "";
  String ReverseAzSTATUS= "";
  if(ReverseAZ==true){
    // ReverseCOLOR= " style='background-color: #c00; color: #FFF;'";
    ReverseAzCOLOR= " class='red'";
    ReverseAzSTATUS= "ON";
  }else{
    ReverseAzCOLOR= "";
    ReverseAzSTATUS= "OFF";
  }

  String HtmlSrc = "<!DOCTYPE html><html><head><title>CALIBRATE</title>";
  HtmlSrc +="<meta http-equiv='Content-Type' content='text/html; charset=UTF-8'>";
  HtmlSrc +="<style type='text/css'>@font-face {font-family: 'Roboto Condensed'; src: url('/RC-R.ttf') format('truetype'); font-weight: 400; font-style: normal; font-display: swap;} @font-face {font-family: 'Roboto Condensed'; src: url('/RC-B.ttf') format('truetype'); font-weight: 700; font-style: normal; font-display: swap;} button {background-color: #ccc; padding: 5px 20px 5px 20px; border: none; -webkit-border-radius: 5px; -moz-border-radius: 5px; border-radius: 5px;} button:hover {background-color: orange;} ";
  HtmlSrc +=".red {background-color: #c00; color: #FFF;} table, th, td { color: #fff; border: 0px; border-color: #666; border-style: solid; margin: 0px;}";
  HtmlSrc +=".tdl { text-align: left; padding: 10px;}";
  HtmlSrc +=".tdc { text-align: center; padding: 10px;}";
  HtmlSrc +=".tdr { text-align: right; padding: 10px;}";
  HtmlSrc +="html,body { background-color: #333; text-color: #ccc; font-family: 'Roboto Condensed',sans-serif,Arial,Tahoma,Verdana;}";
  HtmlSrc +="a:hover {color: #fff;}";
  HtmlSrc +="a {color: #ccc; text-decoration: underline;}";
  HtmlSrc +="</style></head><body>";
  HtmlSrc +="<H1 style='color: #666; text-align: center;'>Calibration steps:<br><span style='font-size: 50%;'>(MAC ";
  HtmlSrc +=MACString;
  HtmlSrc +="|FW ";
  HtmlSrc +=REV;
  HtmlSrc +="|HW ";
  HtmlSrc +=String(HardwareRev);
  // HtmlSrc +="|";
  // HtmlSrc +=String(HWidValue);
  HtmlSrc +=")</span></H1><div style='display: flex; justify-content: center;'>";
  HtmlSrc +="<table cellspacing='0' cellpadding='0'><form action='/cal' method='post' style='color: #ccc; margin: 50 0 0 0; text-align: center;'>";
  HtmlSrc +="<tr><td class='tdc' colspan='3' style='background-color: #666; border-top-left-radius: 20px; border-top-right-radius: 20px;'><span style='font-size: 200%;'>";
  if(ELEVATION==false){
    HtmlSrc +="1. Rotate direction calibrate";  
  }else{
      HtmlSrc +="1. Elevation direction calibrate";
  }
  HtmlSrc +="</span></td></tr>";
  HtmlSrc +="<tr style='background-color: #666;'>";
  HtmlSrc +="<td class='tdr'><button id='ccw' name='ccw'>&#8630; CCW</button></td>";
  HtmlSrc +="<td class='tdc'><button id='stop' name='stop'>&#10008; STOP</button></td>";
  HtmlSrc +="<td class='tdl'><button id='cw' name='cw'>CW &#8631;</button></td>";
  HtmlSrc +="</tr><tr>";
  HtmlSrc +="<td class='tdc' colspan='3' style='background-color: #666;'><button id='reverse' name='reverse'";
  HtmlSrc +=ReverseCOLOR;
  HtmlSrc +=">REVERSE-CONTROL-<strong>";
  HtmlSrc +=ReverseSTATUS;
  HtmlSrc +="</strong></button></td>";
  HtmlSrc +="</tr><tr>";
  HtmlSrc +="<td class='tdc' colspan='3' style='color: #333; background-color: #666; border-bottom-left-radius: 20px; border-bottom-right-radius: 20px;'><span style='color: #ccc;'>Instruction:</span> if it does not rotate according to the buttons, reverse the control</td>";
  HtmlSrc +="</tr><tr>";
  HtmlSrc +="<td class='tdc' colspan='3' style='height:30px'></td></tr>";

  HtmlSrc +="<tr><td class='tdc' colspan='3' style='background-color: #666; border-top-left-radius: 20px; border-top-right-radius: 20px;'><span style='font-size: 200%;'>";
  if(AZsource == 1){
    HtmlSrc +="2. Pulse counter monitor";
  }else if(ELEVATION==false){
    HtmlSrc +="2. Azimuth calibrate";  
  }else{
      HtmlSrc +="2. Elevation calibrate";
  }
  HtmlSrc +="</span></td></tr><tr>";
  HtmlSrc +="<td class='tdc' colspan='3' style='background-color: #666;'><div style='position: relative;'><canvas class='top' id='Azimuth' width='600' height='140'>Your browser does not support the HTML5 canvas tag.</canvas></div></td>";
  HtmlSrc +="</tr><tr style='background-color: #666;'>";
  HtmlSrc +="<td class='tdl'><button id='setccw' name='setccw'";
  if(AZsource == 1){
    HtmlSrc +=" disabled";
  }
  HtmlSrc +=">&#8676; SAVE CCW</button></td>";
  HtmlSrc +="<td class='tdc' style='background-color: #666;'><button id='clear' name='clear'";
  if(AZsource == 1){
    HtmlSrc +=" disabled>COUNTER AUTO-SAVE</button></td>";
  }else{
    HtmlSrc +=">RESET CW/CCW SAVE</button></td>";
  }
  HtmlSrc +="<td class='tdr'><button id='setcw' name='setcw'";
  if(AZsource == 1){
    HtmlSrc +=" disabled";
  }
  HtmlSrc +=">SAVE CW &#8677;</button></td>";
  HtmlSrc +="</tr><tr>";
  HtmlSrc +="<td class='tdc' colspan='3' style='background-color: #666;'><button id='reverseaz' name='reverseaz'";
  HtmlSrc +=ReverseAzCOLOR;
  if(ELEVATION==false){
    HtmlSrc +=">REVERSE-AZIMUTH-<strong>";
  }else{
    HtmlSrc +=">REVERSE-ELEVATION-<strong>";
  }
  HtmlSrc +=ReverseAzSTATUS;
  HtmlSrc +="</strong></button></td>";
  HtmlSrc +="</tr><tr>";
  HtmlSrc +="<td class='tdc' colspan='3' style='color: #333; background-color: #666; border-bottom-left-radius: 20px; border-bottom-right-radius: 20px;'>";
  if( AZsource == 0 && AZtwoWire == true && CwRaw < 1577 ){
    HtmlSrc +="<span style='color: #ccc;'>Recommendation: </span><span style='color: #0c0;'>If you are using a 2 wire potentiometer less than 500Ω,<br>you can increase the sensitivity if you short the J16 jumper on the back side PCB.<br><br></span>";
  }
  if(AZsource == 1){
    HtmlSrc +="<span style='color: #ccc;'>Instruction:</span><br>&#8226; The CW/CCW pulse counter is available on the 3.5mm jack connector<br>&#8226; CW pulses increase the counter and CCW pulses decrease it<br>&#8226; If the counter changes in the opposite direction, activate REVERSE-AZIMUTH<br>&#8226; The last azimuth is saved automatically after the rotator stops</td>";
  }else if(ELEVATION==false){
    HtmlSrc +="<span style='color: #ccc;'>Instruction:</span><br>&#8226; If azimuth potentiometer move opposite direction (CCW left and CW right),<br>activate REVERSE-AZIMUTH button<br>&#8226; Rotate to both CCW ";
    HtmlSrc +=StartAzimuth;
    HtmlSrc +="&deg; and CW ";
    HtmlSrc +=StartAzimuth+MaxRotateDegree;
    HtmlSrc +="&deg; ends and save new limits<br>&#8226; After calibrate rotate to full CCW limits, then measure real azimuth<br>and put this value to &ldquo;Start CCW azimuth:&rdquo;	field in Setup page</td>";
  }else{
    HtmlSrc +="<span style='color: #ccc;'>Instruction:</span><br>&#8226; If elevation potentiometer move opposite direction (CCW left and CW right),<br>activate REVERSE-ELEVATION button<br>&#8226; Rotate to both CCW 0&deg; and CW ";
    HtmlSrc +=MaxRotateDegree;
    HtmlSrc +="&deg; ends and save new limits<br>&#8226; After calibrate rotate to full CCW limits, then measure real elevation<br>and put this value to &ldquo;Start CCW elevation:&rdquo;	field in Setup page</td>";
  }
  
  HtmlSrc +="</tr><tr>";
  HtmlSrc +="<td class='tdc' colspan='3' style='height:30px'></td></tr>";

  HtmlSrc +="<tr><td class='tdc' colspan='3' style='background-color: #666; border-top-left-radius: 20px; border-top-right-radius: 20px;'><span style='font-size: 200%;'>3. Front panel calibrate (optional)</span></td>";
  HtmlSrc +="</tr><tr>";
  HtmlSrc +="<td class='tdc' colspan='3' style='color: #333; background-color: #666; border-bottom-left-radius: 20px; border-bottom-right-radius: 20px;'>";
  HtmlSrc +="<span style='font-size: 150%;'>Panel value <span style='font-weight: bold; color: #0a0;' id='frontAZValue'>0</span><br></span>";
  HtmlSrc +="<span style='color: #ccc;'><br>Instruction:</span><br>&#8226; Rotate front panel potentiometer axis without knob to value 360&deg <br>&#8226; Put knob with orientation to north on axis<br>&#8226; Fixate knob to axis on position north</td></tr>";

  HtmlSrc +="</table></div><div style='display: flex; justify-content: center;'><span><p style='text-align: center;'><a href='https://remoteqth.com/w/doku.php?id=simple_rotator_interface_v' target='_blank'>More on Wiki &#10138;</a></p></span></div>";

  File calFile = SPIFFS.open("/cal.html", "r");
  if(!calFile){
    ajaxserver.send(500, "text/plain", "Missing /cal.html in SPIFFS");
    return;
  }
  HtmlSrc.reserve(HtmlSrc.length() + calFile.size());
  while(calFile.available()){
    HtmlSrc += char(calFile.read());
  }
  calFile.close();
  ajaxserver.send(200, "text/html", HtmlSrc); //Send web page
}

bool streamStaticFile(const char* path, const char* contentType) {
  File file = SPIFFS.open(path, "r");
  if(!file){
    return false;
  }
  ajaxserver.streamFile(file, contentType);
  file.close();
  return true;
}

void RefreshFilesystemDiagnostics() {
  FsMounted = SPIFFS.begin(false);
  FsBuildInfoPresent = false;
  FsBuildMatchesFirmware = false;
  FsTotalBytes = 0;
  FsUsedBytes = 0;
  FsBuildRev = "";
  FsBuildOffset = "";
  FsBuildSize = "";

  if(!FsMounted){
    Serial.println("SPIFFS mount failed");
    return;
  }

  FsTotalBytes = SPIFFS.totalBytes();
  FsUsedBytes = SPIFFS.usedBytes();
  FsBuildInfoPresent = LoadFilesystemBuildInfo();
  FsBuildMatchesFirmware = FsBuildInfoPresent && FsBuildRev == String(REV);

  Serial.print("SPIFFS mounted: totalBytes=");
  Serial.print(FsTotalBytes);
  Serial.print(" usedBytes=");
  Serial.println(FsUsedBytes);

  if(FsBuildInfoPresent){
    Serial.print("SPIFFS build info: REV=");
    Serial.print(FsBuildRev);
    if(FsBuildOffset.length()){
      Serial.print(" OFFSET=");
      Serial.print(FsBuildOffset);
    }
    if(FsBuildSize.length()){
      Serial.print(" SIZE=");
      Serial.print(FsBuildSize);
    }
    Serial.println();
    Serial.print("FW/FS build match: ");
    Serial.println(FsBuildMatchesFirmware ? "OK" : "MISMATCH");
  }else{
    Serial.println("SPIFFS build info missing: /fs_build.txt");
  }
}

bool LoadFilesystemBuildInfo() {
  File file = SPIFFS.open(FS_BUILD_INFO_PATH, "r");
  if(!file){
    return false;
  }

  String line;
  while(file.available()){
    line = file.readStringUntil('\n');
    line.trim();
    if(line.startsWith("REV=")){
      FsBuildRev = line.substring(4);
    }else if(line.startsWith("SPIFFS_OFFSET=")){
      FsBuildOffset = line.substring(14);
    }else if(line.startsWith("SPIFFS_SIZE=")){
      FsBuildSize = line.substring(12);
    }
  }
  file.close();

  return FsBuildRev.length() > 0;
}

void handleRoot() {
  if(!streamStaticFile("/index.html", "text/html")){
    ajaxserver.send(500, "text/plain", "Missing /index.html in SPIFFS");
  }
}
void handleDxcHtml() {
  if(streamStaticFile("/dxc.html", "text/html")){
    return;
  }
  ajaxserver.send(404, "text/plain", "Missing /dxc.html in SPIFFS");
}
void handleADC() {
 ajaxserver.send(200, "text/plane", String(VoltageValue)); //Send ADC value only to client ajax request
}
void handleAZ() {
  ajaxserver.send(200, "text/plane", String(Azimuth) );
}
void handleFrontAZ() {
  if(AZmasterValue==142){
    ajaxserver.send(200, "text/plane", "off" );
  }else{
    ajaxserver.send(200, "text/plane", String(AZmaster)+"&deg;" );
  }
}
void handleAZadc() {
  ajaxserver.send(200, "text/plane", String(GetPulseVirtualAdcValue()) );
}
void handleAZsource() {
  ajaxserver.send(200, "text/plane", String(AZsource) );
}
void handlePulseCounter() {
  ajaxserver.send(200, "text/plane", String(GetPulseCounterValue()) );
}
void handleStat() {
  ajaxserver.send(200, "text/plane", String(Status+4) );
}
void handleTargetUi() {
  ajaxserver.send(200, "text/plane", String(UiTargetAzimuth) );
}
void handleStart() {
  ajaxserver.send(200, "text/plane", String(StartAzimuth) );
}
void handleElevation() {
  if(ELEVATION==true){
    ajaxserver.send(200, "text/plane", String("1") );
  }else{
    ajaxserver.send(200, "text/plane", String("0") );
  }
}
void handleMax() {
  ajaxserver.send(200, "text/plane", String(MaxRotateDegree) );
}
void handleAnt() {
  ajaxserver.send(200, "text/plane", String(AntRadiationAngle) );
}
void handleAntName() {
  ajaxserver.send(200, "text/plane", RotName );
}
void handleMapUrl() {
  ajaxserver.send(200, "text/plane", MapUrl );
}
void handleMapSource() {
  ajaxserver.send(200, "text/plane", String(MapSource) );
}
void handleMapLocator() {
  ajaxserver.send(200, "text/plane", MapLocator );
}
void handleMapZoomKm() {
  ajaxserver.send(200, "text/plane", String(MapZoomKm) );
}
void handleSetMapLocator() {
  if(!ajaxserver.hasArg("value")){
    ajaxserver.send(400, "text/plane", "Missing value");
    return;
  }
  String NewMapLocator = String(ajaxserver.arg("value"));
  NewMapLocator.trim();
  NewMapLocator.toUpperCase();
  if ( NewMapLocator.length()!=6
    || NewMapLocator[0]<'A' || NewMapLocator[0]>'R'
    || NewMapLocator[1]<'A' || NewMapLocator[1]>'R'
    || NewMapLocator[2]<'0' || NewMapLocator[2]>'9'
    || NewMapLocator[3]<'0' || NewMapLocator[3]>'9'
    || NewMapLocator[4]<'A' || NewMapLocator[4]>'X'
    || NewMapLocator[5]<'A' || NewMapLocator[5]>'X'){
    ajaxserver.send(400, "text/plane", "Invalid locator");
    return;
  }
  if(MapLocator != NewMapLocator){
    MapLocator = NewMapLocator;
    for (int i=0; i<6; i++){
      EEPROM.write(267+i, MapLocator[i]);
    }
    EEPROM.commit();
  }
  ajaxserver.send(200, "text/plane", MapLocator);
}
void handleSetMapZoomKm() {
  if(!ajaxserver.hasArg("value")){
    ajaxserver.send(400, "text/plane", "Missing value");
    return;
  }
  int NewMapZoomKm = ajaxserver.arg("value").toInt();
  if(NewMapZoomKm < 1000 || NewMapZoomKm > 20000){
    ajaxserver.send(400, "text/plane", "Out of range");
    return;
  }
  if(MapZoomKm != unsigned(NewMapZoomKm)){
    MapZoomKm = unsigned(NewMapZoomKm);
    EEPROM.writeUShort(273, MapZoomKm);
    EEPROM.commit();
  }
  ajaxserver.send(200, "text/plane", String(MapZoomKm));
}
void handleMapTheme() {
  ajaxserver.send(200, "text/plane", String(MapTheme) );
}
void handleGraylineDarkness() {
  ajaxserver.send(200, "text/plane", String(GraylineDarkness) );
}
void handleMapDisplayDefaults() {
  ajaxserver.send(200, "text/plane",
    String(DefaultDegOverlayEnabled ? "1" : "0") + "|" +
    String(DefaultAntOverlayEnabled ? "1" : "0") + "|" +
    String(DefaultMapLocGridEnabled ? "1" : "0") + "|" +
    String(DefaultMapGraylineEnabled ? "1" : "0") + "|" +
    String(DefaultMapBordersEnabled ? "1" : "0") + "|" +
    String(DefaultMapDxccEnabled ? "1" : "0")
  );
}
void handleGraylineInfo() {
  time_t now;
  if(GraylineUtcAvailable(&now)){
    ajaxserver.send(200, "text/plane", String("1|") + String((unsigned long)now));
  }else{
    ajaxserver.send(200, "text/plane", "0|0");
  }
}
void handleRev() {
  ajaxserver.send(200, "text/plane", String(REV));
}
void handleFsDiag() {
  String status = "missing";
  if(!FsMounted){
    status = "mount-failed";
  }else if(FsBuildInfoPresent){
    status = FsBuildMatchesFirmware ? "ok" : "mismatch";
  }

  String payload = status + "|" + String(REV) + "|" + FsBuildRev + "|" + FsBuildOffset + "|" + FsBuildSize + "|" + String((unsigned long)FsTotalBytes) + "|" + String((unsigned long)FsUsedBytes);
  ajaxserver.send(200, "text/plain", payload);
}
void handlePwmUi() {
  bool pwmUiEnabled = (ACmotor==false && PWMenable==true);
  ajaxserver.send(200, "text/plane", String(pwmUiEnabled ? 1 : 0) + "|" + String(PwmMaxDuty) + "|" + String(dutyCycle));
}
void handleSetPwmMaxDuty() {
  if(!ajaxserver.hasArg("value")){
    ajaxserver.send(400, "text/plane", "Missing value");
    return;
  }
  int newDuty = ajaxserver.arg("value").toInt();
  if(newDuty < 40 || newDuty > 255){
    ajaxserver.send(400, "text/plane", "Out of range");
    return;
  }
  if(PwmMaxDuty != byte(newDuty)){
    PwmMaxDuty = byte(newDuty);
    EEPROM.writeByte(327, PwmMaxDuty);
    EEPROM.commit();
    MqttPubString("PwmMaxDuty", String(PwmMaxDuty), true);
  }
  ajaxserver.send(200, "text/plane", String(PwmMaxDuty));
}
void handleMap50js() {
  if(streamStaticFile("/map50.js", "application/javascript")){
    return;
  }
  ajaxserver.send(404, "text/plain", "Missing /map50.js in SPIFFS");
}
void handleMap50jsGz() {
  File file = SPIFFS.open("/map50.js.gz", "r");
  if(file){
    ajaxserver.streamFile(file, "application/gzip");
    file.close();
    return;
  }
  ajaxserver.send(404, "text/plain", "Missing /map50.js.gz in SPIFFS");
}
void handleFontRegular() {
  if(streamStaticFile("/RC-R.ttf", "font/ttf")){
    return;
  }
  ajaxserver.send(404, "text/plain", "Missing /RC-R.ttf in SPIFFS");
}
void handleFontBold() {
  if(streamStaticFile("/RC-B.ttf", "font/ttf")){
    return;
  }
  ajaxserver.send(404, "text/plain", "Missing /RC-B.ttf in SPIFFS");
}
void handleBackupConfigDownload() {
  ajaxserver.sendHeader("Content-Disposition", "attachment; filename=\"ip-rotator-config.json\"");
  ajaxserver.send(200, "application/json; charset=utf-8", ExportConfigBackupJson());
}
void handleBackupConfigUploadData() {
  HTTPUpload& upload = ajaxserver.upload();
  if(upload.status == UPLOAD_FILE_START){
    ConfigBackupUploadBuffer = "";
    ConfigBackupUploadBuffer.reserve(3072);
    ConfigBackupUploadError = "";
  }else if(upload.status == UPLOAD_FILE_WRITE){
    if(ConfigBackupUploadError.length() == 0){
      if(ConfigBackupUploadBuffer.length() + upload.currentSize > 8192){
        ConfigBackupUploadError = "Backup file is too large";
      }else{
        for(size_t i = 0; i < upload.currentSize; i++){
          ConfigBackupUploadBuffer += char(upload.buf[i]);
        }
      }
    }
  }else if(upload.status == UPLOAD_FILE_END){
    if(ConfigBackupUploadBuffer.length() == 0 && ConfigBackupUploadError.length() == 0){
      ConfigBackupUploadError = "Backup file is empty";
    }
  }
}
void handleBackupConfigUpload() {
  if(ConfigBackupUploadError.length() > 0){
    ajaxserver.send(400, "text/plain", ConfigBackupUploadError);
  }else{
    String importError = ImportConfigBackupJson(ConfigBackupUploadBuffer);
    if(importError.length() > 0){
      ajaxserver.send(400, "text/plain", importError);
    }else{
      ajaxserver.send(200, "text/plain", "Backup restored. Refresh the main page to apply all changes.");
    }
  }
  ConfigBackupUploadBuffer = "";
  ConfigBackupUploadError = "";
}
void handleEndstop() {
  ajaxserver.send(200, "text/plane", String(Endstop) );
}
void handleEndstopLowZone() {
  ajaxserver.send(200, "text/plane", String(NoEndstopLowZone) );
}
void handleEndstopHighZone() {
  ajaxserver.send(200, "text/plane", String(NoEndstopHighZone) );
}
void handleSetEndstopZones() {
  if(!ajaxserver.hasArg("low") || !ajaxserver.hasArg("high")){
    ajaxserver.send(400, "text/plain", "Missing low/high");
    return;
  }

  int lowTenth = ajaxserver.arg("low").toInt();
  int highTenth = ajaxserver.arg("high").toInt();
  if(lowTenth < 2 || lowTenth > 15 || highTenth < 16 || highTenth > 31){
    ajaxserver.send(400, "text/plain", "Out of range");
    return;
  }

  float newLowZone = float(lowTenth) / 10.0;
  float newHighZone = float(highTenth) / 10.0;
  bool changed = false;

  if(NoEndstopLowZone != newLowZone){
    NoEndstopLowZone = newLowZone;
    EEPROM.writeByte(36, lowTenth);
    MqttPubString("NoEndstopLowZone", String(NoEndstopLowZone), true);
    changed = true;
  }

  if(NoEndstopHighZone != newHighZone){
    NoEndstopHighZone = newHighZone;
    EEPROM.writeByte(222, highTenth);
    MqttPubString("NoEndstopHighZone", String(NoEndstopHighZone), true);
    changed = true;
  }

  if(changed){
    EEPROM.commit();
  }

  ajaxserver.send(200, "text/plain", "OK");
}
void handleCwraw() {
  if(IsPulseAzimuthSource()){
    ajaxserver.send(200, "text/plane", "3155" );
  }else{
    ajaxserver.send(200, "text/plane", String(CwRaw) );
  }
}
void handleCcwraw() {
  if(IsPulseAzimuthSource()){
    ajaxserver.send(200, "text/plane", "142" );
  }else{
    ajaxserver.send(200, "text/plane", String(CcwRaw) );
  }
}
void handleMAC() {
  ajaxserver.send(200, "text/plane", String(MACString) );
}
void handleUptime() {
  ajaxserver.send(200, "text/plane", String(millis()/1000) );
}

bool DxcConfigReady(){
  return DxcHost.length() > 0 && DxcPort > 0 && DxcCallsign.length() > 0;
}

void DxcDisconnectTelnet(){
  if(DxcTelnetClient.connected()){
    DxcTelnetClient.stop();
  }
  DxcTelnetLoginPending = false;
  DxcUpdateTelnetStatus(false);
}

void DxcDisconnectWebSocket(){
  if(DxcWsClient.connected()){
    DxcWsClient.stop();
  }
  DxcWsStatus = false;
  DxcDisconnectTelnet();
}

void DxcRequestReconnect(){
  DxcDisconnectTelnet();
  DxcReconnectTimer = millis() + 250;
}

void DxcUpdateTelnetStatus(bool connected, bool forceSend){
  if(!forceSend && DxcTelnetStatus == connected){
    return;
  }
  DxcTelnetStatus = connected;
  DxcSendTelnetStatus();
}

void DxcSendTelnetStatus(){
  if(!DxcWsClient.connected()){
    return;
  }
  String payload = String("{\"telnet\":") + String(DxcTelnetStatus ? "true" : "false") + "}";
  DxcSendWebSocketText(payload);
}

bool DxcConnectTelnet(){
  if(!DxcWsClient.connected() || !DxcConfigReady()){
    DxcUpdateTelnetStatus(false);
    return false;
  }
  if(DxcTelnetClient.connected()){
    return true;
  }
  WiFiClient newClient;
  newClient.setNoDelay(true);
  if(!newClient.connect(DxcHost.c_str(), DxcPort)){
    DxcReconnectTimer = millis() + 5000;
    DxcUpdateTelnetStatus(false);
    return false;
  }
  DxcTelnetClient = newClient;
  DxcTelnetLoginPending = true;
  DxcUpdateTelnetStatus(true, true);
  return true;
}

String Base64Encode(const uint8_t* data, size_t length){
  static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String encoded = "";
  encoded.reserve(((length + 2) / 3) * 4);
  for(size_t i = 0; i < length; i += 3){
    uint32_t block = uint32_t(data[i]) << 16;
    bool hasSecond = (i + 1) < length;
    bool hasThird = (i + 2) < length;
    if(hasSecond){
      block |= uint32_t(data[i + 1]) << 8;
    }
    if(hasThird){
      block |= uint32_t(data[i + 2]);
    }
    encoded += alphabet[(block >> 18) & 0x3F];
    encoded += alphabet[(block >> 12) & 0x3F];
    encoded += hasSecond ? alphabet[(block >> 6) & 0x3F] : '=';
    encoded += hasThird ? alphabet[block & 0x3F] : '=';
  }
  return encoded;
}

String DxcComputeWebSocketAccept(const String& secKey){
  String source = secKey;
  source += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  uint8_t digest[20];
  mbedtls_sha1(reinterpret_cast<const unsigned char*>(source.c_str()), source.length(), digest);
  return Base64Encode(digest, sizeof(digest));
}

bool DxcSendWebSocketFrame(uint8_t opcode, const uint8_t* payload, size_t length){
  if(!DxcWsClient.connected()){
    return false;
  }
  uint8_t header[10];
  size_t headerLen = 0;
  header[headerLen++] = 0x80 | (opcode & 0x0F);
  if(length < 126){
    header[headerLen++] = uint8_t(length);
  }else if(length <= 0xFFFF){
    header[headerLen++] = 126;
    header[headerLen++] = uint8_t((length >> 8) & 0xFF);
    header[headerLen++] = uint8_t(length & 0xFF);
  }else{
    header[headerLen++] = 127;
    for(int shift = 56; shift >= 0; shift -= 8){
      header[headerLen++] = uint8_t((uint64_t(length) >> shift) & 0xFF);
    }
  }
  if(DxcWsClient.write(header, headerLen) != headerLen){
    DxcDisconnectWebSocket();
    return false;
  }
  if(length > 0 && payload != nullptr){
    if(DxcWsClient.write(payload, length) != length){
      DxcDisconnectWebSocket();
      return false;
    }
  }
  return true;
}

bool DxcSendWebSocketText(const char* text){
  if(text == nullptr){
    return DxcSendWebSocketFrame(0x1, nullptr, 0);
  }
  return DxcSendWebSocketFrame(0x1, reinterpret_cast<const uint8_t*>(text), strlen(text));
}

bool DxcSendWebSocketText(const String& text){
  return DxcSendWebSocketFrame(0x1, reinterpret_cast<const uint8_t*>(text.c_str()), text.length());
}

bool DxcHandleWebSocketUpgrade(WiFiClient& webClient, const String& request, const String& method, const String& uri){
  if(method != "GET" || uri != "/dxcws"){
    return false;
  }
  String secKey = ExtractHttpHeader(request, "Sec-WebSocket-Key");
  String upgrade = ExtractHttpHeader(request, "Upgrade");
  String connection = ExtractHttpHeader(request, "Connection");
  String upgradeLower = upgrade;
  String connectionLower = connection;
  upgradeLower.toLowerCase();
  connectionLower.toLowerCase();
  if(secKey.length() == 0 || upgradeLower != "websocket" || connectionLower.indexOf("upgrade") < 0){
    webClient.println(F("HTTP/1.1 400 Bad Request"));
    webClient.println(F("Content-Type: text/plain"));
    webClient.println(F("Connection: close"));
    webClient.println();
    webClient.println(F("Invalid WebSocket handshake"));
    return true;
  }
  if(DxcWsClient.connected()){
    DxcDisconnectWebSocket();
  }
  String accept = DxcComputeWebSocketAccept(secKey);
  webClient.println(F("HTTP/1.1 101 Switching Protocols"));
  webClient.println(F("Upgrade: websocket"));
  webClient.println(F("Connection: Upgrade"));
  webClient.print(F("Sec-WebSocket-Accept: "));
  webClient.println(accept);
  webClient.println();
  DxcWsClient = webClient;
  DxcWsClient.setNoDelay(true);
  DxcWsStatus = true;
  DxcUpdateTelnetStatus(DxcTelnetClient.connected(), true);
  DxcRequestReconnect();
  return true;
}

void DxcHandleWebSocketClient(){
  if(!DxcWsClient.connected()){
    if(DxcWsStatus){
      DxcWsStatus = false;
      DxcDisconnectTelnet();
    }
    return;
  }
  while(DxcWsClient.available() >= 2){
    uint8_t hdr[2];
    if(DxcWsClient.read(hdr, 2) != 2){
      DxcDisconnectWebSocket();
      return;
    }
    uint8_t opcode = hdr[0] & 0x0F;
    bool masked = (hdr[1] & 0x80) != 0;
    uint64_t payloadLen = hdr[1] & 0x7F;

    if(payloadLen == 126){
      uint8_t ext[2];
      while(DxcWsClient.connected() && DxcWsClient.available() < 2){
        delay(1);
      }
      if(DxcWsClient.read(ext, 2) != 2){
        DxcDisconnectWebSocket();
        return;
      }
      payloadLen = (uint16_t(ext[0]) << 8) | uint16_t(ext[1]);
    }else if(payloadLen == 127){
      uint8_t ext[8];
      while(DxcWsClient.connected() && DxcWsClient.available() < 8){
        delay(1);
      }
      if(DxcWsClient.read(ext, 8) != 8){
        DxcDisconnectWebSocket();
        return;
      }
      payloadLen = 0;
      for(int i = 0; i < 8; i++){
        payloadLen = (payloadLen << 8) | ext[i];
      }
    }

    uint8_t maskKey[4] = {0, 0, 0, 0};
    if(masked){
      while(DxcWsClient.connected() && DxcWsClient.available() < 4){
        delay(1);
      }
      if(DxcWsClient.read(maskKey, 4) != 4){
        DxcDisconnectWebSocket();
        return;
      }
    }

    if(payloadLen > 2048){
      DxcDisconnectWebSocket();
      return;
    }

    static uint8_t payload[2048];
    size_t needed = size_t(payloadLen);
    while(DxcWsClient.connected() && DxcWsClient.available() < int(needed)){
      delay(1);
    }
    if(needed > 0 && DxcWsClient.read(payload, needed) != int(needed)){
      DxcDisconnectWebSocket();
      return;
    }
    if(masked){
      for(size_t i = 0; i < needed; i++){
        payload[i] ^= maskKey[i % 4];
      }
    }

    if(opcode == 0x8){
      DxcDisconnectWebSocket();
      return;
    }
    if(opcode == 0x9){
      DxcSendWebSocketFrame(0xA, payload, needed);
      continue;
    }
    if(opcode != 0x1){
      continue;
    }

    String command = "";
    command.reserve(needed + 1);
    for(size_t i = 0; i < needed; i++){
      if(payload[i] != '\0'){
        command += char(payload[i]);
      }
    }
    command.trim();
    if(command.length() == 0){
      continue;
    }
    if(command == "@reconnect"){
      DxcRequestReconnect();
      continue;
    }
    if(!DxcTelnetClient.connected()){
      DxcRequestReconnect();
      continue;
    }
    DxcTelnetClient.print(command);
    DxcTelnetClient.print("\r\n");
  }
}

void DxcHandleTelnetClient(){
  if(!DxcWsClient.connected()){
    if(DxcTelnetClient.connected()){
      DxcDisconnectTelnet();
    }
    return;
  }

  if(!DxcTelnetClient.connected()){
    DxcUpdateTelnetStatus(false);
    return;
  }

  if(DxcTelnetLoginPending && DxcCallsign.length() > 0){
    DxcTelnetClient.print(DxcCallsign);
    DxcTelnetClient.print("\r\n");
    DxcTelnetLoginPending = false;
  }

  static uint8_t telnetBuffer[1024];
  while(DxcTelnetClient.available()){
    int chunk = DxcTelnetClient.read(telnetBuffer, sizeof(telnetBuffer));
    if(chunk <= 0){
      break;
    }
    if(!DxcSendWebSocketFrame(0x1, telnetBuffer, size_t(chunk))){
      return;
    }
  }
}

void DxcLoop(){
  DxcHandleWebSocketClient();
  if(!DxcWsClient.connected()){
    DxcDisconnectTelnet();
    return;
  }
  if(!DxcTelnetClient.connected() && DxcConfigReady() && millis() >= DxcReconnectTimer){
    DxcConnectTelnet();
  }
  DxcHandleTelnetClient();
}
