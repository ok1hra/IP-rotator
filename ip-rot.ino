/*

3D printed IP rotator
----------------------
Compile for HARDWARE ESP32-POE

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

MQTT monitor
mosquitto_sub -v -h 192.168.1.200 -t 'OK1HRA/ROT/#'
mosquitto_sub -v -h 54.38.157.134 -t 'BD:2F/ROT/#'

MQTT topic
mosquitto_pub -h 192.168.1.200 -t OK1HRA/ROT/Target -m '10'
mosquitto_pub -h 54.38.157.134 -t BD:2F/ROT/Target -m '10'


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

ToDo
- test
  - if stop, after run over target
  + 10 turn pot without preamp
  + test preamp linearity

- HW rev 06 - change POE voltage limit with enable AMP to 13,0V (2V dropout 11V LDO)
- PWM up/dwn ramp long to setup page, and ° before target start PWM (measure and saved) - PwmDwnDelay/PwmUpDelay set from setup gui
- save settings (dump eeprom?)
- show target from az pot in map
- disable target in map if status = 0 (not rotate)
- need implement
  - CW/CCW pulse functionality
  - CW/CCW pulse pulse per grad
  - serial protocol easycomm2
- watchdog if eth_connected = false; > 5s, then reconect
- do debugu vypisovat prumer casu behu hlavni smycky v ms
- telnet
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
const char* REV = "20230722";

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
                //$ /usr/bin/xplanet -window -config ./geoconfig -longitude 13.8 -latitude 50.0 -geometry 600x600 -projection azimuthal -num_times 1 -output ./map.png
                //$ /usr/bin/xplanet -window -config ./geoconfig -longitude 13.8 -latitude 50.0 -geometry 600x600 -projection azimuthal -radius 500 -num_times 1 -output ./OK500.png
bool Endstop =  false;
bool ACmotor =  false;
bool AZsource =  false;
short PulsePerDegree = 0;
bool AZtwoWire =  false;
bool AZpreamp =  false;

bool PWMenable = true;
unsigned int PwmUpDelay  = 3;  // [ms]*255
unsigned int PwmDwnDelay = 2;  // [ms]*255
byte dutyCycle = 0;
long StatusWatchdogTimer = 0;
long RotateWatchdogTimer = 0;
int AzimuthWatchdog = 0;
bool ErrorDetect = 0;

//---------------------------------------------------------
// #define HWREV 8                     // PCB version [7-8]
#define OTAWEB                      // enable upload firmware via web
// #define DS18B20                     // external 1wire Temperature sensor
// #define BMP280                      // pressure I2C sensor
// #define HTU21D                      // humidity I2C sensor
// #define SHT21                       // humidity I2C sensor
// #define SHT                         // general SHT sensors I2C sensor https://github.com/Sensirion/arduino-sht/blob/master/examples/multiple-sht-sensors/multiple-sht-sensors.ino
// #define RF69_EXTERNAL_SENSOR        // RX temp and humidity radio sensor RF69
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
const char* ssid     = "";
const char* password = "";
//-------------------------------------------------------------------------------------------------------

#include "esp_adc_cal.h"
const int AzimuthPin    = 39;  // analog
float AzimuthValue      = 0.0;
int Azimuth             = 0;
int AzimuthTarget       = 0;
int Status              = 0; // -3 PwmDwnCCW|-2 CCW|-1 PwmUpCCW|0 off|1 PwmUpCW|2 CW|3 PwmDwnCW
const int VoltagePin    = 35;  // analog
float VoltageValue      = 0.0;
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
char key[100];
long MeasureTimer[2]={2800000,300000};   //  millis,timer (5 min)


int RainCount;
String RainCountDayOfMonth;
bool RainStatus;
/*
1mm rain = 15,7cm^2/10 = 1,57ml     <- by rain funnel radius
10ml = 11,5 pulses = 0,87ml/pulse   <- constanta tilting measuring cup
*/
float mmInPulse = 0.2 ; // callibration rain 17,7-20,2 mm with 95 pulse

int WindDir = 0;
int WindDirShift = 0;

long RpmTimer[2]={0,3000};
long RpmPulse = 987654321;
long PeriodMinRpmPulse = 987654321;
String PeriodMinRpmPulseTimestamp;
long MinRpmPulse;
String MinRpmPulseTimestamp;
unsigned int RpmSpeed = 0;
unsigned long RpmAverage[2]={1,0};  // counter,sum time
bool RpmInterrupt = false;

int SpeedAlertLimit_ms = 0;
int SpeedAlert_ms = 3000;
// bool NeedSpeedAlert_ms = false;
long AlertTimer[2]={0,60000};
//  |alert...........|alert........... everry max 1 minutes and with publish max value in period

// 73 seconds WDT (WatchDogTimer)
#include <esp_task_wdt.h>
#define WDT_TIMEOUT 73
long WdtTimer=0;

byte InputByte[21];
// #define Ser2net                  // Serial to ip proxy - DISABLE if board revision 0.3 or lower
#define EnableOTA                // Enable flashing ESP32 Over The Air
int NumberOfEncoderOutputs = 8;  // 2-16
int EnableSerialDebug     = 0;
long FreneticModeTimer ;
#define HTTP_SERVER_PORT  80     // Web server port
int IncomingSwitchUdpPort;
#define ShiftOut                 // Enable ShiftOut register
#define UdpAnswer                // Send UDP answer confirm packet
int BroadcastPort;               // destination broadcast packet port
bool EnableGroupPrefix = 0;      // enable multi controller control
bool EnableGroupButton = 0;      // group to one from
unsigned int GroupButton[8]={1,2,3,4,5,6,7,8};
byte DetectedRemoteSw[16][4];
unsigned int DetectedRemoteSwPort[16];

int BaudRate = 115200; // serial debug baudrate
int SERIAL1_BAUDRATE; // serial1 to IP baudrate
int incomingByte = 0;   // for incoming serial data

int i = 0;
#include <WiFi.h>
#include <WiFiUdp.h>
#include "EEPROM.h"
#define EEPROM_SIZE 244   /*

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
37-40 - Authorised telnet client IP
41-140 - Authorised telnet client key
141-160 - YOUR_CALL
161-164 - MQTT broker IP
165-166 - MQTT_PORT
167-168 - none
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

231-234 - Altitude 4
// 232 - SpeedAlert 4
235-238 - SpeedAlert 4
// 233 - DS18B20 on/off 1
239 - DS18B20 on/off 1
240-243 - WindDirShift 4

!! Increment EEPROM_SIZE #define !!

*/
int Altitude = 0;
bool needEEPROMcommit = false;
unsigned int RebootWatchdog;
unsigned int OutputWatchdog;
unsigned long WatchdogTimer=0;

//ajax
#include <WebServer.h>
#include "index.h"  //Web page header file
#include "index-cal.h"  //Web page header file
WebServer ajaxserver(HTTP_SERVER_PORT+8);

WiFiServer server(HTTP_SERVER_PORT);
#if defined(CN3A)
  bool DHCP_ENABLE = 0;
#else
  bool DHCP_ENABLE = 1;
#endif
// Client variables
char linebuf[80];
int charcount=0;
//Are we currently connected?
boolean connected = false;
//The udp library class
WiFiUDP UdpCommand;
uint8_t buffer[50] = "";
unsigned char packetBuffer[10];
int UDPpacketSize;
byte TxUdpBuffer[8];
#include <ETH.h>
static bool eth_connected = false;
IPAddress RemoteSwIP(0, 0, 0, 0);         // remote UDP IP switch - set from UDP DetectRemote array
int RemoteSwPort         = 0;             // remote UDP IP switch port
String HTTP_req;
#if defined(EnableOTA)
  #include <ESPmDNS.h>
  #include <ArduinoOTA.h>
#endif
#if defined(OTAWEB)
  #include <AsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <AsyncElegantOTA.h>
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
boolean MQTT_LOGIN      = 0;          // enable MQTT broker login
// char MQTT_USER= 'login';    // MQTT broker user login
// char MQTT_PASS= 'passwd';   // MQTT broker password
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

#if defined(BMP280)||defined(HTU21D)||defined(SHT21)
  // #include <SPI.h>
  #include <Adafruit_Sensor.h>
  TwoWire I2Cone = TwoWire(0);
#endif

#if defined(BMP280)
  #include <Adafruit_BMP280.h>
  Adafruit_BMP280 bmp(&I2Cone); // use I2C interface
  Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
  Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();
  bool BMP280enable;
#endif

#if defined(HTU21D)
  #include "Adafruit_HTU21DF.h"
  Adafruit_HTU21DF htu = Adafruit_HTU21DF();
  bool HTU21Denable;
#endif

#if defined(SHT21)
  #include "SHT2x.h"
  SHT2x internal;
  // SHT2x external;
#endif

#if defined(SHT)
  #include "SHTSensor.h"
  // Sensor with normal i2c address
  // Sensor 1 with address pin pulled to GND
  SHTSensor sht1(SHTSensor::SHT3X);
  // Sensor with alternative i2c address
  // Sensor 2 with address pin pulled to Vdd
  // SHTSensor sht2(SHTSensor::SHT3X_ALT);
#endif

// https://github.com/PaulStoffregen/RadioHead
#if defined(RF69_EXTERNAL_SENSOR)
  bool RF69enable;
  #include <SPI.h>
  #include <RH_RF69.h>
  // Change to 434.0 or other frequency, must match RX's freq!
  #define RF69_FREQ 434.0
  #define RFM69_RST     -1   // same as LED
  #define RFM69_CS      0   // "B"
  #define RFM69_INT     16   // "A"

  // Singleton instance of the radio driver
  RH_RF69 rf69(RFM69_CS, RFM69_INT);

  int16_t packetnum = 0;  // packet counter, we increment per xmission
  String received_data;
  float humidity, temp_f, temp_f2, temp_PT100, temp_dallas;
  String temp_radio;
  String humidity_radio;
  String vbat_radio;

  uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
#endif

const int RpmPin = 39;
#if HWREV==8
  const int RainPin = 36;
#endif
#if HWREV==7
  const int Rain1Pin = 36;
  const int Rain2Pin = 35;
#endif
// const int EnablePin = 13;
// const int ButtonPin = 34;

#if defined(Ser2net)
  #define RX1 3
  #define TX1 1
  HardwareSerial Serial_one(1);
#endif

const int MappingRow = 5;
const long mapping[MappingRow][2] = { // ms > m/s
  {987654321,0},
  {120,1},
  {50,2},
  {2,4},
  {1,200},
};
// WX end

// SD
// #define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
// #define ETH_PHY_POWER 12
// #include "FS.h"
// #include "SD_MMC.h"

// ntp
#include "time.h"
const char* ntpServer = "pool.ntp.org";
// const char* ntpServer = "tik.cesnet.cz";
// const char* ntpServer = "time.google.com";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;

#define MAX_SRV_CLIENTS 1
int SerialServerIPport;
// WiFiServer SerialServer(SerialServerIPport);
WiFiServer SerialServer;
WiFiClient SerialServerClients[MAX_SRV_CLIENTS];

int TelnetServerIPport = 23;
WiFiServer TelnetServer;
WiFiClient TelnetServerClients[MAX_SRV_CLIENTS];
IPAddress TelnetServerClientAuth;
bool TelnetAuthorized = false;
int TelnetAuthStep=0;
int TelnetAuthStepFails=0;
int TelnetLoginFails=0;
long TelnetLoginFailsBanTimer[2]={0,600000};
int RandomNumber;
bool FirstListCommands=true;

int CompareInt;

// APRS
WiFiClient AprsClient;
boolean AprsON = false;
uint16_t AprsPort;
IPAddress aprs_server_ip(0, 0, 0, 0);
String AprsPassword;
String AprsCoordinates;

// DS18B20
#if defined(DS18B20)
  // #include <OneWire.h>
  // #include <DallasTemperature.h>
  // // const int DsPin = 3;
  // // OneWire ds(DsPin);
  // // DallasTemperature sensors(&ds);
  // const int oneWireBus = 13;
  // OneWire oneWire(oneWireBus);
  // DallasTemperature sensors(&oneWire);
  bool ExtTemp = true;

  // Include the libraries we need
  #include <OneWire.h>
  #include <DallasTemperature.h>
  // Data wire is plugged into port 2 on the Arduino
  #if HWREV==8
    #define ONE_WIRE_BUS 2
  #endif
  #if HWREV==7
    #define ONE_WIRE_BUS 13
  #endif
  #define TEMPERATURE_PRECISION 9 // 9-12
  // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
  OneWire oneWire(ONE_WIRE_BUS);
  // Pass our oneWire reference to Dallas Temperature.
  DallasTemperature sensors(&oneWire);
  // arrays to hold device addresses
  DeviceAddress insideThermometer, outsideThermometer;
  // Assign address manually. The addresses below will need to be changed
  // to valid device addresses on your bus. Device address can be retrieved
  // by using either oneWire.search(deviceAddress) or individually via
  // sensors.getAddress(deviceAddress, index)
  // DeviceAddress insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };
  // DeviceAddress outsideThermometer   = { 0x28, 0x3F, 0x1C, 0x31, 0x2, 0x0, 0x0, 0x2 };

#endif

//-------------------------------------------------------------------------------------------------------

void setup() {

  pinMode(AzimuthPin, INPUT);
  // pinMode(CwCcwButtPin, INPUT);
  pinMode(CwInputPin, INPUT);
  pinMode(CcwInputPin, INPUT);

  pinMode(HWidPin, INPUT);
    HWidValue = readADC_Cal(analogRead(HWidPin));
    if(HWidValue<=150){
      HardwareRev=0;
    }else if(HWidValue>150 && HWidValue<=450){
      HardwareRev=3;  // 319
    }else if(HWidValue>450 && HWidValue<=700){
      HardwareRev=4;  // 604
    }else if(HWidValue>700){
      HardwareRev=5;  // 807
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
  #if HWREV==8
    pinMode(RainPin, INPUT);
  #endif
  #if HWREV==7
    pinMode(Rain1Pin, INPUT);
    pinMode(Rain2Pin, INPUT);
  #endif

  pinMode(RpmPin, INPUT);
  // pinMode(EnablePin, OUTPUT);
  // digitalWrite(EnablePin,1);

  // pinMode(ButtonPin, INPUT);
  // SHIFT IN
  // pinMode(ShiftInLatchPin, OUTPUT);
  // pinMode(ShiftInClockPin, OUTPUT);
  // pinMode(ShiftInDataPin, INPUT);

  Serial.begin(115200); //BaudRate
  while(!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  #if defined(DS18B20)
    sensors.begin();

    // locate devices on the bus
    Serial.print("DS18B20 found ");
    Serial.print(sensors.getDeviceCount(), DEC);
    Serial.println(" devices.");

    // report parasite power requirements
    //  Serial.print("Parasite power is: ");
    //  if (sensors.isParasitePowerMode()) Serial.println("ON");
    //  else Serial.println("OFF");

    // Search for devices on the bus and assign based on an index. Ideally,
    // you would do this to initially discover addresses on the bus and then
    // use those addresses and manually assign them (see above) once you know
    // the devices on your bus (and assuming they don't change).
    //
    // method 1: by index
    if (!sensors.getAddress(insideThermometer, 0)){
      ExtTemp = false;
      Serial.println("DS18B20 unable to find address for Device 0");
    }
    //  if (!sensors.getAddress(outsideThermometer, 1)) Serial.println("Unable to find address for Device 1");

    // method 2: search()
    // search() looks for the next device. Returns 1 if a new address has been
    // returned. A zero might mean that the bus is shorted, there are no devices,
    // or you have already retrieved all of them. It might be a good idea to
    // check the CRC to make sure you didn't get garbage. The order is
    // deterministic. You will always get the same devices in the same order
    //
    // Must be called before search()
    //oneWire.reset_search();
    // assigns the first address found to insideThermometer
    //if (!oneWire.search(insideThermometer)) Serial.println("Unable to find address for insideThermometer");
    // assigns the seconds address found to outsideThermometer
    //if (!oneWire.search(outsideThermometer)) Serial.println("Unable to find address for outsideThermometer");

    // show the addresses we found on the bus
    Serial.print("DS18B20 device 0 Address: ");
    printAddress(insideThermometer);
    Serial.println();

    //  Serial.print("Device 1 Address: ");
    //  printAddress(outsideThermometer);
    //  Serial.println();

    // set the resolution to 9 bit per device
    sensors.setResolution(insideThermometer, TEMPERATURE_PRECISION);
    //  sensors.setResolution(outsideThermometer, TEMPERATURE_PRECISION);

    Serial.print("DS18B20 device 0 Resolution: ");
    Serial.print(sensors.getResolution(insideThermometer), DEC);
    Serial.println();

    //  Serial.print("Device 1 Resolution: ");
    //  Serial.print(sensors.getResolution(outsideThermometer), DEC);
    //  Serial.println();
  #endif

  #if defined(BMP280)
    // I2Cone.begin(0x76, I2C_SDA, I2C_SCL, 100000); // SDA pin, SCL pin, 100kHz frequency
    // I2Cone.begin(I2C_SDA, I2C_SCL, (uint32_t)100000); // SDA pin, SCL pin, 100kHz frequency
    Serial.print("BMP280 sensor init ");
    if(!bmp.begin(0x76)){
      Serial.println("failed!");
      // while (1) delay(10);
      BMP280enable=false;
    }else{
      Serial.println("OK");
      BMP280enable=true;
      /* Default settings from datasheet. */
      bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
        Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
        Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
        Adafruit_BMP280::FILTER_X16,      /* Filtering. */
        Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
        bmp_temp->printSensorDetails();
    }
  #endif

  #if defined(HTU21D) || defined(SHT)
    // Wire.begin(I2C_SDA, I2C_SCL);
  #endif
  #if defined(HTU21D)
    Serial.print("HTU21D sensor init ");
    if(!htu.begin()){
      Serial.println("failed!");
      // while (1);
      HTU21Denable=false;
    }else{
      Serial.println("OK");
      HTU21Denable=true;
    }
  #endif

  #if defined(SHT21)
    Serial.println(__FILE__);
    Serial.print("SHT2x_LIB_VERSION: \t");
    Serial.println(SHT2x_LIB_VERSION);

    internal.begin(&I2Cone);
    // external.begin(&I2Ctwo);

    uint8_t stat = internal.getStatus();
    Serial.print(stat, HEX);
    Serial.println();
    // stat = external.getStatus();
    // Serial.print(stat, HEX);
    // Serial.println();
    Serial.println();
  #endif

  #if !defined(BMP280) && !defined(HTU21D)
    // Wire.begin(I2C_SDA, I2C_SCL);
  #endif

  // // SD
  // if(!SD_MMC.begin()){
  //   Serial.println("SD card Mount Failed");
  //   // return;
  // }

  // Listen source
  if (!EEPROM.begin(EEPROM_SIZE)){
    if(EnableSerialDebug>0){
      Serial.println("failed to initialise EEPROM"); delay(1);
    }
  }

  // 0-1 net ID
    if(EEPROM.read(0)==0xff){
      NET_ID="0";
    }else{
      for (int i=0; i<2; i++){
        if(EEPROM.read(i)!=0xff){
          NET_ID=NET_ID+char(EEPROM.read(i));
        }
      }
    }

  // 2-22 RotName
  if(EEPROM.read(2)==0xff){
    RotName="Antenna";
  }else{
    for (int i=2; i<23; i++){
      if(EEPROM.read(i)!=0xff){
        RotName=RotName+char(EEPROM.read(i));
      }
    }
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
    for (int i=169; i<220; i++){
      if(EEPROM.read(i)!=0xff){
        MapUrl=MapUrl+char(EEPROM.read(i));
      }
    }
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
  // NoEndstopHighZone = 3.3 - NoEndstopLowZone;
  // NoEndstopLowZone = NoEndstopLowZone;

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
    AZsource=false;
  }else{
    if(EEPROM.readBool(223)==1){
      AZsource=true;
    }else{
      AZsource=false;
    }
  }

  // 224-225  PulsePerDegree
  if(EEPROM.read(224)==0xff){
    PulsePerDegree=1;
  }else{
    PulsePerDegree = EEPROM.readUShort(224);
  }

  // 226-227 BaudRate
  if(EEPROM.read(226)==0xff || EEPROM.readUShort(226) > 9600){
    BaudRate=115200;
  }else{
    BaudRate = EEPROM.readUShort(226);
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





  // 2-encoder range
  NumberOfEncoderOutputs = EEPROM.read(2);
  if(NumberOfEncoderOutputs < 2 || NumberOfEncoderOutputs > 0x0f){
    NumberOfEncoderOutputs=8;
  }

  // 4-EnableGroupPrefix
  // Serial.print("Enable group NET-ID prefix ");
  if(EEPROM.read(4)<2){
    EnableGroupPrefix=EEPROM.read(4);
  }
  // Serial.println(EnableGroupPrefix);

  // 5-EnableGroupButton
  if(EEPROM.read(5)<2){
    EnableGroupButton=EEPROM.read(5);
  }

  // 6-13 ButtonGroup
  if(EnableGroupButton==true){
    for (int i = 0; i < 8; i++) {
      if(EEPROM.read(6+i)<9){
        GroupButton[i]=EEPROM.read(6+i);
      }
    }
  }

  SERIAL1_BAUDRATE=EEPROM.readInt(14);
  SerialServerIPport=EEPROM.readInt(18);
  IncomingSwitchUdpPort=EEPROM.readInt(22);
  BroadcastPort=IncomingSwitchUdpPort;
  RebootWatchdog=EEPROM.readUInt(26);
  if(RebootWatchdog>10080){
    RebootWatchdog=0;
  }
  OutputWatchdog=EEPROM.readUInt(30);
  if(OutputWatchdog>10080){
    OutputWatchdog=0;
  }
  // if(RebootWatchdog>0){
  //   ShiftOutByte[0]=EEPROM.readByte(34);
  //   ShiftOutByte[1]=EEPROM.readByte(35);
  //   ShiftOutByte[2]=EEPROM.readByte(36);
  // }
  TelnetServerClientAuth[0]=EEPROM.readByte(37);
  TelnetServerClientAuth[1]=EEPROM.readByte(38);
  TelnetServerClientAuth[2]=EEPROM.readByte(39);
  TelnetServerClientAuth[3]=EEPROM.readByte(40);

  // 41-140 key
  // if clear, generate
  if(EEPROM.readByte(41)==255 && EEPROM.readByte(140)==255){
    Serial.println();
    Serial.println("  ** GENERATE KEY **");
    for(int i=41; i<141; i++){
      EEPROM.writeChar(i, RandomChar());
      Serial.print("*");
    }
    EEPROM.commit();
    Serial.println();
  }
  // read
  for(int i=41; i<141; i++){
    key[i-41] = EEPROM.readChar(i);
  }

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

  if(EEPROM.read(164)==0xff){
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



  // RPM
  MinRpmPulse = EEPROM.readLong(169);
  if(MinRpmPulse<=0){
    MinRpmPulse=987654321;
  }

  // 173
  if(EEPROM.readByte(173)!=255){
    MinRpmPulseTimestamp = EEPROM.readString(173);
  }

  // 199   - APRS ON/OFF;
  if(EEPROM.read(199)<2){
    AprsON=EEPROM.read(199);
  }

  // 200-203 - APRS server IP
  // 204-207 - APRS server port
  for(int i=0; i<4; i++){
    aprs_server_ip[i]=EEPROM.readByte(i+200);
  }
  AprsPort = EEPROM.readInt(204);

  // 208-212 - APRS password
  for (int i=208; i<213; i++){
    if(EEPROM.read(i)!=0xff){
      AprsPassword=AprsPassword+char(EEPROM.read(i));
    }
  }

  // 213-230 - APRS coordinate
  // for (int i=213; i<231; i++){
  //   if(EEPROM.read(i)!=0xff){
  //     AprsCoordinates=AprsCoordinates+char(EEPROM.read(i));
  //   }
  // }
  //
  // // 231-234 Altitude
  // if(EEPROM.readByte(231)!=255){
  //   Altitude = EEPROM.readInt(231);
  // }

  // 235-238 AlertLimit
  if(EEPROM.readByte(235)!=255){
    SpeedAlertLimit_ms = EEPROM.readInt(235);
  }

  #if defined(DS18B20)
    // 239   - ExtTemp ON/OFF;
    if(EEPROM.read(239)<2){
      ExtTemp=EEPROM.read(239);
    }
  #endif

  // 240-243 WindDirShift
  if(EEPROM.readByte(240)!=255){
    WindDirShift = EEPROM.readInt(240);
  }

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
    server.begin();
    UdpCommand.begin(IncomingSwitchUdpPort);    // incoming udp port
    // chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
    //   unsigned long long1 = (unsigned long)((chipid & 0xFFFF0000) >> 16 );
    //   unsigned long long2 = (unsigned long)((chipid & 0x0000FFFF));
    //   ChipidHex = String(long1, HEX) + String(long2, HEX); // six octets
    //   YOUR_CALL=ChipidHex;

  #if defined(EnableOTA)
    // Port defaults to 3232
    // ArduinoOTA.setPort(3232);
    // Hostname defaults to esp3232-[MAC]

    // String StringHostname = "WX-station-"+String(NET_ID, HEX);
    String StringHostname = "ROT-"+String(YOUR_CALL);
    char copy[13];
    StringHostname.toCharArray(copy, 13);

    ArduinoOTA.setHostname(copy);
    ArduinoOTA.setPassword("remoteqth");
    // $ echo password | md5sum
    // ArduinoOTA.setPasswordHash("5587ba7a03b12a409ee5830cea97e079");
    ArduinoOTA
      .onStart([]() {
        esp_task_wdt_reset();
        WdtTimer=millis();

        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
      });

    ArduinoOTA.begin();
  #endif
  #if defined(OTAWEB)
    OTAserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "PSE QSY to /update");
    });
    AsyncElegantOTA.begin(&OTAserver);    // Start ElegantOTA
    OTAserver.begin();
  #endif

  #if defined(Ser2net)
    Serial_one.begin(SERIAL1_BAUDRATE, SERIAL_8N1, RX1, TX1);
  // Serial2.begin(9600);
  SerialServer.begin(SerialServerIPport);
  SerialServer.setNoDelay(true);
  #endif

  TelnetServer.begin(TelnetServerIPport);
  // TelnetlServer.setNoDelay(true);


  #if HWREV==8
    int intBuf = analogRead(RainPin);
    if(intBuf<1000){
      RainStatus=false;
    }else if(intBuf>=1000 && intBuf<=2000){
      RainStatus=true;
    }
  #endif
  #if HWREV==7
    if(digitalRead(Rain1Pin)==0 && digitalRead(Rain2Pin)==1){
      RainStatus=false;
    }else if(digitalRead(Rain1Pin)==1 && digitalRead(Rain2Pin)==0){
      RainStatus=true;
    }
  #endif

  #if defined(RF69_EXTERNAL_SENSOR)
   // clock, miso,mosi, ss
    SPI.begin(14, 15, 2, 0);
    //while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer

    Serial.println();
    Serial.print("RFM69 radio init ");
    if (!rf69.init()) {
      Serial.println("failed!");
      RF69enable = false;
      // while (1);
    }else{
      Serial.println("OK");
      RF69enable = true;

      // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
      // No encryption
      Serial.print("RFM69 setFrequency ");
      if (!rf69.setFrequency(RF69_FREQ)) {
        Serial.println("failed!");
      }else{
        Serial.println("OK");
        // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
        // ishighpowermodule flag set like this:
        rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW

        // The encryption key has to be the same as the one in the server
        uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
          0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
        };
        rf69.setEncryptionKey(key);
        Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");
      }
    }

  #endif
  //------------------------------------------------

  // digitalWrite(EnablePin,0);

  // WDT
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch
  WdtTimer=millis();

  //init and get the time
   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
   RainCountDayOfMonth=UtcTime(2);

   // ajax
   ajaxserver.on("/",HTTP_POST, handlePostRot);
   // ajaxserver.on("/STOP",HTTP_POST, handlePostStop);
   ajaxserver.on("/", handleRoot);      //This is display page
   ajaxserver.on("/readADC", handleADC);//To get update of ADC Value only
   ajaxserver.on("/readAZ", handleAZ);
   ajaxserver.on("/readFrontAZ", handleFrontAZ);
   ajaxserver.on("/readAZadc", handleAZadc);
   ajaxserver.on("/readStat", handleStat);
   ajaxserver.on("/readStart", handleStart);
   ajaxserver.on("/readMax", handleMax);
   ajaxserver.on("/readAnt", handleAnt);
   ajaxserver.on("/readAntName", handleAntName);
   ajaxserver.on("/readMapUrl", handleMapUrl);
   ajaxserver.on("/set", handleSet);
   ajaxserver.on("/cal", handleCal);
   ajaxserver.on("/readEndstop", handleEndstop);
   ajaxserver.on("/readEndstopLowZone", handleEndstopLowZone);
   ajaxserver.on("/readEndstopHighZone", handleEndstopHighZone);
   ajaxserver.on("/readCwraw", handleCwraw);
   ajaxserver.on("/readCcwraw", handleCcwraw);
   ajaxserver.on("/readMAC", handleMAC);
   ajaxserver.on("/readUptime", handleUptime);
   // ajaxserver.on("/cal/readAZ", handleAZ);
   ajaxserver.begin();                  //Start server
   Serial.println("HTTP ajax server started");

}

//-------------------------------------------------------------------------------------------------------

void loop() {
  http();
  Mqtt();
  // CLI();
  CLI2();
  Telnet();
  RunByStatus();
  Watchdog();

  ajaxserver.handleClient();

  #if defined(EnableOTA)
   ArduinoOTA.handle();
  #endif

  #if defined(OTAWEB)
   // OTAserver.on("/printIp", HTTP_GET, [](AsyncWebServerRequest *request){
   //     request->send(200, "text/plain", "ok");
   //     Serial.println(request->client()->remoteIP());
   // });
   AsyncElegantOTA.loop();
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
    Azimuth=map(AzimuthValue, CcwRaw, CwRaw, 0, MaxRotateDegree);
    ADCTimer=millis();
    AZmaster=map(AZmasterValue, 142, 3150, 0, 360);
    if(AZmaster<=180){
      AZmaster=AZmaster+180;
    }else{
      AZmaster=AZmaster-180;
    }
  }

  // AZ master potentiometer - NOT TESTED
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
    AZmasterChangeTimer=millis();
    // MqttPubString("AZmasterStart", String(AZmaster), false);
  }
  if( Status==0 && abs(AZmaster-AZmasterTmp)>3 && Run==true){
    AZmasterChangeTimer=millis();
    AZmasterTmp=AZmaster;
    // MqttPubString("AZmasterRun", String(AZmaster), false);
  }
  if( Status==0 && millis()-AZmasterChangeTimer >2000 && abs(AZmaster-AZmasterTmp)<=3 && Run==true){
    AZmasterTmp=AZmaster;
    AzimuthTarget = AZmaster - StartAzimuth;
    if(AzimuthTarget<0){
        AzimuthTarget = 360+AzimuthTarget;
    }
    RotCalculate();
    Run = false;
    MqttPubString("AzimuthTargetPot", String(AzimuthTarget), false);
    // MqttPubString("AZmasterStop", String(AZmaster), false);
  }

  // KEY
  if(AZsource==false){ // potentiometer
    static bool RunByKey = false;
    if(CwCcwInputValue==1 && Status>=0){
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
      if(Status==-2){
        Status=-3;
        RunByKey=false;
      }else if(Status==2){
        Status=3;
        RunByKey=false;
      }
    }
  }else{
    // pulse
    // PulsePerDegree
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
      case -3: {StatusStr = "PwmDwn-CCW"; break; }
      case -2: {StatusStr = "CCW"; break; }
      case -1: {StatusStr = "PwmUp-CCW"; break; }
      case  0: {StatusStr = "STOP"; break; }
      case  1: {StatusStr = "PwmUp-CW"; break; }
      case  2: {StatusStr = "CW"; break; }
      case  3: {StatusStr = "PwmDwn-CW"; break; }
    }
    MqttPubString("StatusHuman", StatusStr, false);
    // MqttPubString("Status", String(Status+4), false);
    StatusTmp=Status;
    LedStatus();
  }

  // info if change AZ and voltage
  if( (Status==0 && millis()-AZchangeTimer >2000) || (Status!=0 && millis()-AZchangeTimer >100) ){
    if(AzimuthTmp!=Azimuth){
      MqttPubString("Azimuth", String(Azimuth), false);
      AzimuthTmp=Azimuth;
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

  if(Status!=0){
    // status watchdog timeout
    if(millis()-StatusWatchdogTimer > 120000){ // after 90sec
      if(Status<0){
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
        if(Status<0){
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
    if(Status==2 || Status==-2){
        if(VoltageValue < 11.0){
          if(Status<0){
            Status=-3;
          }else{
            Status=3;
          }
          MqttPubString("Debug", "Stopped by under voltage 11V POE", false);
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

  if(RebootWatchdog > 0 && millis()-WatchdogTimer > RebootWatchdog*60000){
    Prn(3, 1,"** Activate reboot watchdog - IP switch will be restarted **");
    // EEPROM.writeByte(34, ShiftOutByte[0]);
    // EEPROM.writeByte(35, ShiftOutByte[1]);
    // EEPROM.writeByte(36, ShiftOutByte[2]);
    // EEPROM.commit();
    delay(1000);
    TelnetServerClients[0].stop();
    ESP.restart();
  }

  if(OutputWatchdog > 0 && millis()-WatchdogTimer > OutputWatchdog*60000 && OutputWatchdog < 123456){
    Prn(3, 1,"** Activate clear output watchdog **");
    // ShiftOutByte[0]=0x00;
    // ShiftOutByte[1]=0x00;
    // ShiftOutByte[2]=0x00;
    // EEPROM.writeByte(34, ShiftOutByte[0]);
    // EEPROM.writeByte(35, ShiftOutByte[1]);
    // EEPROM.writeByte(36, ShiftOutByte[2]);
    // EEPROM.commit();
    #if defined(ShiftOut)
      // digitalWrite(ShiftOutLatchPin, LOW);  // když dáme latchPin na LOW mužeme do registru poslat data
      // shiftOut(ShiftOutDataPin, ShiftOutClockPin, LSBFIRST, 0x01);
      // shiftOut(ShiftOutDataPin, ShiftOutClockPin, LSBFIRST, ShiftOutByte[2]);
      // shiftOut(ShiftOutDataPin, ShiftOutClockPin, LSBFIRST, ShiftOutByte[1]);
      // shiftOut(ShiftOutDataPin, ShiftOutClockPin, LSBFIRST, ShiftOutByte[0]);
      // digitalWrite(ShiftOutLatchPin, HIGH);    // jakmile dáme latchPin na HIGH data se objeví na výstupu
    #endif
    // deactivate
    OutputWatchdog=123456;
  }


  if(!TelnetServerClients[0].connected() && FirstListCommands==false){
    FirstListCommands=true;
  }

}

//-------------------------------------------------------------------------------------------------------

// LED
void LedStatus(){
  switch (Status) {
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
    if(Status==-1 || Status==-11 || Status==-2){  // run status CCW
      if(CcwRaw<CwRaw){ // standard az potentiometer
        if(AzimuthValue/1000>NoEndstopLowZone){
          // run
        }else{
          Status=-3;
        }
      }else{ // reverse az potentiometer value
        if(AzimuthValue/1000<NoEndstopHighZone){
          // run
        }else{
          Status=-3;
        }
      }
    }
    if(Status==1 || Status==11 || Status==2){  // run status CW
      if(CcwRaw<CwRaw){ // standard az potentiometer
        if(AzimuthValue/1000<NoEndstopHighZone){
          // MqttPubString("Debug +", String(Endstop)+"|"+String(AzimuthValue/1000)+"<"+String(NoEndstopHighZone), false);
          // run
        }else{
          // MqttPubString("Debug +3", String(Endstop)+"|"+String(AzimuthValue/1000)+"<"+String(NoEndstopHighZone), false);
          Status=3;
        }
      }else{ // reverse az potentiometer value
        if(AzimuthValue/1000>NoEndstopLowZone){
          // run
        }else{
          Status=3;
        }
      }
    }
  }
}

//-------------------------------------------------------------------------------------------------------

void RunByStatus(){
  static long PwmTimer = 0;
  DetectEndstopZone();

  // }else if( (Azimuth>=0 && Azimuth<=450) ){
    switch (Status) {
      case -3: {
        if(ACmotor==false){
          //DC
          if(PWMenable==true){
            if(millis()-PwmTimer > PwmDwnDelay){
              if(dutyCycle!=0){
                dutyCycle--;
              }
              ledcWrite(mosfetPWMChannel, dutyCycle);
              PwmTimer=millis();
              if(dutyCycle<1){
                dutyCycle=0;
                ledcWrite(mosfetPWMChannel, 0);
                digitalWrite(BrakePin, LOW);
                delay(24);
                // ReverseProcedure(false);
                digitalWrite(ReversePin, LOW);
                Status=0;
                AzimuthTarget=-1;
              }
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
            AzimuthTarget=-1;
          }
        }else{
          //AC
          digitalWrite(ACcwPin, LOW);
          digitalWrite(ReversePin, LOW);
          delay(24);
          digitalWrite(BrakePin, LOW);
          Status=0;
        }
        ; break; }
      case -2: {
        if(PWMenable==true){
          if(abs(AzimuthTarget-Azimuth)<10){
            Status=3;
          }
        }else{
          if(abs(AzimuthTarget-Azimuth)<2){
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
            if(millis()-PwmTimer > PwmUpDelay){
              dutyCycle+=2;
              ledcWrite(mosfetPWMChannel, dutyCycle);
              PwmTimer=millis();
              if(dutyCycle>253){
                ledcWrite(mosfetPWMChannel, 255);
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
        Status=-11;
        ; break; }
      case  0: {
        LedStatusErr();
        ; break; }
      case 1: {
        ReverseProcedure(false);
        RunTimer();
       Status=11;
        ; break; }
      case  11: {
        ErrorDetect=0;
        // ReverseProcedure(false);
        if(ACmotor==false){
          //DC
          if(PWMenable==true){
            if(millis()-PwmTimer > PwmUpDelay){
              dutyCycle+=2;
              ledcWrite(mosfetPWMChannel, dutyCycle);
              PwmTimer=millis();
              if(dutyCycle>253){
                ledcWrite(mosfetPWMChannel, 255);
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
          if(abs(AzimuthTarget-Azimuth)<10){
            Status=3;
          }
        }else{
          if(abs(AzimuthTarget-Azimuth)<2){
            Status=3;
          }
        }
        ; break; }
      case  3: {
        if(ACmotor==false){
          //DC
          if(PWMenable==true){
            if(millis()-PwmTimer > PwmDwnDelay){
              if(dutyCycle!=0){
                dutyCycle--;
              }
              ledcWrite(mosfetPWMChannel, dutyCycle);
              PwmTimer=millis();
              if(dutyCycle<1){
                dutyCycle=0;
                ledcWrite(mosfetPWMChannel, 0);
                digitalWrite(BrakePin, LOW);
                delay(24);
                // ReverseProcedure(false);
                digitalWrite(ReversePin, LOW);
                Status=0;
                AzimuthTarget=-1;
              }
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
            AzimuthTarget=-1;
          }
        }else{
          //AC
          digitalWrite(ACcwPin, LOW);
          digitalWrite(ReversePin, LOW);
          delay(24);
          digitalWrite(BrakePin, LOW);
          Status=0;
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




//-------------------------------------------------------------------------------------------------------
void AzShift(int AZ){
  WindDir=AZ+WindDirShift;
  if(WindDir>360){
    WindDir=WindDir-360;
  }
}

//-------------------------------------------------------------------------------------------------------
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

//---------------------------------------------------------------------------------------------------------
byte HiByte(int ID){
  bitClear(ID, 0);  // ->
  bitClear(ID, 1);
  bitClear(ID, 2);
  bitClear(ID, 3);
  ID = ID >> 4;
  return(ID);
}

//---------------------------------------------------------------------------------------------------------
byte LowByte(int ID){
  bitClear(ID, 4);
  bitClear(ID, 5);
  bitClear(ID, 6);
  bitClear(ID, 7);  // <-
  return(ID);
}

//-------------------------------------------------------------------------------------------------------
void CLI2(){
  int OUT=2;
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    OUT = 0;
  }
  // if(TelnetServerClients[0].available()){
  //   incomingByte=TelnetServerClients[0].read();
  //   OUT = 1;
  // }
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
    Prn(OUT, 1, "http://"+String(ETH.localIP()[0])+"."+String(ETH.localIP()[1])+"."+String(ETH.localIP()[2])+"."+String(ETH.localIP()[3]) );
    // Serial.print("MAC ");
    // Serial.println(MACString);
    // Prn(OUT, 1,"");
    Prn(OUT, 1,"Supported GS-232 commands: R L A S C Mxxx O F");

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
    if(Status<0){
      Status=-3;
    }else if(Status>0){
      Status=3;
    }
    Prn(OUT, 1, "Rotation Stop" );

  // C 67 Antenna Direction Value
  }else if(incomingByte==67){
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
              InputByte[0]--;
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
//-------------------------------------------------------------------------------------------------------
void CLI(){
  int OUT=2;
  // incomingByte = 0;

  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    OUT = 0;
  }

  if(TelnetServerClients[0].connected() && OUT!=0){
    TelnetAuthorized = false;
    if(TelnetServerClients[0].remoteIP()==TelnetServerClientAuth){
        TelnetAuthorized = true;
        // timeout ... > false
    }else{
      TelnetAuth();
    }
    if(TelnetAuthorized==true){
      // incomingByte = TelnetRX();
      if(incomingByte!=0){
        OUT=1;
        ListCommands(OUT);
        if(FirstListCommands==true){
          FirstListCommands=false;
        }
      }
    }
  }else if(!TelnetServerClients[0].connected()){
    TelnetAuthStep=0;
  }

  if(OUT==1){     // telnet only
  // if(OUT>-1){  // always
    esp_task_wdt_reset();
    WdtTimer=millis();

    // ?
    if(incomingByte==63){
      ListCommands(OUT);

    // +
    }else if(incomingByte==43){
      Prn(OUT, 1,"  enter MQTT broker IP address by four number (0-255) and press [enter] after each");
      Prn(OUT, 1,"  NOTE: public remoteqth broker 54.38.157.134:1883");
      for (int i=0; i<5; i++){
        if(i==4){
          Prn(OUT, 0,"enter IP port (1-65535) and press [");
        }
        if(TelnetAuthorized==true){
          Prn(OUT, 1,"enter]");
        }else{
          Prn(OUT, 1,";]");
        }
        Enter();
        int intBuf=0;
        int mult=1;
        for (int j=InputByte[0]; j>0; j--){
          intBuf = intBuf + ((InputByte[j]-48)*mult);
          mult = mult*10;
        }
        if( (i<4 && intBuf>=0 && intBuf<=255) || (i==4 && intBuf>=1 && intBuf<=65535) ){
          if(i==4){
            EEPROM.writeUShort(165, intBuf);
          }else{
            EEPROM.writeByte(161+i, intBuf);
          }
          // Prn(OUT, 1,"EEPROMcomit");
          EEPROM.commit();
        }else{
          Prn(OUT, 1,"Out of range.");
          break;
        }
      }
      Prn(OUT, 0,"** device will be restarted **");
      delay(1000);
      TelnetServerClients[0].stop();
      ESP.restart();

    // e
    }else if(incomingByte==101){
        Prn(OUT, 1,"List EEPROM [adr>value]");
        for (int i=0; i<EEPROM_SIZE; i++){
          if( (i<41)||(i>140&&i<208)||(i>212) ){  // hiden pass
            Prn(OUT, 0, String(i));
            Prn(OUT, 0, ">" );
            Prn(OUT, 0, String(EEPROM.read(i)) );
            Prn(OUT, 0, " " );
          }
        }
        Prn(OUT, 1, "" );

    // *
    }else if(incomingByte==42){
      EnableSerialDebug++;
      if(EnableSerialDebug>2){
        EnableSerialDebug=0;
      }
      Prn(OUT, 0,"** Terminal DEBUG ");
      if(EnableSerialDebug==0){
        Prn(OUT, 1,"DISABLE **");
      }else if(EnableSerialDebug==1){
        Prn(OUT, 1,"ENABLE **");
      }else if(EnableSerialDebug==2){
        Prn(OUT, 1,"ENABLE with frenetic mode **");
      }

    // E
    }else if(incomingByte==69 && OUT==0){
      Prn(OUT, 1,"  Erase whole eeprom (also telnet key)? (y/n)");
      EnterChar(OUT);
      if(incomingByte==89 || incomingByte==121){
        Prn(OUT, 1,"  Stop erase? (y/n)");
        EnterChar(OUT);
        if(incomingByte==78 || incomingByte==110){
          for(int i=0; i<EEPROM_SIZE; i++){
            EEPROM.write(i, 0xff);
            Prn(OUT, 0,".");
          }
          EEPROM.commit();
          Prn(OUT, 1,"");
          Prn(OUT, 1,"  Eeprom erased done");
        }else{
          Prn(OUT, 1,"  Erase aborted");
        }
      }else{
        Prn(OUT, 1,"  Erase aborted");
      }

    // .
    }else if(incomingByte==46){
      Prn(OUT, 1,"Reset timer and sent measure");
      MeasureTimer[0]=millis()-MeasureTimer[1];

    // q
    }else if(incomingByte==113 && TelnetServerClients[0].connected() ){
      TelnetServerClients[0].stop();
      TelnetAuthorized=false;
      // TelnetServerClientAuth = {0,0,0,0};
      TelnetAuthStep=0;
      FirstListCommands=true;

    // Q
    }else if(incomingByte==81 && TelnetServerClients[0].connected() ){
      TelnetServerClients[0].stop();
      TelnetAuthorized=false;
      TelnetServerClientAuth = {0,0,0,0};
      Prn(OUT, 1,String(TelnetServerClientAuth));
      TelnetAuthStep=0;
      FirstListCommands=true;
      EEPROM.write(37, 0); // address, value
      EEPROM.write(38, 0); // address, value
      EEPROM.write(39, 0); // address, value
      EEPROM.write(40, 0); // address, value
      EEPROM.commit();

    // @
    }else if(incomingByte==64){
      Prn(OUT, 1,"** IP switch will be restarted **");
      if(RebootWatchdog > 0){
        Prn(OUT, 1,"   Activate reboot watchdog - store outputs to EEPROM...");
        // EEPROM.writeByte(34, ShiftOutByte[0]);
        // EEPROM.writeByte(35, ShiftOutByte[1]);
        // EEPROM.writeByte(36, ShiftOutByte[2]);
        // EEPROM.commit();
        delay(1000);
      }
      TelnetServerClients[0].stop();
      ESP.restart();

    // ~K
    // }else if(incomingByte==126){
    //   EnterChar(OUT);
    //   if(incomingByte==75){
    //     Prn(OUT, 1,"");
    //     Prn(OUT, 0," ");
    //     for(int i=0; i<10; i++){
    //       Prn(OUT, 0,"    ["+String(i*10+1)+"-"+String(i*10+10)+"]  ");
    //       if(i<9){
    //         Prn(OUT, 0," ");
    //       }
    //       for(int j=0; j<10; j++){
    //         Prn(OUT, 0, String(key[i*10+j]));
    //       }
    //       Prn(OUT, 1,"");
    //     }
    //     Prn(OUT, 1,"");
    //   }

    // L
    }else if(incomingByte==76){
      Prn(OUT, 0,"  Input new location (CALLSIGN-ssid) and press [");
      if(TelnetAuthorized==true){
        Prn(OUT, 0,"enter");
      }else{
        Prn(OUT, 0,";");
      }
      Prn(OUT, 1,"]. If blank, will be use MAC");
      Enter();
      YOUR_CALL="";
      for (int i=1; i<21; i++){
        YOUR_CALL=YOUR_CALL+char(InputByte[i]);
        if(i<InputByte[0]+1){
          EEPROM.write(140+i, InputByte[i]);
        }else{
          EEPROM.write(140+i, 0xff);
        }
      }
      EEPROM.commit();
      Prn(OUT, 1,"** Device will be restarted **");
      delay(1000);
      ESP.restart();

    // A
    }else if(incomingByte==65){
      if(AprsON==true){
        AprsON=false;
        Prn(OUT, 1,"** APRS DISABLE **");
        // EEPROM.write(199, AprsON); // address, value
        // EEPROM.commit();
      }else{
        Prn(OUT, 1,"** Do you own a valid amateur radio license? **");
        EnterChar(OUT);
        if(incomingByte==89 || incomingByte==121){
          Prn(OUT, 1,"  Do you nothing disable APRS, after the license expires?");
          EnterChar(OUT);
          if(incomingByte==78 || incomingByte==110){
            AprsON=true;
            Prn(OUT, 1,"** APRS ENABLE **");
            // EEPROM.write(199, AprsON); // address, value
            // EEPROM.commit();
          }else{
            Prn(OUT, 1,"  bye");
          }
        }else{
          Prn(OUT, 1,"  bye");
        }
      }

    // p
    }else if(incomingByte==112 && AprsON==true){
        Prn(OUT, 0,"  Input APRS i-gate password and press [");
        if(TelnetAuthorized==true){
          Prn(OUT, 1,"enter]");
        }else{
          Prn(OUT, 1,";]");
        }
        Enter();
        AprsPassword="";
        for (int i=1; i<6; i++){
          AprsPassword=AprsPassword+char(InputByte[i]);
          if(i<InputByte[0]+1){
            // EEPROM.write(207+i, InputByte[i]);
          }else{
            // EEPROM.write(207+i, 0xff);
          }
        }
        // EEPROM.commit();

      // c
      }else if(incomingByte==99 && AprsON==true){
        Prn(OUT, 1,"  ddmm.ssN/dddmm.ssE | dd-degrees | mm-minutes | ss-seconds | N-north, S-south.");
        Prn(OUT, 1,"  for example 5108.41N/01259.35E");
        Prn(OUT, 0,"  Input WX coordinates and press [");
        if(TelnetAuthorized==true){
          Prn(OUT, 1,"enter]");
        }else{
          Prn(OUT, 1,";]");
        }
        Enter();
        AprsCoordinates="";
        for (int i=1; i<19; i++){
          AprsCoordinates=AprsCoordinates+char(InputByte[i]);
          if(i<InputByte[0]+1){
            // EEPROM.write(212+i, InputByte[i]);
          }else{
            // EEPROM.write(212+i, 0xff);
          }
        }
        EEPROM.commit();

      // m
      }else if(incomingByte==109){
        Prn(OUT, 0,"write your altitude in meter, and press [");
        if(TelnetAuthorized==true){
          Prn(OUT, 1,"enter]");
        }else{
          Prn(OUT, 1,";]");
        }
        Enter();
        int intBuf=0;
        int mult=1;
        for (int j=InputByte[0]; j>0; j--){
          // detect -
          if(j==1 && InputByte[j]==45){
            intBuf = 0-intBuf;
          }else{
            intBuf = intBuf + ((InputByte[j]-48)*mult);
            mult = mult*10;
          }
        }
        if(intBuf>=0 && intBuf<=7300){
          Altitude = intBuf;
          // EEPROM.writeInt(231, Altitude); // address, value
          // EEPROM.commit();
          Prn(OUT, 1," altitude "+String(EEPROM.readInt(231))+"m has been saved");
        }else{
          Prn(OUT, 1," Out of range");
        }

      // S
      }else if(incomingByte==83){
        Prn(OUT, 0,"write wind direction shift 0-359°, and press [");
        if(TelnetAuthorized==true){
          Prn(OUT, 1,"enter]");
        }else{
          Prn(OUT, 1,";]");
        }
        Enter();
        int intBuf=0;
        int mult=1;
        for (int j=InputByte[0]; j>0; j--){
          // detect -
          if(j==1 && InputByte[j]==45){
            intBuf = 0-intBuf;
          }else{
            intBuf = intBuf + ((InputByte[j]-48)*mult);
            mult = mult*10;
          }
        }
        if(intBuf>=0 && intBuf<=359){
          WindDirShift = intBuf;
          // EEPROM.writeInt(240, WindDirShift); // address, value
          // EEPROM.commit();
          Prn(OUT, 1," shift "+String(EEPROM.readInt(240))+"° has been saved");
        }else{
          Prn(OUT, 1," Out of range");
        }

    // W
    }else if(incomingByte==87){
      Prn(OUT, 1,"** Erase WindSpeedMax memory? [y/n] **");
      EnterChar(OUT);
      if(incomingByte==89 || incomingByte==121){
        // EEPROM.writeLong(169, 987654321);
        // EEPROM.writeByte(173, 255);
        EEPROM.commit();
        MqttPubString("WindSpeedMax-mps", "", true);
        MqttPubString("WindSpeedMax-utc", "", true);
        Prn(OUT, 1,"** IP switch will be restarted **");
        delay(1000);
        TelnetServerClients[0].stop();
        ESP.restart();
      }else{
        Prn(OUT, 1,"  bye");
      }

// // a
// }else if(incomingByte==97){
//     Prn(OUT, 0,"write limit for wind speed alert 0-60 m/s, and press [");
//     if(TelnetAuthorized==true){
//       Prn(OUT, 1,"enter]");
//     }else{
//       Prn(OUT, 1,";]");
//     }
//     Enter();
//     int intBuf=0;
//     int mult=1;
//     for (int j=InputByte[0]; j>0; j--){
//       // detect -
//       if(j==1 && InputByte[j]==45){
//         intBuf = 0-intBuf;
//       }else{
//         intBuf = intBuf + ((InputByte[j]-48)*mult);
//         mult = mult*10;
//       }
//     }
//     if(intBuf>=0 && intBuf<=60){
//       SpeedAlertLimit_ms = MpsToMs(intBuf);
//       EEPROM.writeInt(235, SpeedAlertLimit_ms); // address, value
//       EEPROM.commit();
//     }else{
//       Prn(OUT, 1," Out of range");
//     }

    // CR/LF
    }else if(incomingByte==13||incomingByte==10){

    // anykey
    }else{

      if(EnableSerialDebug>0){
        Prn(OUT, 0," [");
        Prn(OUT, 0, String(incomingByte) ); //, DEC);
        Prn(OUT, 1,"] unknown command");
      }
    }
    incomingByte=0;

  }else if(OUT==0){
    // ?
    if(incomingByte==63){
      ListCommands(OUT);

    // E
    }else if(incomingByte==69){
      Prn(OUT, 1,"  Erase whole eeprom (also telnet key)? (y/n)");
      EnterChar(OUT);
      if(incomingByte==89 || incomingByte==121){
        Prn(OUT, 1,"  Stop erase? (y/n)");
        EnterChar(OUT);
        if(incomingByte==78 || incomingByte==110){
          for(int i=0; i<EEPROM_SIZE; i++){
            EEPROM.write(i, 0xff);
            Prn(OUT, 0,".");
          }
          EEPROM.commit();
          Prn(OUT, 1,"");
          Prn(OUT, 1,"  Eeprom erased done");
        }else{
          Prn(OUT, 1,"  Erase aborted");
        }
      }else{
        Prn(OUT, 1,"  Erase aborted");
      }
    }

    // e
    }else if(incomingByte==101){
        Prn(OUT, 1,"List EEPROM [adr>value]");
        for (int i=0; i<EEPROM_SIZE; i++){
          if( (i<41)||(i>140&&i<208)||(i>212) ){  // hiden pass
            Prn(OUT, 0, String(i));
            Prn(OUT, 0, ">" );
            Prn(OUT, 0, String(EEPROM.read(i)) );
            Prn(OUT, 0, " " );
          }
        }
        Prn(OUT, 1, "" );


  }

}
//-------------------------------------------------------------------------------------------------------
void Enter(){
  int OUT;
  if(TelnetAuthorized==true){
    OUT=1;
  }else{
    OUT=0;
  }

  InputByte[0]=0;
  incomingByte = 0;
  bool br=false;
  Prn(OUT, 0,"> ");

  if(OUT==0){
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
              InputByte[0]--;
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

  }else if(OUT==1){
    if (TelnetServerClients[0] && TelnetServerClients[0].connected()){

        while(br==false){
          if(TelnetServerClients[0].available()){
            incomingByte=TelnetServerClients[0].read();
            if(incomingByte==10){
              br=true;
              Prn(OUT, 1,"");
            }else{
              TelnetServerClients[0].write(incomingByte);
              if(incomingByte!=10 && incomingByte!=13){
                if(incomingByte==127){
                  InputByte[0]--;
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
  if(OUT==0){
    while (Serial.available() == 0) {
      // Wait
    }
    incomingByte = Serial.read();
  }else if(OUT==1){
    if (TelnetServerClients[0] && TelnetServerClients[0].connected()){
      while(incomingByte==0){
        if(TelnetServerClients[0].available()){
          incomingByte=TelnetServerClients[0].read();
        }
      }
      if(EnableSerialDebug>0){
        Serial.println();
        Serial.print("Telnet rx-");
        Serial.print(incomingByte, DEC);
      }
    }
  }
  Prn(OUT, 1, String(char(incomingByte)) );
}

//-------------------------------------------------------------------------------------------------------

void EnterInt(int OUT){
  incomingByte = 0;
  Prn(OUT, 0,"> ");
  if(OUT==0){
    while(!Serial.available()) {
    }
    delay(3000);
    CompareInt = Serial.parseInt();
  }else if(OUT==1){
    if (TelnetServerClients[0] && TelnetServerClients[0].connected()){
      bool br=true;
      int intField[10];
      int count=0;

      while(incomingByte==0 && br==true){
        if(TelnetServerClients[0].available()){
          incomingByte=TelnetServerClients[0].read();
          // out of 0-9
          if(incomingByte<48 || incomingByte>57){
            br=false;
            intField[count]=0;
            Prn(OUT, 1,"");
          }else{
            intField[count]=incomingByte-48;
            Prn(OUT, 0,String(intField[count]));
            count++;
            incomingByte=0;
          }
        }
      }

      count--;
      CompareInt=0;
      int i=1;
      while(count>-1){
        CompareInt=CompareInt+intField[count]*i;
        // Prn(OUT, 1, String(intField[count])+"*"+String(i)+"="+String(CompareInt) );
        i=i*10;
        count--;
      }
    }
  }
  // Prn(OUT, 1, "out"+String(CompareInt) );
}

//-------------------------------------------------------------------------------------------------------
void Prn(int OUT, int LN, String STR){
  if(OUT==3){
    if(TelnetAuthorized==true){
      OUT=1;
    }else{
      OUT=0;
    }
  }

  if(OUT==0){
    Serial.print(STR);
    if(LN==1){
      Serial.println();
    }
  }else if(OUT==1){
    size_t len = STR.length()+1;
    // uint8_t sbuf[len];
    char sbuf[len];
    STR.toCharArray(sbuf, len);
    //push data to all connected telnet clients
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      if (TelnetServerClients[i] && TelnetServerClients[i].connected()){
        TelnetServerClients[i].write(sbuf, len);
        // delay(1);
        if(LN==1){
          TelnetServerClients[i].write(13); // CR
          TelnetServerClients[i].write(10); // LF
        }
      }
    }
  }
}

//-------------------------------------------------------------------------------------------------------
void ListCommands(int OUT){
  // digitalWrite(EnablePin,1);
  if(OUT==0){
    Prn(OUT, 1,"");
    Prn(OUT, 1,"");
    Prn(OUT, 1," =============================================================");
    Prn(OUT, 1," Please copy and save the IP address, MAC and telnet acces KEY");
    Prn(OUT, 1,"");
      Prn(OUT, 1, "   "+String(ETH.localIP()[0])+"."+String(ETH.localIP()[1])+"."+String(ETH.localIP()[2])+"."+String(ETH.localIP()[3]) );
      Serial.print("   ");
      Serial.println(MACString);
    Prn(OUT, 1,"");
    Prn(OUT, 1,"   [position]    key");
    Prn(OUT, 0," ");
    for(int i=0; i<10; i++){
      Prn(OUT, 0,"    ["+String(i*10+1)+"-"+String(i*10+10)+"]  ");
      if(i<9){
        Prn(OUT, 0," ");
      }
      for(int j=0; j<10; j++){
        Prn(OUT, 0, String(key[i*10+j]));
      }
      Prn(OUT, 1,"");
    }
    Prn(OUT, 1,"");
    Prn(OUT, 1," Then disconnect the USB, and log in using telnet");
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
      Prn(OUT, 1,"  Key for telnet access:");
      Prn(OUT, 0,"    ");
      for(int i=0; i<100; i++){
        Prn(OUT, 0, String(key[i]));
        if((i+1)%10==0){
          Prn(OUT, 0," ");
        }
      }
      Prn(OUT, 1,"");
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

    if(RebootWatchdog > 0){
      Prn(OUT, 0,"> Reboot countdown in ");
      Prn(OUT, 0, String(RebootWatchdog-((millis()-WatchdogTimer)/60000)) );
      Prn(OUT, 1, " minutes");
    }
    if(OutputWatchdog > 0 && OutputWatchdog<123456){
      Prn(OUT, 0,"> Clear output countdown in ");
      Prn(OUT, 0, String(OutputWatchdog-((millis()-WatchdogTimer)/60000)) );
      Prn(OUT, 1," minutes");
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
    Prn(OUT, 1,"-----------------  Sensors  -----------------");
    #if HWREV==8
      int intBuf = analogRead(RainPin);
      Prn(OUT, 0,"  RainPin raw "+String(analogRead(intBuf)));
      if(intBuf<1000){
        Prn(OUT, 0," > false");
      }else if(intBuf>=1000 && intBuf<=2000){
        Prn(OUT, 0," > true");
      }else if(intBuf>2000){
        Prn(OUT, 0," > malformed");
      }
      Prn(OUT, 1,"|"+RainCountDayOfMonth+"th day Rain counter "+String(RainCount)+"|"+String(RainPulseToMM(RainCount))+"mm|"+String(mmInPulse)+" rain mm per pulse" );
    #endif

    #if HWREV==7
      Prn(OUT, 0,"  Rain1Pin ");
      Prn(OUT, 0,String(digitalRead(Rain1Pin)));
      Prn(OUT, 0,"|Rain2Pin ");
      Prn(OUT, 0,String(digitalRead(Rain2Pin)));
      // Prn(OUT, 0," | ButtonPin ");
      //   Prn(OUT, 1, String(digitalRead(ButtonPin)));
      if(digitalRead(Rain1Pin)==digitalRead(Rain2Pin)){
        Prn(OUT, 1,"|sensor malformed!" );
      }else{
        Prn(OUT, 1,"|"+RainCountDayOfMonth+"th day Rain counter "+String(RainCount)+"|"+String(RainPulseToMM(RainCount))+"mm|"+String(mmInPulse)+" rain mm per pulse" );
      }
    #endif
    // if(EnableSerialDebug>0){
    // }else{
    // }
    // Azimuth();

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

    Prn(OUT, 0,"      S  Wind direction shift [");
    Prn(OUT, 1, String(WindDirShift)+"°]");

    // #if defined(DS18B20)
    //   // Prn(OUT, 1,"      s  scan DS18B20 1wire temperature sensor");
    //   Prn(OUT, 0,"      s  external temperature Sensor DS18B20 [O");
    //     if(ExtTemp==true){
    //       Prn(OUT, 1,"N]");
    //     }else{
    //       Prn(OUT, 1,"FF] CONNECT SENSOR (used inaccurate internal)");
    //     }
    // #endif
    // Prn(OUT, 0,"      a  speed Alert [");
    // Prn(OUT, 1, String(PulseToMetterBySecond(SpeedAlertLimit_ms))+" m/s] - not implemented");
    if(TxUdpBuffer[2]!='n'){
      Prn(OUT, 0,"      w  inactivity reboot watchdog ");
      if(RebootWatchdog>0){
        Prn(OUT, 0,"after [");
        Prn(OUT, 0, String(RebootWatchdog) );
        Prn(OUT, 1,"] minutes");
      }else{
        Prn(OUT, 1,"[disable]");
      }
      Prn(OUT, 0,"      W  inactivity clear output watchdog ");
      if(OutputWatchdog>0){
        Prn(OUT, 0,"after [");
        Prn(OUT, 0, String(OutputWatchdog) );
        Prn(OUT, 1,"] minutes");
      }else{
        Prn(OUT, 1,"[disable]");
      }
      Prn(OUT, 0,"      <  change Switch incoming UDP port [");
      Prn(OUT, 0, String(IncomingSwitchUdpPort) );
      Prn(OUT, 0,"]");
      if(IncomingSwitchUdpPort!=88){
        Prn(OUT, 0,"<-- WARNING! default is 88");
      }
      Prn(OUT, 1,"");
    }
    if(TxUdpBuffer[2] == 'm'){
      Prn(OUT, 0,"      /  set encoder range - now [");
      Prn(OUT, 0, String(NumberOfEncoderOutputs+1) );
      if(NumberOfEncoderOutputs>7){
        Prn(OUT, 1,"] (two bank)");
      }else{
        Prn(OUT, 1,"]");
      }
      Prn(OUT, 0,"      %  group buttons (select one from group) [");
      if(EnableGroupButton==true){
        Prn(OUT, 1,"ON]");
        Prn(OUT, 1,"         !  SET group buttons");
        Prn(OUT, 1,"         :  list group buttons");
      }else{
        Prn(OUT, 1,"OFF]");
      }
    }
    Prn(OUT, 0,"      *  terminal debug ");
      if(EnableSerialDebug==0){
        Prn(OUT, 1,"[OFF]");
      }else if(EnableSerialDebug==1){
        Prn(OUT, 1,"[ON]");
      }else if(EnableSerialDebug==2){
        Prn(OUT, 1,"[ON-frenetic]");
      }
    if(TxUdpBuffer[2]!='n'){
      Prn(OUT, 0,"      #  network ID prefix [");
      byte ID = 0;
      bitClear(ID, 0); // ->
      bitClear(ID, 1);
      bitClear(ID, 2);
      bitClear(ID, 3);
      ID = ID >> 4;
      if(EnableGroupPrefix==true && TxUdpBuffer[2]=='m'){
        Prn(OUT, 0,"x");
      }else{
        Prn(OUT, 0, String(ID, HEX) );
      }
      Prn(OUT, 0,"] hex");
      if(TxUdpBuffer[2]=='m' && EnableGroupPrefix==true){
        Prn(OUT, 0," (set only on controllers)");
      }
      Prn(OUT, 1,"");

      // if(HW_BCD_SW==false){
        ID = 0;
        bitClear(ID, 4);
        bitClear(ID, 5);
        bitClear(ID, 6);
        bitClear(ID, 7); // <-
        Prn(OUT, 0,"         +network ID sufix [");
        Prn(OUT, 0, String(ID, HEX) );
        Prn(OUT, 0,"] hex");
        if(TxUdpBuffer[2]=='m' && EnableGroupPrefix==true){
          Prn(OUT, 0," (multi control group - same at all)");
        }
        Prn(OUT, 1,"");
      // }

      if(TxUdpBuffer[2]=='m'){
        Prn(OUT, 0,"      $  group network ID prefix (multi control) [");
        if(EnableGroupPrefix==true){
          Prn(OUT, 1,"ON]");
        }else{
          Prn(OUT, 1,"OFF]");
        }
      }
      if(TxUdpBuffer[2]=='m' && EnableGroupPrefix==true){
        Prn(OUT, 1,"      .  list detected IP switch (multi control)");
      }
    }
    #if defined(Ser2net)
      Prn(OUT, 0,"      (  change serial1 baudrate [");
      Prn(OUT, 0, String(SERIAL1_BAUDRATE) );
      Prn(OUT, 1,"]");
      Prn(OUT, 0,"      )  change ser2net IP port [");
      Prn(OUT, 0, String(SerialServerIPport) );
      Prn(OUT, 1,"]");
    #endif

    Prn(OUT, 1, "      +  change MQTT broker IP | "+String(mqtt_server_ip[0])+"."+String(mqtt_server_ip[1])+"."+String(mqtt_server_ip[2])+"."+String(mqtt_server_ip[3])+":"+String(MQTT_PORT));
    // Prn(OUT, 0, String(mqtt_server_ip));
    // Prn(OUT, 0, ":");
    // Prn(OUT, 1, String(MQTT_PORT));
    Prn(OUT, 1,"      x  TX repeat time ["+String(MeasureTimer[1]/60000)+" min]");
    Prn(OUT, 0,"      A  APRS [");
      if(AprsON==true){
        Prn(OUT, 1,"ON]");
      }else{
        Prn(OUT, 1,"OFF]");
      }
      if(AprsON==true){
        // Prn(OUT, 0,"         i  change APRS server ip:port | ");
        // Prn(OUT, 1,String(aprs_server_ip[0])+"."+String(aprs_server_ip[1])+"."+String(aprs_server_ip[2])+"."+String(aprs_server_ip[3])+":"+String(AprsPort));
        Prn(OUT, 0,"         p  change APRS password | ");
        for (int i=208; i<213; i++){
          if(EEPROM.read(i)!=0xff){
            Prn(OUT, 0,"*");
          }
        }
        Prn(OUT, 1,"");
        Prn(OUT, 0,"         c  change APRS coordinate | ");
        Prn(OUT, 1,AprsCoordinates);
      }
    Prn(OUT, 0,"      L  change ");
      if(AprsON==true){
        Prn(OUT, 1,"CALLSIGN with ssid 6-4 | "+YOUR_CALL);
      }else{
        Prn(OUT, 1,"location | "+YOUR_CALL);
      }
    if(TxUdpBuffer[2]!='n'){
      Prn(OUT, 1,"      &  send broadcast packet");
    }
    if(TelnetServerClients[0].connected()){
      Prn(OUT, 0,"      q  disconnect and close telnet [verified IP ");
      Prn(OUT, 0, String(TelnetServerClientAuth[0])+"."+String(TelnetServerClientAuth[1])+"."+String(TelnetServerClientAuth[2])+"."+String(TelnetServerClientAuth[3]) );
      Prn(OUT, 1,"]");
      Prn(OUT, 1,"      Q  logout with erase your verified IP from memory and close telnet");
    }else{
      Prn(OUT, 1,"      E  erase whole eeprom (telnet key also)");
      // Prn(OUT, 1,"      C  eeprom commit");
      Prn(OUT, 1,"      /  list directory");
      Prn(OUT, 1,"      R  read log file");
    }
    Prn(OUT, 1,"      e  list EEPROM");
    #if !defined(BMP280) && !defined(HTU21D)
      Prn(OUT, 1,"      2  I2C scanner");
    #endif
    Prn(OUT, 1,"      .  reset timer and send measure");
    Prn(OUT, 1,"      W  erase wind speed max memory");
    Prn(OUT, 1,"      @  restart device");
    // Prn(OUT, 1,"---------------------------------------------");
    Prn(OUT, 0, " > " );
  }
  // digitalWrite(EnablePin,0);
}

//-------------------------------------------------------------------------------------------------------
float RainPulseToMM(int PULSE){
  if(PULSE>20000 || PULSE==0){
    return 0;
  }else{
    float mm;
    mm = PULSE*mmInPulse;
    return mm;
  }
}

//-------------------------------------------------------------------------------------------------------
char RandomChar(){
    int R = random(48, 122);
    if(R>=58 && 64>=R){
      R=R-random(7, 10);
    }
    if(R>=91 && 96>=R){
      R=R+random(6, 26);
    }
    return char(R);
}

// http://www.catonmat.net/blog/low-level-bit-hacks-you-absolutely-must-know/
//-------------------------------------------------------------------------------------------------------

byte IdPrefix(byte ID){
  bitClear(ID, 0);  // ->
  bitClear(ID, 1);
  bitClear(ID, 2);
  bitClear(ID, 3);
  ID = ID >> 4;
  return(ID);
}

//---------------------------------------------------------------------------------------------------------

byte IdSufix(byte ID){
  bitClear(ID, 4);
  bitClear(ID, 5);
  bitClear(ID, 6);
  bitClear(ID, 7);  // <-
  return(ID);
}

//-------------------------------------------------------------------------------------------------------
byte AsciiToHex(int ASCII){
  if(ASCII>=48 && ASCII<=57){
    return(ASCII-48);
  }else if(ASCII>=97 && ASCII<=102){
    return(ASCII-87);
  }else{
    return(255);
  }
}
//-------------------------------------------------------------------------------------------------------

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

//-------------------------------------------------------------------------------------------------------

unsigned char hexToDecBy4bit(unsigned char hex)
// convert a character representation of a hexidecimal digit into the actual hexidecimal value
{
  if(hex > 0x39) hex -= 7; // adjust for hex letters upper or lower case
  return(hex & 0xf);
}

//-------------------------------------------------------------------------------------------------------

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
          webClient.println(F("          <meta name=\"theme-color\" content=\"#ffffff\">"));
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
          #if defined(OTAWEB)
            webClient.print(F("&nbsp;| <a href=\"http://"));
            webClient.println(ETH.localIP());
            webClient.print(F(":82/update\" target=_blank>Upload&nbsp;FW</a>&nbsp;| <a href=\"https://github.com/ok1hra/IP-rotator/releases\" target=_blank>Releases</a><br><a href=\"http://"));
            webClient.println(ETH.localIP());
            webClient.print(F(":88\" onclick=\"window.open( this.href, this.href, 'width=620,height=710,left=0,top=0,menubar=no,location=no,status=no' ); return false;\"><button style='color: #fff; background-color: #060; padding: 5px 20px 5px 20px; margin:15px; border: none; -webkit-border-radius: 5px; -moz-border-radius: 5px; border-radius: 5px;} :hover {background-color: orange;} '>Azimuth Map Control</button></a>"));
          #endif
          // END STATUS
          webClient.println(F("              </p>"));
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
      Serial.println("ETH  Connected");
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
        if (MQTT_ENABLE == true && MQTT_LOGIN == true){
          // if (mqttClient.connect("esp32gwClient", MQTT_USER, MQTT_PASS)){
          //   AfterMQTTconnect();
          // }
        }else if(MQTT_ENABLE == true){
          Serial.print("EthEvent-mqtt ");

          mqttClient.setServer(mqtt_server_ip, MQTT_PORT);
          mqttClient.setCallback(MqttRx);
          lastMqttReconnectAttempt = 0;

          // EEPROM YOUR_CALL
          if(EEPROM.read(141)==0xff){
            YOUR_CALL=MACString;
            YOUR_CALL.remove(0, 12);
          }else{
            for (int i=141; i<161; i++){
              if(EEPROM.read(i)!=0xff){
                YOUR_CALL=YOUR_CALL+char(EEPROM.read(i));
              }
            }
          }

          char charbuf[50];
           // // memcpy( charbuf, ETH.macAddress(), 6);
           // ETH.macAddress().toCharArray(charbuf, 18);
           // // charbuf[6] = 0;
          if (mqttClient.connect(MACchar)){
            // Serial.println(charbuf);
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
  if (mqttClient.connect(MACchar)) {
    if(EnableSerialDebug>0){
      Prn(3, 0, "mqttReconnect-connected");
    }
    // IPAddress IPlocalAddr = ETH.localIP();                           // get
    // String IPlocalAddrString = String(IPlocalAddr[0]) + "." + String(IPlocalAddr[1]) + "." + String(IPlocalAddr[2]) + "." + String(IPlocalAddr[3]);   // to string
    // MqttPubStringQC(1, "IP", IPlocalAddrString, true);

    // resubscribe

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

  }
  return mqttClient.connected();
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

    CheckTopicBase = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/stop";
    if ( CheckTopicBase.equals( String(topic) )){
      if(EnableSerialDebug>0){
        Prn(3, 1, "/stop ");
      }
      if(Status<0){
        Status=-3;
      }else if(Status>0){
        Status=3;
      }
    }

    CheckTopicBase = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/get";
    if ( CheckTopicBase.equals( String(topic) )){
      MqttPubString("Azimuth", String(Azimuth), false);
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
    if (mqttClient.connect(MACchar)) {
      String topic = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/"+TOPIC;
      topic.toCharArray( mqttPath, 50 );
      DATA.toCharArray( mqttTX, 50 );
      mqttClient.publish(mqttPath, mqttTX, RETAIN);
    }
  }
}
//-----------------------------------------------------------------------------------

void testClient(const char * host, uint16_t port)
{
  Serial.print("\nETH  connecting to ");
  Serial.println(host);
  WiFiClient webClient;
  if (!webClient.connect(host, port)) {
    Serial.println("ETH  connection failed");
    return;
  }
  webClient.printf("GET / HTTP/1.1\r\nHost: %s\r\n\r\n", host);
  while (webClient.connected() && !webClient.available());
  while (webClient.available()) {
    Serial.write(webClient.read());
  }
  Serial.println("ETH closing connection\n");
  webClient.stop();
}

//-------------------------------------------------------------------------------------------------------
void TelnetAuth(){

  switch (TelnetAuthStep) {
    case 0: {
      if(TelnetLoginFails>=3 && millis()-TelnetLoginFailsBanTimer[0]<TelnetLoginFailsBanTimer[1]){
        Prn(1, 1,"");
        Prn(1, 0,"   Ten minutes login ban, PSE QRX ");
        Prn(1, 0,String((TelnetLoginFailsBanTimer[1]-millis()-TelnetLoginFailsBanTimer[0])/1000));
        Prn(1, 1," seconds");
        delay(3000);
        TelnetServerClients[0].stop();
        break;
      }else if(TelnetLoginFails>2 && millis()-TelnetLoginFailsBanTimer[0]>TelnetLoginFailsBanTimer[1]){
        TelnetLoginFails=0;
      }
      if(TelnetLoginFails<=3){
        Prn(1, 1,"Login? [y/n] ");
        TelnetAuthStep++;
        incomingByte=0;
      }
      break; }
    case 1: {
      // incomingByte=TelnetRX();
      if(incomingByte==121 || incomingByte==89){
        Prn(1, 1,String(char(incomingByte)));
        TelnetAuthStep++;
      }else if(incomingByte!=121 && incomingByte!=0){
        // TelnetServerClients[0].stop();
        TelnetAuthorized=false;
        TelnetAuthStep=0;
        // TelnetServerClientAuth = {0,0,0,0};
      }
      break; }
    case 2: {
      AuthQ(1, 0);
      TelnetAuthStepFails=0;
      break; }
    case 3: {
      if(incomingByte==key[RandomNumber]){
        Prn(1, 1, String(char(incomingByte)) );
        AuthQ(2, 0);
      }else if(incomingByte!=0 && incomingByte!=key[RandomNumber]){
        Prn(1, 1, String(char(incomingByte)) );
        AuthQ(2, 1);
      }
      break; }
    case 4: {
      if(incomingByte==key[RandomNumber]){
        Prn(1, 1, String(char(incomingByte)) );
        AuthQ(3, 0);
      }else if(incomingByte!=0 && incomingByte!=key[RandomNumber]){
        Prn(1, 1, String(char(incomingByte)) );
        AuthQ(3, 1);
      }
      break; }
    case 5: {
      if(incomingByte==key[RandomNumber]){
        Prn(1, 1, String(char(incomingByte)) );
        AuthQ(4, 0);
      }else if(incomingByte!=0 && incomingByte!=key[RandomNumber]){
        Prn(1, 1, String(char(incomingByte)) );
        AuthQ(4, 1);
      }
      break; }
    case 6: {
      if(incomingByte==key[RandomNumber]){
        Prn(1, 1, String(char(incomingByte)) );
        TelnetAuthStep++;
        incomingByte=0;
      }else if(incomingByte!=0 && incomingByte!=key[RandomNumber]){
        Prn(1, 1, String(char(incomingByte)) );
        TelnetAuthStep++;
        incomingByte=0;
        TelnetAuthStepFails++;
      }
      break; }
    case 7: {
      if(TelnetAuthStepFails==0){
        TelnetAuthorized = true;
        TelnetServerClientAuth = TelnetServerClients[0].remoteIP();
        Prn(1, 1,"Login OK");
        ListCommands(1);
        TelnetAuthStep++;
        incomingByte=0;
      }else{
        TelnetAuthorized = false;
        TelnetServerClientAuth = {0,0,0,0};
        Prn(1, 1,"Access denied");
        TelnetAuthStep=0;
        incomingByte=0;
        TelnetLoginFails++;
        TelnetLoginFailsBanTimer[0]=millis();
      }
      EEPROM.write(37, TelnetServerClientAuth[0]); // address, value
      EEPROM.write(38, TelnetServerClientAuth[1]); // address, value
      EEPROM.write(39, TelnetServerClientAuth[2]); // address, value
      EEPROM.write(40, TelnetServerClientAuth[3]); // address, value
      EEPROM.commit();
      break; }
  }
}

//-------------------------------------------------------------------------------------------------------

void AuthQ(int NR, bool BAD){
  Prn(1, 0,"What character is at ");
  RandomNumber=random(0, strlen(key));
  Prn(1, 0, String(RandomNumber+1) );
  Prn(1, 0," position, in key? (");
  Prn(1, 0,String(NR));
  Prn(1, 1,"/4)");
  // Prn(1, 1, String(key[RandomNumber]) );
  TelnetAuthStep++;
  incomingByte=0;
  if(BAD==true){
    TelnetAuthStepFails++;
  }
}

//-------------------------------------------------------------------------------------------------------
void Telnet(){
  uint8_t i;
  // if (wifiMulti.run() == WL_CONNECTED) {
  if (eth_connected==true) {

    //check if there are any new clients
    if (TelnetServer.hasClient()){
      for(i = 0; i < MAX_SRV_CLIENTS; i++){
        //find free/disconnected spot
        if (!TelnetServerClients[i] || !TelnetServerClients[i].connected()){
          if(TelnetServerClients[i]) TelnetServerClients[i].stop();
          TelnetServerClients[i] = TelnetServer.available();
          if (!TelnetServerClients[i]) Serial.println("Telnet available broken");
          if(EnableSerialDebug>0){
            Serial.println();
            Serial.print("New Telnet client: ");
            Serial.print(i); Serial.print(' ');
            Serial.println(TelnetServerClients[i].remoteIP());
          }
          break;
        }
      }
      if (i >= MAX_SRV_CLIENTS) {
        //no free/disconnected spot so reject
        TelnetServer.available().stop();
      }
    }

    //check clients for data
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      if (TelnetServerClients[i] && TelnetServerClients[i].connected()){
        if(TelnetServerClients[i].available()){
          //get data from the telnet client and push it to the UART
          // while(TelnetServerClients[i].available()) Serial_one.write(TelnetServerClients[i].read());
          if(EnableSerialDebug>0){
            Serial.println();
            Serial.print("TelnetRX ");
          }

          while(TelnetServerClients[i].available()){
            incomingByte=TelnetServerClients[i].read();
            // Serial_one.write(RX);
            if(EnableSerialDebug>0){
              // Serial.write(RX);
              Serial.print(char(incomingByte));
            }
          }
        }
      }else{
        if (TelnetServerClients[i]) {
          TelnetServerClients[i].stop();
          TelnetAuthorized=false;
          FirstListCommands=true;
          // TelnetServerClientAuth = {0,0,0,0};
        }
      }
    }

    //check UART for data
    // if(Serial_one.available()){
    //   size_t len = Serial_one.available();
    //   uint8_t sbuf[len];
    //   Serial_one.readBytes(sbuf, len);
    //   //push UART data to all connected telnet clients
    //   for(i = 0; i < MAX_SRV_CLIENTS; i++){
    //     if (TelnetServerClients[i] && TelnetServerClients[i].connected()){
    //       TelnetServerClients[i].write(sbuf, len);
    //       // delay(1);
    //       if(EnableSerialDebug>0){
    //         Serial.println();
    //         Serial.print("Telnet tx-");
    //         Serial.write(sbuf, len);
    //       }
    //     }
    //   }
    // }

  }else{
    // if(EnableSerialDebug>0){
    //   Serial.println("Telnet not connected!");
    // }
    for(i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (TelnetServerClients[i]) TelnetServerClients[i].stop();
    }
    delay(1000);
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

String Timestamp(){
  //time_t now;
  //time(&now);
  struct timeval tv;
  gettimeofday(&tv, NULL);
  char timestamp[40];
  // sprintf(timestamp,"%u.%06u",tv.tv_sec, tv.tv_usec);
  sprintf(timestamp,"%ld.%06ld",tv.tv_sec, tv.tv_usec);
  return String(timestamp);
}

//-------------------------------------------------------------------------------------------------------

// ajax rx
void handlePostRot() {
 // String s = MAIN_page; //Read HTML contents
 String str = ajaxserver.arg("ROT");
 if(Status==0){
   AzimuthTarget = str.toInt() - StartAzimuth;
   if(AzimuthTarget<0){
       AzimuthTarget = 360+AzimuthTarget;
   }
   MqttPubString("AzimuthTarget", String(AzimuthTarget), false);
   RotCalculate();
 }else{
   if(Status<0){
     Status=-3;
   }else{
     Status=3;
   }
 }
}

void handleSet() {

  String yourcallERR= "";
  String rotidERR= "";
  String rotnameERR= "";
  String startazimuthERR= "";
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
  String baudSELECT0= "";
  String baudSELECT1= "";
  String baudSELECT2= "";
  String baudSELECT3= "";
  String baudSELECT4= "";
  String twowireSELECT0= "";
  String twowireSELECT1= "";
  String preampSELECT0= "";
  String preampSELECT1= "";

  if ( ajaxserver.hasArg("yourcall") == false \
    && ajaxserver.hasArg("rotid") == false \
    && ajaxserver.hasArg("rotname") == false \
    && ajaxserver.hasArg("startazimuth") == false \
    && ajaxserver.hasArg("maxrotatedegree") == false \
    && ajaxserver.hasArg("mapurl") == false \
    && ajaxserver.hasArg("antradiationangle") == false \
    && ajaxserver.hasArg("edstoplowzone") == false \
    && ajaxserver.hasArg("edstophighzone") == false \
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
        char char_array[str_len];
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
        char char_array[str_len];
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
        char char_array[str_len];
        str.toCharArray(char_array, str_len+1);
        for (int i=0; i<19; i++){
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

    // StartAzimuth
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

    // MaxRotateDegree
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
        char char_array[str_len];
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

    // 223 AZsource
    if(ajaxserver.arg("source").toInt()==0 && AZsource==true){
      AZsource = false;
      EEPROM.writeBool(223, 0);
      MqttPubString("AZsource", "Potentiometer", true);
    }else if(ajaxserver.arg("source").toInt()==1 && AZsource==false){
      AZsource = true;
      EEPROM.writeBool(223, 1);
      MqttPubString("AZsource", "CW/CCW pulse", true);
      if(Endstop == false){
        Endstop = true;
        EEPROM.writeBool(29, Endstop);
        MqttPubString("EndstopUse", String(Endstop), true);
      }
    }

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

    // 29  - Endstop
    if(ajaxserver.arg("edstops").toInt()==1 && Endstop==false){
      Endstop = true;
      EEPROM.writeBool(29, Endstop);
      // EEPROM.commit();
      MqttPubString("EndstopUse", String(Endstop), true);
    }else if(ajaxserver.arg("edstops").toInt()!=1 && Endstop==true){
      if(AZsource == true){ //pulse
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


    EEPROM.commit();
  } // else form valid

if(AZsource==true){
  sourceSELECT0= "";
  sourceSELECT1= " selected";
  pulseperdegreeDisable="";
  pulseperdegreeSTYLE="";
  twowireSTYLE=" style='text-decoration: line-through; color: #555;'";
  twowireDisable=" disabled";
}else{
  sourceSELECT0= " selected";
  sourceSELECT1= "";
  pulseperdegreeDisable=" disabled";
  pulseperdegreeSTYLE=" style='text-decoration: line-through; color: #555;'";
  twowireDisable="";
  twowireSTYLE="";
}

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
}else{
  motorSELECT0= " selected";
  motorSELECT1= "";
  pwmenableSTYLE="";
  pwmenableDisable="";
}

if(PWMenable==true){
  pwmSELECT0= "";
  pwmSELECT1= " selected";
}else{
  pwmSELECT0= " selected";
  pwmSELECT1= "";
}

  String HtmlSrc = "<!DOCTYPE html><html><head><title>SETUP</title>\n";
  HtmlSrc +="<meta http-equiv='Content-Type' content='text/html; charset=UTF-8'>\n";
  // <meta http-equiv = 'refresh' content = '600; url = /'>\n";
  HtmlSrc +="<style type='text/css'> button#go {background-color: #ccc; padding: 5px 20px 5px 20px; border: none; -webkit-border-radius: 5px; -moz-border-radius: 5px; border-radius: 5px;} button#go:hover {background-color: orange;} table, th, td {color: #fff; border-collapse: collapse; border:0px } .tdr {color: #0c0; height: 40px; text-align: right; vertical-align: middle; padding-right: 15px} html,body {background-color: #333; text-color: #ccc; font-family: 'Roboto Condensed',sans-serif,Arial,Tahoma,Verdana;} a:hover {color: #fff;} a { color: #ccc; text-decoration: underline;} ";
  HtmlSrc +=".b {border-top: 1px dotted #666;} .tooltip-text {visibility: hidden; position: absolute; z-index: 1; width: 300px; color: white; font-size: 12px; background-color: #DE3163; border-radius: 10px; padding: 10px 15px 10px 15px; } .hover-text:hover .tooltip-text { visibility: visible; } #right { top: -30px; left: 200%; } #top { top: -60px; left: -150%; } #left { top: -8px; right: 120%;}";
  HtmlSrc +=".hover-text {position: relative; background: #888; padding: 5px 12px; margin: 5px; font-size: 15px; border-radius: 100%; color: #FFF; display: inline-block; text-align: center; }</style>\n";
  HtmlSrc +="<link href='http://fonts.googleapis.com/css?family=Roboto+Condensed:300italic,400italic,700italic,400,700,300&subset=latin-ext' rel='stylesheet' type='text/css'></head><body>\n";
  HtmlSrc +="<H1 style='color: #666; text-align: center;'>Setup<br><span style='font-size: 50%;'>(MAC ";
  HtmlSrc +=MACString;
  HtmlSrc +="|FW ";
  HtmlSrc +=REV;
  HtmlSrc +="|HW ";
  HtmlSrc +=String(HardwareRev);
  HtmlSrc +=")</span><span style='color: #333;'>";
  HtmlSrc +=String(HWidValue);
  HtmlSrc +="</span></H1><div style='display: flex; justify-content: center;'><table><form action='/set' method='post' style='color: #ccc; margin: 50 0 0 0; text-align: center;'>\n";
  HtmlSrc +="<tr class='b'><td class='tdr'><label for='yourcall'>Your callsign:</label></td><td><input type='text' id='yourcall' name='yourcall' size='10' value='";
  HtmlSrc += YOUR_CALL;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += yourcallERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 200px;'>Used as part of an MQTT topic</span></td></tr>\n<tr><td class='tdr'><label for='rotid'>Rotator ID:</label></td><td><input type='text' id='rotid' name='rotid' size='2' value='";
  HtmlSrc += NET_ID;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += rotidERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 300px;'>1-2 chars<br>Multiple rotators with the same TOPIC must have different IDs<br>Second part of MQTT topic</span></span></td></tr>\n<tr><td class='tdr'><label for='rotname'>Rotator name:</label></td><td><input type='text' id='rotname' name='rotname' size='20' value='";
  HtmlSrc += RotName;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += rotnameERR;
  HtmlSrc +="</span></td></tr>\n<tr><td class='tdr'><label for='startazimuth'>Start CCW azimuth:</label></td><td><input type='text' id='startazimuth' name='startazimuth' size='3' value='";
  HtmlSrc += StartAzimuth;
  HtmlSrc +="'>&deg; <span style='color:red;'>";
  HtmlSrc += startazimuthERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 100px;'>Allowed range<br>0-359&deg;</span></span></td></tr>\n<tr><td class='tdr'><label for='maxrotatedegree'>Rotation range in degrees:</label></td><td><input type='text' id='maxrotatedegree' name='maxrotatedegree' size='3' value='";
  HtmlSrc += MaxRotateDegree;
  HtmlSrc +="'>&deg; <span style='color:red;'>";
  HtmlSrc += maxrotatedegreeERR;
  HtmlSrc +="</span></td></tr>\n<tr><td class='tdr'><label for='mapurl'>Background azimuth map URL:</label></td><td><input type='text' id='mapurl' name='mapurl' size='30' value='";
  HtmlSrc += MapUrl;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += mapurlERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='left'>DXCC generated every quarter hour is available at https://remoteqth.com/xplanet/. If you need another, please contact OK1HRA.</span></span> <a href='https://remoteqth.com/xplanet/' target='_blank'>Available list</a></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='antradiationangle'>Antenna radiation angle in degrees:</label></td><td><input type='text' id='antradiationangle' name='antradiationangle' size='3' value='";
  HtmlSrc += AntRadiationAngle;
  HtmlSrc +="'>&deg; <span style='color:red;'>";
  HtmlSrc += antradiationangleERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 100px;'>Allowed range<br>1-180&deg;</span></span></td></tr>\n";

  HtmlSrc +="<tr class='b'><td class='tdr'><label for='source'>Azimuth source:</label></td><td><select name='source' id='source'><option value='0'";
  HtmlSrc += sourceSELECT0;
  HtmlSrc +=">Potentiometer</option><option value='1'";
  HtmlSrc += sourceSELECT1;
  HtmlSrc +=">CW/CCW pulse</option></select><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 250px;'>Pulse deactivate control with KEY, and SW endstop</span></span></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='pulseperdegree'><span";
  HtmlSrc += pulseperdegreeSTYLE;
  HtmlSrc +=">Pulse count per degree:</span></label></td><td><input type='text' id='pulseperdegree' name='pulseperdegree' size='3' value='";
  HtmlSrc += PulsePerDegree;
  HtmlSrc +="'";
  HtmlSrc += pulseperdegreeDisable;
  HtmlSrc +="><span style='color:red;'>";
  HtmlSrc += pulseperdegreeERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 100px;'>Allowed range<br>1-100</span></span></td></tr>\n";

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
  HtmlSrc +=">ON</option></select><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 200px;'>For potentiometer use one turn from any<br>Need manualy preset with two trimmer</span></span></td></tr>\n";

  // if(AZsource==false){ // potentiometer
    HtmlSrc +="<tr class='b'><td class='tdr'><label for='edstops'><span";
    HtmlSrc += edstopsSTYLE;
    HtmlSrc +=">Endstops INSTALLED:</span></label></td><td><input type='checkbox' id='edstops' name='edstops' value='1' ${postData.edstops?'checked':''} ";
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
      HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 100px;'>Allowed range<br>2-15 tenths of a Volt</span></span></td></tr>\n";

      HtmlSrc +="<tr><td class='tdr'><label for='edstophighzone'><span";
      HtmlSrc += edstophighzoneSTYLE;
      HtmlSrc +=">CW forbidden zone<br>(software endstops):</span></label></td><td><input type='text' id='edstophighzone' name='edstophighzone' size='3' value='";
      HtmlSrc += int(NoEndstopHighZone*10);
      HtmlSrc +="'";
      HtmlSrc += edstophighzoneDisable;
      HtmlSrc +="> tenths of a Volt <span style='color:red;'>";
      HtmlSrc += edstophighzoneERR;
      HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 100px;'>Allowed range<br>16-31 tenths of a Volt</span></span></td></tr>\n";
  // }

  HtmlSrc +="<tr class='b'><td class='tdr'><label for='oneturnlimitsec'>Watchdog speed:</label></td><td><input type='text' id='oneturnlimitsec' name='oneturnlimitsec' size='3' value='";
  HtmlSrc += OneTurnLimitSec;
  HtmlSrc +="'> seconds per one turn <span style='color:red;'>";
  HtmlSrc += oneturnlimitsecERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='left' style='width: 300px;'>Allowed range 20-600sec<br>Lower speed limit activating the watchdog<br>Use a number 50% higher than the actual speed of your rotator</span></span></td></tr>\n";

  HtmlSrc +="<tr><td class='tdr'><label for='acmotor'>Motor supply:</label></td><td><select name='motor' id='motor'><option value='0'";
  HtmlSrc += motorSELECT0;
  HtmlSrc +=">DC</option><option value='1'";
  HtmlSrc += motorSELECT1;
  HtmlSrc +=">AC</option></select><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 150px;'>DC use PWM<br>AC activate the other two relays</span></span></td></tr>\n";

  HtmlSrc +="<tr><td class='tdr'><label for='pwmenable'><span";
  HtmlSrc += pwmenableSTYLE;
  HtmlSrc += ">DC PWM control:</label></td><td><select name='pwmenable' id='pwmenable' ";
  HtmlSrc += pwmenableDisable;
  HtmlSrc +="><option value='0'";
  HtmlSrc += pwmSELECT0;
  HtmlSrc +=">OFF</option><option value='1'";
  HtmlSrc += pwmSELECT1;
  HtmlSrc +=">ON</option></select><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 150px;'>If disable, mosfet will be only on/off operation</span></span></td></tr>\n";

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
  HtmlSrc +=">115200</option></select><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 150px;'>Use for GS-232 protocol<br>Restart after change recommended</span></span></td></tr>\n";

  HtmlSrc +="<tr class='b'><td class='tdr'><label for='mqttip0'>MQTT broker IP:</label></td><td>";
  HtmlSrc +="<input type='text' id='mqttip0' name='mqttip0' size='1' value='" + String(mqtt_server_ip[0]) + "'>&nbsp;.&nbsp;<input type='text' id='mqttip1' name='mqttip1' size='1' value='" + String(mqtt_server_ip[1]) + "'>&nbsp;.&nbsp;<input type='text' id='mqttip2' name='mqttip2' size='1' value='" + String(mqtt_server_ip[2]) + "'>&nbsp;.&nbsp;<input type='text' id='mqttip3' name='mqttip3' size='1' value='" + String(mqtt_server_ip[3]) + "'>";
  HtmlSrc +="<span style='color:red;'>";
  HtmlSrc += mqttERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 250px;'>Default public broker 54.38.157.134<br>If first digit zero, MQTT disable</span></span></td></tr>\n";

  HtmlSrc +="<tr><td class='tdr'><label for='mqttport'>MQTT broker PORT:</label></td><td>";
  HtmlSrc +="<input type='text' id='mqttport' name='mqttport' size='2' value='" + String(MQTT_PORT) + "'>\n";
  HtmlSrc +="<span style='color:red;'>";
  HtmlSrc += mqttportERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 150px;'>Default public broker port 1883</span></span></td></tr>\n";

  HtmlSrc +="<tr class='b'><td class='tdr'></td><td><button id='go'>&#10004; Change</button></form>&nbsp; ";
  HtmlSrc +="<a href='/cal' onclick=\"window.open( this.href, this.href, 'width=700,height=1100,left=0,top=0,menubar=no,location=no,status=no' ); return false;\"><button id='go'>Calibrate &#8618;</button></a>";
  HtmlSrc +="</td></tr>\n";

  // HtmlSrc +="<tr><td class='tdr'></td><td style='height: 42px;'></td></tr>\n";
  // HtmlSrc +="<tr><td class='tdr'></td><td style='height: 42px;'></td></tr>";
  // HtmlSrc +="<tr><td class='tdr'><a href='/'><button id='go'>&#8617; Back to Control</button></a></td><td class='tdl'><a href='/cal' onclick=\"window.open( this.href, this.href, 'width=700,height=715,left=0,top=0,menubar=no,location=no,status=no' ); return false;\"><button id='go'>Calibrate &#8618;</button></a></td></tr>";
  HtmlSrc +="<tr><td class='tdr'></td><td class='tdl'><span style='color: #666;'>After change, refresh all other page for apply changes.</span><br><a href='https://remoteqth.com/w/doku.php?id=simple_rotator_interface_v' target='_blank'>More on Wiki &#10138;</a></td></tr>\n";
  HtmlSrc +="</body></html>\n";

  ajaxserver.send(200, "text/html", HtmlSrc); //Send web page
}

void handleCal() {

  if ( ajaxserver.hasArg("stop")==1 ){
    if(Status<0){
      Status=-3;
    }else if(Status>0){
      Status=3;
    }
  }

  if ( ajaxserver.hasArg("cw")==1 ){
    Status=1; //digitalWrite(BrakePin, HIGH); delay(24);
    // RunTimer();
  }

  if ( ajaxserver.hasArg("ccw")==1 ){
    Status=-1; //digitalWrite(BrakePin, HIGH); delay(24);
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
    CcwRaw=142;
    CwRaw = 3155;
    EEPROM.writeUShort(31, CcwRaw);
    EEPROM.writeUShort(33, CwRaw);
    EEPROM.commit();
    MqttPubString("CcwRaw", String(CcwRaw), true);
    MqttPubString("CwRaw", String(CwRaw), true);
  }

  long RawTmp = 0;

  // 31-32 CcwRaw
  if ( ajaxserver.hasArg("setccw")==1 ){
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

  // 33-34  CwRaw
  if ( ajaxserver.hasArg("setcw")==1 ){
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
  HtmlSrc +="<style type='text/css'>button {background-color: #ccc; padding: 5px 20px 5px 20px; border: none; -webkit-border-radius: 5px; -moz-border-radius: 5px; border-radius: 5px;} button:hover {background-color: orange;} ";
  HtmlSrc +=".red {background-color: #c00; color: #FFF;} table, th, td { color: #fff; border: 0px; border-color: #666; border-style: solid; margin: 0px;}";
  HtmlSrc +=".tdl { text-align: left; padding: 10px;}";
  HtmlSrc +=".tdc { text-align: center; padding: 10px;}";
  HtmlSrc +=".tdr { text-align: right; padding: 10px;}";
  HtmlSrc +="html,body { background-color: #333; text-color: #ccc; font-family: 'Roboto Condensed',sans-serif,Arial,Tahoma,Verdana;}";
  HtmlSrc +="a:hover {color: #fff;}";
  HtmlSrc +="a {color: #ccc; text-decoration: underline;}";
  HtmlSrc +="</style><link href='http://fonts.googleapis.com/css?family=Roboto+Condensed:300italic,400italic,700italic,400,700,300&subset=latin-ext' rel='stylesheet' type='text/css'></head><body>";
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
  HtmlSrc +="<tr><td class='tdc' colspan='3' style='background-color: #666; border-top-left-radius: 20px; border-top-right-radius: 20px;'><span style='font-size: 200%;'>1. Rotate direction calibrate</span></td></tr>";
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

  HtmlSrc +="<tr><td class='tdc' colspan='3' style='background-color: #666; border-top-left-radius: 20px; border-top-right-radius: 20px;'><span style='font-size: 200%;'>2. Azimuth calibrate</span></td>";
  HtmlSrc +="</tr><tr>";
  HtmlSrc +="<td class='tdc' colspan='3' style='background-color: #666;'><div style='position: relative;'><canvas class='top' id='Azimuth' width='600' height='140'>Your browser does not support the HTML5 canvas tag.</canvas></div></td>";
  HtmlSrc +="</tr><tr style='background-color: #666;'>";
  HtmlSrc +="<td class='tdl'><button id='setccw' name='setccw'>&#8676; SAVE CCW</button></td>";
  HtmlSrc +="<td class='tdc' style='background-color: #666;'><button id='clear' name='clear'>";
  HtmlSrc +="RESET CW/CCW SAVE</button></td>";
  HtmlSrc +="<td class='tdr'><button id='setcw' name='setcw'>SAVE CW &#8677;</button></td>";
  HtmlSrc +="</tr><tr>";
  HtmlSrc +="<td class='tdc' colspan='3' style='background-color: #666;'><button id='reverseaz' name='reverseaz'";
  HtmlSrc +=ReverseAzCOLOR;
  HtmlSrc +=">REVERSE-AZIMUTH-<strong>";
  HtmlSrc +=ReverseAzSTATUS;
  HtmlSrc +="</strong></button></td>";
  HtmlSrc +="</tr><tr>";
  HtmlSrc +="<td class='tdc' colspan='3' style='color: #333; background-color: #666; border-bottom-left-radius: 20px; border-bottom-right-radius: 20px;'>";
  if( AZsource == false && AZtwoWire == true && CwRaw < 1577 ){
    HtmlSrc +="<span style='color: #ccc;'>Recommendation: </span><span style='color: #0c0;'>If you are using a 2 wire potentiometer less than 500Ω,<br>you can increase the sensitivity if you short the J16 jumper on the back side PCB.<br><br></span>";
  }
  HtmlSrc +="<span style='color: #ccc;'>Instruction:</span><br>&#8226; If azimuth potentiometer shows opposite values, activate reverse button <br>&#8226; Rotate to both CCW ";
  HtmlSrc +=StartAzimuth;
  HtmlSrc +="&deg; and CW ";
  HtmlSrc +=StartAzimuth+MaxRotateDegree;
  HtmlSrc +="&deg; ends and save new limits<br>&#8226; After calibrate rotate to full CCW limits, measure real azimuth<br>and put this value to &ldquo;Start CCW azimuth:&rdquo;	field in Setup page</td>";
  HtmlSrc +="</tr><tr>";
  HtmlSrc +="<td class='tdc' colspan='3' style='height:30px'></td></tr>";

  HtmlSrc +="<tr><td class='tdc' colspan='3' style='background-color: #666; border-top-left-radius: 20px; border-top-right-radius: 20px;'><span style='font-size: 200%;'>3. Front panel calibrate (optional)</span></td>";
  HtmlSrc +="</tr><tr>";
  HtmlSrc +="<td class='tdc' colspan='3' style='color: #333; background-color: #666; border-bottom-left-radius: 20px; border-bottom-right-radius: 20px;'>";
  HtmlSrc +="<span style='font-size: 150%;'>Panel value <span style='font-weight: bold; color: #0a0;' id='frontAZValue'>0</span><br></span>";
  HtmlSrc +="<span style='color: #ccc;'><br>Instruction:</span><br>&#8226; Rotate front panel potentiometer axis without knob to value 0&deg <br>&#8226; Put knob with orientation to north on axis<br>&#8226; Fixate knob to axis on position north</td></tr>";

  HtmlSrc +="</table></div><div style='display: flex; justify-content: center;'><span><p style='text-align: center;'><a href='https://remoteqth.com/w/doku.php?id=simple_rotator_interface_v' target='_blank'>More on Wiki &#10138;</a></p></span></div>";

  String s = CAL_page; //Read HTML contents
  HtmlSrc +=s;
  ajaxserver.send(200, "text/html", HtmlSrc); //Send web page
}

void handlePostStop() {
  if(Status<0){
    Status=-3;
  }else if(Status>0){
    Status=3;
  }
}
void handleRoot() {
 String s = MAIN_page; //Read HTML contents
 ajaxserver.send(200, "text/html", s); //Send web page
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
  ajaxserver.send(200, "text/plane", String(AzimuthValue) );
}
void handleStat() {
  ajaxserver.send(200, "text/plane", String(Status+4) );
}
void handleStart() {
  ajaxserver.send(200, "text/plane", String(StartAzimuth) );
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
void handleEndstop() {
  ajaxserver.send(200, "text/plane", String(Endstop) );
}
void handleEndstopLowZone() {
  ajaxserver.send(200, "text/plane", String(NoEndstopLowZone) );
}
void handleEndstopHighZone() {
  ajaxserver.send(200, "text/plane", String(NoEndstopHighZone) );
}
void handleCwraw() {
  ajaxserver.send(200, "text/plane", String(CwRaw) );
}
void handleCcwraw() {
  ajaxserver.send(200, "text/plane", String(CcwRaw) );
}
void handleMAC() {
  ajaxserver.send(200, "text/plane", String(MACString) );
}
void handleUptime() {
  ajaxserver.send(200, "text/plane", String(millis()/1000) );
}
