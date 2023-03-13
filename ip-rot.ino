/*

python /home/dan/Arduino/hardware/espressif/esp32/tools/esptool/esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x10000 /home/dan/Arduino/hra/ok1hra/esp32/wx/20210407-wx.ino.esp32-poe.bin

3D printed IP rotator
----------------------
https://remoteqth.com/w/doku.php?id=3d_print_wx_station
TNX OK1IAK for code help

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

MQTT monitor
------------
mosquitto_sub -v -h 192.168.1.200 -t 'OK1HRA/ROT/#'
mosquitto_sub -v -h 54.38.157.134 -t 'BD:2F/ROT/#'

MQTT topic
------------
mosquitto_pub -h 192.168.1.200 -t OK1HRA/ROT/Target -m '10'
mosquitto_pub -h 54.38.157.134 -t BD:2F/ROT/Target -m '10'


HARDWARE ESP32-POE

Changelog:
20221104

ToDo
+ implementovat jmeno rotatoru
+ watchdog na cas a minimalni pohyb spravnym smerem
+ v ochranych zonach zobrazovat azimut sedou barvou
+ hlidat min napeti (11V) pod kterym prestat otacet + warning do /status
+ v html zobrazit target azimuth
- if mqtt_server_ip[0]=0 then disable MQTT
- html ne jako smerova mapa, ale linearni stupnice
- status endstop true/false - aktivuje ochranne zony na krajich potenciometru ()
- automaticka detekce smeru otaceni pri MAXpwm = automatikcky reverse rezim
- kalibrace promene OneTurnCalibrateRaw
- implementovat ID (vice rotatoru)
- web form https://github.com/mobizt/ESPForm
- do debugu vypisovat prumer casu behu hlavni smycky v ms
- nastaveni MAX PWM
- vycistit kod


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
// not impemented
bool Reverse =  false;

const char* REV = "20230312";


// need set from GUI
String YOUR_CALL = "";
String NET_ID = "";
String RotName = "";
int StartAzimuth = 0;         // max CCW limit callibrate in real using
unsigned int AntRadiationAngle = 10;
String MapUrl = "" ;
                //$ /usr/bin/xplanet -window -config ./geoconfig -longitude 13.8 -latitude 50.0 -geometry 600x600 -projection azimuthal -num_times 1 -output ./map.png
unsigned int MaxRotateDegree = 0;
bool Endstop =  false;
bool ACmotor =  false;




unsigned int PwmUpDelay  = 4;  // [ms]*255
unsigned int PwmDwnDelay = 3;  // [ms]*255
long StatusWatchdogTimer = 0;
long RotateWatchdogTimer = 0;
int AzimuthWatchdog = 0;

// #define WWWtwo

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
#define ETH_POWER 12 //0                // #define ETH_PHY_POWER 0 ./Arduino/hardware/espressif/esp32/variants/esp32-poe/pins_arduino.h
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
const float FunelDiaInCM = 10.0; // cm funnel diameter
//-------------------------------------------------------------------------------------------------------

#include "esp_adc_cal.h"
const int AzimuthPin    = 39;  // analog
float AzimuthValue        = 0.0;
int Azimuth              = 0;
int AzimuthTarget        = 0;
int Status               = 0; // -3 PwmDwnCCW|-2 CCW|-1 PwmUpCCW|0 off|1 PwmUpCW|2 CW|3 PwmDwnCW
const int CwCcwButtPin  = 36;  // analog
int CwCcwButtValue      = 0;
const int HWidPin       = 34;  // analog
float HWidValue           = 0.0;
const int VoltagePin    = 35;  // analog
float VoltageValue        = 0.0;

const int ReversePin    = 16;  //
const int PwmPin        = 4;   //
// setting PWM properties
const int PwmFreq = 1000;
const int ledChannel = 0;
const int PwmResolution = 8;

// unsigned long TimerTemp;

// interrupts
#include "esp_attr.h"

// values
const int keyNumber = 1;
char key[100];
long MeasureTimer[2]={2800000,300000};   //  millis,timer (5 min)
long RainTimer[2]={0,5000};   //  <---------------- rain timing
int RainCount;
String RainCountDayOfMonth;
bool RainStatus;
/*
1mm rain = 15,7cm^2/10 = 1,57ml     <- by rain funnel radius
10ml = 11,5 pulses = 0,87ml/pulse   <- constanta tilting measuring cup
*/
// float mmInPulse = 0.87/(3.14*(FunelDiaInCM/2)*(FunelDiaInCM/2)/10); // calculate rain mm, in one pulse
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

const int SERIAL_BAUDRATE = 115200; // serial debug baudrate
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
52|String   |I love ESP32.
66|String   |Thank You Espressif!


0-1   - NET_ID
2-22  - RotName
23  - StartAzimuth UShort
25  - MaxRotateDegree UShort
27  - AntRadiationAngle UShort
29  - Endstop Bool
30  - ACmotor Bool

37-40 - Authorised telnet client IP
41-140 - Authorised telnet client key
141-160 - YOUR_CALL
161-164 - MQTT broker IP
165-166 - MQTT_PORT
167-168 -
169-219 - MapUrl

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
WebServer ajaxserver(HTTP_SERVER_PORT+8);

WiFiServer server(HTTP_SERVER_PORT);
#if defined(WWWtwo)
  WiFiServer server2(HTTP_SERVER_PORT+8);
#endif
bool DHCP_ENABLE = 1;
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

// WX pinout
const int ShiftInDataPin = 34;  // to rev0.3 13
const int ShiftInLatchPin = 4;
const int ShiftInClockPin = 5;
bool rxShiftInRead;

byte ShiftOutByte[3];
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
  pinMode(CwCcwButtPin, INPUT);
  pinMode(HWidPin, INPUT);
    HWidValue = readADC_Cal(analogRead(HWidPin));
  pinMode(VoltagePin, INPUT);
  pinMode(ReversePin, OUTPUT);

  ledcSetup(ledChannel, PwmFreq, PwmResolution);
  ledcAttachPin(PwmPin, ledChannel);
  // ledcWrite(ledChannel, 0);

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
  pinMode(ShiftInLatchPin, OUTPUT);
  pinMode(ShiftInClockPin, OUTPUT);
  pinMode(ShiftInDataPin, INPUT);

  Serial.begin(SERIAL_BAUDRATE);
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
    I2Cone.begin(I2C_SDA, I2C_SCL, (uint32_t)100000); // SDA pin, SCL pin, 100kHz frequency
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
    Wire.begin(I2C_SDA, I2C_SCL);
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
    Wire.begin(I2C_SDA, I2C_SCL);
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
  if(RebootWatchdog>0){
    ShiftOutByte[0]=EEPROM.readByte(34);
    ShiftOutByte[1]=EEPROM.readByte(35);
    ShiftOutByte[2]=EEPROM.readByte(36);
  }
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
  for (int i=213; i<231; i++){
    if(EEPROM.read(i)!=0xff){
      AprsCoordinates=AprsCoordinates+char(EEPROM.read(i));
    }
  }

  // 231-234 Altitude
  if(EEPROM.readByte(231)!=255){
    Altitude = EEPROM.readInt(231);
  }

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

  if(EnableSerialDebug>0){
    Serial.println();
    Serial.print("Version: ");
    Serial.println(REV);
    Serial.println("===============================");
    Serial.print("SLAVE DEVICE NET-ID: 0x");
    Serial.println(NET_ID);
    Serial.print("Listen MASTER: ");
    if(TxUdpBuffer[2] == 'o'){
      Serial.println("Open Interface III");
    }
    if(TxUdpBuffer[2] == 'r' ){
      Serial.println("Band decoder MK2");
    }
    if(TxUdpBuffer[2] == 'm' ){
      Serial.println("IP switch master");
    }
    if(TxUdpBuffer[2] == 'n' ){
      Serial.println("none");
    }
    Serial.println("===============================");
    Serial.println("  press '?' for list commands");
    Serial.println();
    Serial.println();
  }

  #if defined(WIFI)
    if(EnableSerialDebug>0){
      Serial.print("WIFI Connecting to ");
      Serial.print(ssid);
    }
    WiFi.begin(ssid, password);
    // attempt to connect to Wifi network:
    while(WiFi.status() != WL_CONNECTED) {
      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
      delay(500);
      if(EnableSerialDebug>0){
        Serial.print(".");
      }
    }
    // LED1status = !LED1status;
    // digitalWrite(LED1, LED1status);           // signalize wifi connected
    if(EnableSerialDebug>0){
      Serial.println("");
      Serial.println("WIFI connected");
      Serial.print("WIFI IP address: ");
      Serial.println(WiFi.localIP());
      Serial.print("WIFI dBm: ");
      Serial.println(WiFi.RSSI());
    }
  #endif

  #if defined(ETHERNET)
    // mqtt_server_ip=BrokerIpArray[0];
    // MQTT_PORT = MQTT_PORT_Array[0];

    WiFi.onEvent(EthEvent);
    // ETH.begin();
    ETH.begin(ETH_ADDR, ETH_POWER, ETH_MDC, ETH_MDIO, ETH_TYPE, ETH_CLK);
    if(DHCP_ENABLE==false){
      ETH.config(IPAddress(192, 168, 1, 188), IPAddress(192, 168, 1, 255),IPAddress(255, 255, 255, 0),IPAddress(8, 8, 8, 8));
      //config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = (uint32_t)0x00000000, IPAddress dns2 = (uint32_t)0x00000000);
    }
  #endif
    server.begin();
    #if defined(WWWtwo)
      server2.begin();
    #endif
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

        Interrupts(false);
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
        Interrupts(true);
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
        Interrupts(true);
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
   Interrupts(true);

   // ajax
   ajaxserver.on("/",HTTP_POST, handlePostRot);
   // ajaxserver.on("/STOP",HTTP_POST, handlePostStop);
   ajaxserver.on("/", handleRoot);      //This is display page
   ajaxserver.on("/readADC", handleADC);//To get update of ADC Value only
   ajaxserver.on("/readAZ", handleAZ);
   ajaxserver.on("/readStat", handleStat);
   ajaxserver.on("/readStart", handleStart);
   ajaxserver.on("/readMax", handleMax);
   ajaxserver.on("/readAnt", handleAnt);
   ajaxserver.on("/readAntName", handleAntName);
   ajaxserver.on("/readMapUrl", handleMapUrl);
   ajaxserver.on("/set", handleSet);
   ajaxserver.begin();                  //Start server
   Serial.println("HTTP ajax server started");

}

//-------------------------------------------------------------------------------------------------------

void loop() {
  http();
  #if defined(WWWtwo)
    http2();
  #endif
  Mqtt();
  CLI();
  Telnet();
  Watchdog();
  RunByStatus();

  ajaxserver.handleClient();

  // RX_UDP();
  // PWM demo


  // static long PwmTimer = 0;
  // static unsigned int dutyCycle = 0;
  // static unsigned int PwmDemo = 1;
  //
  // switch (PwmDemo) {
  //   case  1: {
  //     if(millis()-PwmTimer > 5){
  //       dutyCycle+=2;
  //       ledcWrite(ledChannel, dutyCycle);
  //       PwmTimer=millis();
  //       if(dutyCycle>254){
  //         PwmDemo=2;
  //       }
  //     }
  //     ; break; }
  //   case  2: {
  //     if(millis()-PwmTimer > 0){
  //       dutyCycle--;
  //       ledcWrite(ledChannel, dutyCycle);
  //       PwmTimer=millis();
  //       if(dutyCycle<1){
  //         PwmDemo=3;
  //         digitalWrite(ReversePin, HIGH); delay(12);
  //       }
  //     }
  //     ; break; }
  //     case  3: {
  //       if(millis()-PwmTimer > 5){
  //         dutyCycle+=2;
  //         ledcWrite(ledChannel, dutyCycle);
  //         PwmTimer=millis();
  //         if(dutyCycle>254){
  //           PwmDemo=4;
  //         }
  //       }
  //       ; break; }
  //     case  4: {
  //       if(millis()-PwmTimer > 0){
  //         dutyCycle--;
  //         ledcWrite(ledChannel, dutyCycle);
  //         PwmTimer=millis();
  //         if(dutyCycle<1){
  //           PwmDemo=1;
  //           digitalWrite(ReversePin, LOW); delay(12);
  //         }
  //       }
  //       ; break; }
  // }




  // for(int dutyCycle = 0; dutyCycle <= 255; dutyCycle++){
  //   ledcWrite(ledChannel, dutyCycle);
  //   delay(15);
  // }
  // // digitalWrite(ReversePin, HIGH); delay(12);
  //
  // for(int dutyCycle = 255; dutyCycle >= 0; dutyCycle--){
  //   ledcWrite(ledChannel, dutyCycle);
  //   delay(15);
  // }
  //
  // // digitalWrite(ReversePin, LOW); delay(12);


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
}
// SUBROUTINES -------------------------------------------------------------------------------------------------------

void IRAM_ATTR RPMcount(){    // must be before attachInterrupt ;)
  // Interrupts(false);
  if(digitalRead(RpmPin)==0){   // because FALLING not work
    // Serial.println("RPM");
    RpmPulse = millis()-RpmTimer[0];
    RpmTimer[0] = millis();
    // Prn(3, 0,"*");
    RpmInterrupt = true;
    // if(RpmPulse<PeriodMinRpmPulse){
    //   PeriodMinRpmPulse=RpmPulse;
    //   PeriodMinRpmPulseTimestamp=UtcTime(1);
    // }
    // if(RpmPulse<MinRpmPulse && needEEPROMcommit==false){
    //   MinRpmPulseTimestamp=PeriodMinRpmPulseTimestamp;
    //   EEPROM.writeLong(169, RpmPulse);
    //   EEPROM.writeString(173, MinRpmPulseTimestamp);
    //   MinRpmPulse=RpmPulse;
    //   needEEPROMcommit = true;
    // }
    // if(RpmPulse<RpmTimer[1]){
    //   RpmAverage[1]=RpmAverage[1]+RpmPulse;
    //   RpmAverage[0]++;
    // }

  }
  // Interrupts(true);
}
//-------------------------------------------------------------------------------------------------------
void Interrupts(boolean ON){
  if(ON==true){
    attachInterrupt(RpmPin, RPMcount, FALLING);
    // attachInterrupt(digitalPinToInterrupt(RpmPin), RPMcount, FALLING);
    // attachInterrupt(digitalPinToInterrupt(RpmPin), RPMcount, RISING);
  }else{
    detachInterrupt(digitalPinToInterrupt(RpmPin));
  }
}
//-------------------------------------------------------------------------------------------------------
uint32_t readADC_Cal(int ADC_Raw)
{
  esp_adc_cal_characteristics_t adc_chars;

  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  return(esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}
//-------------------------------------------------------------------------------------------------------
void Watchdog(){
  /* Azimuth potentiometer to antenna transfer ratio: 2x
  RAW         0-4095
  calibrate 142-3139 (dif 2997)

                       1T                   2T                  3T       Potentiometer turn
  -45°   0°       90°       180°      270°      360°     *[1]  *[2]      Azimuth
    '....|____'____|____'____|____'____|____'____|____'____|....'
    1    3                                       2         2    3        Calibrate RAW (2,775 per 1°)
    4    9                                       6         8    1
    2    2                                       0         8    3
                                                 0         9?   9
  */

// read ADC
const unsigned int CCWlimitRaw = 142+2997/12;
static unsigned int OneTurnCalibrateRaw = 2600;     // ----------------- need procedure fo callibrate
const unsigned int CWlimit = 3139-2997/12;
MaxRotateDegree = map(CWlimit, CCWlimitRaw, OneTurnCalibrateRaw, 0, 360);     // *[1]
// static unsigned int MaxRotateDegree = CalibrateRawToDeg(CWlimit);     // *[1]
// static unsigned int MaxLimitDegree = map(3139, CCWlimitRaw, OneTurnCalibrateRaw, 0, 360);     // *[2]
static unsigned int MaxLimitDegree = map(3139, 392, 2600, 0, 360);     // *[2]
// static unsigned int MaxLimitDegree = CalibrateRawToDeg(3139);
static long ADCTimer = 0;
if(millis()-ADCTimer > 100){
  AzimuthValue = readADC_Cal(analogRead(AzimuthPin));
  CwCcwButtValue = analogRead(CwCcwButtPin);
  // R divider | 12,95/2,95=4,38983050847458
  VoltageValue = readADC_Cal(analogRead(VoltagePin))/1000.0*4.39;
  // Azimuth=map(AzimuthValue, 142, 3139, -45, 495);
  Azimuth=map(AzimuthValue, CCWlimitRaw, OneTurnCalibrateRaw, 0, 360);

  // Azimuth=CalibrateRawToDeg(AzimuthValue);
  ADCTimer=millis();
}

// MQTT pub
static long MqttTimer = 0;
static int AzimuthTmp = 0;
static float VoltageValueTmp = 0;
static int StatusTmp = 0; // -3 PwmDwnCCW|-2 CCW|-1 PwmUpCCW|0 off|1 PwmUpCW|2 CW|3 PwmDwnCW
static String StatusStr = "";

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
  MqttPubString("Status", String(Status+4), false);
  StatusTmp=Status;
}

// info if change
if( (Status==0 && millis()-MqttTimer >2000) || (Status!=0 && millis()-MqttTimer >100) ){
  if(abs(AzimuthTmp-Azimuth)>2){
    MqttPubString("Azimuth", String(Azimuth), false);
    if(Status==0){
      // MqttPubString("AzimuthRAWcal", String(AzimuthValue), false);
      MqttPubString("MaxLimitDegree", String(MaxLimitDegree), false);
    }
    AzimuthTmp=Azimuth;
  }
  if(abs(VoltageValueTmp-VoltageValue)>0.5){
    MqttPubString("VoltageValue", String(VoltageValue), false);
    VoltageValueTmp=VoltageValue;
  }
  MqttTimer=millis();
}

if(Status!=0){
  // status watchdog
  if(millis()-StatusWatchdogTimer > 90000){ // after 90sec
    if(Status<0){
      Status=-3;
    }else{
      Status=3;
    }
    MqttPubString("Debug", "Status0 by time watchdog", false);
    StatusWatchdogTimer = StatusWatchdogTimer+5000; // next 5sec check
  }
  // Azimuth change watchdog
  if(millis()-RotateWatchdogTimer > 10000){    // check every 10 sec
    if(abs(AzimuthWatchdog-Azimuth)>5){
      RotateWatchdogTimer=millis();
      AzimuthWatchdog=Azimuth;
    }else{
      if(Status<0){
        Status=-3;
      }else{
        Status=3;
      }
      RotateWatchdogTimer=millis();
      MqttPubString("Debug", "Status0 by AZ watchdog", false);
    }
  }
}

static long DCunderVoltageWatchdog = 0;
if(millis()-DCunderVoltageWatchdog > 1000){
  if(Status==2 || Status==-2){
      if(VoltageValue < 11.0){
        if(Status<0){
          Status=-3;
        }else{
          Status=3;
        }
        MqttPubString("Debug", "Status0 by under voltage 11V", false);
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
    // MqttPubString("CwCcwButtValue", String(CwCcwButtValue/1000.0), false);
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

  // frenetic mode
  if(EnableSerialDebug>1 && millis()-FreneticModeTimer > 1000){
    FreneticModeTimer=millis();
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
    ShiftOutByte[0]=0x00;
    ShiftOutByte[1]=0x00;
    ShiftOutByte[2]=0x00;
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

void RotCalculate(){

  const unsigned int CCWlimitRaw = 142+2997/12;
  static unsigned int OneTurnCalibrateRaw = 2600;     // ----------------- need procedure fo callibrate
  const unsigned int CWlimit = 3139-2997/12;
  MaxRotateDegree = map(CWlimit, CCWlimitRaw, OneTurnCalibrateRaw, 0, 360);     // *[1]
  const unsigned int HalfPoint = MaxRotateDegree/2;

  // if(EnableSerialDebug>0){
    MqttPubString("MaxRotateDegree", String(MaxRotateDegree), true);
    MqttPubString("HalfPoint", String(HalfPoint), true);
// }
/*
StartAzimuth---------HalfPoint---------StartAzimuth+MaxRotateDegree
 0                     203,5           360       407
                                        0         47
*/

  // overlap
  if(Azimuth < HalfPoint && AzimuthTarget < HalfPoint){
    //CCW
  }else{
    if(AzimuthTarget<MaxRotateDegree-360){  // if in overlap
      AzimuthTarget=AzimuthTarget+360;      // go to overlap
    }
    //CW
  }

  if(AzimuthTarget>=0 && AzimuthTarget <=MaxRotateDegree){
    if(AzimuthTarget>Azimuth){
      Status=1;
      digitalWrite(ReversePin, LOW); delay(12);
      StatusWatchdogTimer = millis();
      RotateWatchdogTimer = millis();
      AzimuthWatchdog=Azimuth;
    }else{
      Status=-1;
      digitalWrite(ReversePin, HIGH); delay(12);
      StatusWatchdogTimer = millis();
      RotateWatchdogTimer = millis();
      AzimuthWatchdog=Azimuth;
    }

  // escape from the forbidden zone
  }else if(Azimuth<0 && AzimuthTarget>0){
    digitalWrite(ReversePin, LOW); delay(12);
    Status=1;
    StatusWatchdogTimer = millis();
    RotateWatchdogTimer = millis();
    AzimuthWatchdog=Azimuth;
  }else if(Azimuth>MaxRotateDegree && AzimuthTarget<MaxRotateDegree){
    digitalWrite(ReversePin, HIGH); delay(12);
    Status=-1;
    StatusWatchdogTimer = millis();
    RotateWatchdogTimer = millis();
    AzimuthWatchdog=Azimuth;
  }else{
    AzimuthTarget=-1;
  }
}

//-------------------------------------------------------------------------------------------------------

void RunByStatus(){

static long PwmTimer = 0;
static unsigned int dutyCycle = 0;

  // }else if( (Azimuth>=0 && Azimuth<=450) ){
    switch (Status) {
      case -3: {
        if(millis()-PwmTimer > PwmDwnDelay){
          dutyCycle--;
          ledcWrite(ledChannel, dutyCycle);
          PwmTimer=millis();
          if(dutyCycle<1){
            Status=0;
            digitalWrite(ReversePin, LOW); delay(12);
            AzimuthTarget=-1;
          }
        }
        ; break; }
      case -2: {
        if(abs(Azimuth-AzimuthTarget)<10){
          Status=-3;
        }

        // watchdog - pokud se nemeni azimut, tak stop !!!
        ; break; }
      case -1: {
        if(millis()-PwmTimer > PwmUpDelay){
          dutyCycle+=2;
          ledcWrite(ledChannel, dutyCycle);
          PwmTimer=millis();
          if(dutyCycle>254){
            Status=-2;
          }
        }
        ; break; }
      case  0: {
        // nill
        ; break; }
      case  1: {
        if(millis()-PwmTimer > PwmUpDelay){
          dutyCycle+=2;
          ledcWrite(ledChannel, dutyCycle);
          PwmTimer=millis();
          if(dutyCycle>254){
            Status=2;
          }
        }
        ; break; }
      case  2: {
        if(abs(AzimuthTarget-Azimuth)<10){
          Status=3;
        }

        // watchdog - pokud se nemeni azimut, tak stop !!!
        ; break; }
      case  3: {
        if(millis()-PwmTimer > PwmDwnDelay){
          dutyCycle--;
          ledcWrite(ledChannel, dutyCycle);
          PwmTimer=millis();
          if(dutyCycle<1){
            Status=0;
            AzimuthTarget=-1;
          }
        }
        ; break; }
    }
  // }


}

//----------------------------RADIO--------------------------------------------------------------------
// https://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF69.html

void check_radio() {
  #if defined(RF69_EXTERNAL_SENSOR)
    static long CheckRadioTimer;
    if ((millis() - CheckRadioTimer) > 2000 && RF69enable==true){
      Interrupts(false);
      if (rf69.available()) {
        if(EnableSerialDebug>0){
          Prn(3, 0,"RF69 ");
        }
        if(rf69.recv(buf, &len)) {
          if (!len) return;
          buf[len] = 0;
          received_data = ((char*)buf);
          if(EnableSerialDebug>0){
            Prn(3, 1,"RX: "+String(received_data) );
          }
          // Serial.print("Received:" );
          // Serial.println(received_data);
          temp_radio = getValue(received_data, ',', 0);
          humidity_radio = getValue(received_data, ',', 1);
          vbat_radio = getValue(received_data, ',', 2);
          MqttPubString("RF-Temperature-Celsius", temp_radio, false);
          MqttPubString("RF-HumidityRel-Percent", humidity_radio, false);
          MqttPubString("RF-BattVoltage", vbat_radio, false);
        }else{
          if(EnableSerialDebug>0){
            Prn(3, 1,"receive failed!");
          }
        }
      // }else{
      //   if(EnableSerialDebug>0){
      //     Prn(3, 1,"RF69 not available!");
      //   }
      }
      CheckRadioTimer=millis();
      Interrupts(true);
    }
  #endif
}
//-------------------------------------------------------------------------------------------------------

// Prepocet tlaku vzduchu z absolutni hodnoty na hladinu more (OK1IRG)
// double CMain::Babinet(double Pz, double T){
double Babinet(double Pz, double T){
  double C, P0;
  // prepocet provadime podle Babinetova vzorce - nejdriv spocti teplotni korekci
  C = 16e3 * (1 + T/273);
  // vypocti tlak na hladine more
  P0 = Pz * (C + double(Altitude)) / (C - double(Altitude));
  return(P0);
}


//-------------------------------------------------------------------------------------------------------
// ToDo
// https://techtutorialsx.com/2017/10/07/esp32-arduino-timer-interrupts/
int MpsToMs(int MPS){
  if(MPS>0){
    int ms;
    // ms = 1000*CupRotationCircleCircumferenceInMeters*Kfactor/MPS; // 0,21195 = cup rotaion circle circumference in meters (for 135 mm diameter), 1,3 = K factor
    ms = pow(MPS/500, 1/-0.784);
    return ms;
  }else{
    return 0;
    // for (int i = 0; i < MappingRow; i++) {
    //   if(mapping[i][0] >= PULSE && PULSE >= mapping[i+1][0]){   // find range
    //     return map(PULSE, mapping[i][0], mapping[i+1][0], mapping[i][1], mapping[i+1][1]);  // map(value, fromLow, fromHigh, toLow, toHigh) //*10 one decimal
    //     break;
    //   }
    // }
    // return 0;
  }
}
//-------------------------------------------------------------------------------------------------------
float PulseToMetterBySecond(long PULSE){
  // < 0,28 m/s
  if(PULSE>2000 || PULSE==0){
    return 0;
  }else{
    float mps;
    // ms = 1000/PULSE*CupRotationCircleCircumferenceInMeters*Kfactor; // 0,21195 = cup rotaion circle circumference in meters (for 135 mm diameter), 1,3 = K factor
    mps = 500*pow(PULSE, -0.784);  // https://mycurvefit.com
    return mps;

    // for (int i = 0; i < MappingRow; i++) {
    //   if(mapping[i][0] >= PULSE && PULSE >= mapping[i+1][0]){   // find range
    //     return map(PULSE, mapping[i][0], mapping[i+1][0], mapping[i][1], mapping[i+1][1]);  // map(value, fromLow, fromHigh, toLow, toHigh) //*10 one decimal
    //     break;
    //   }
    // }
    // return 0;
  }
}
//-------------------------------------------------------------------------------------------------------
// byte Azimuth(){    // run from interrupt
//   byte rxShiftInByte=0x00;
//   digitalWrite(ShiftInLatchPin,1);   //Set latch pin to 1 to get recent data into the CD4021
//   delayMicroseconds(10);
//   // delay(1);
//   digitalWrite(ShiftInLatchPin,0);     //Set latch pin to 0 to get data from the CD4021
//
//   for (int i=0; i<8; i++){                // 16 = two bank
//     digitalWrite(ShiftInClockPin, 0);
//     delayMicroseconds(10);
//     // delay(1);
//     // rxShiftInByte = rxShiftInByte  | (digitalRead(ShiftInDataPin)<<i);
//     bitWrite(rxShiftInByte, i, digitalRead(ShiftInDataPin));
//     digitalWrite(ShiftInClockPin, 1);
//     delayMicroseconds(10);
//     // delay(1);
//   }
//   switch (rxShiftInByte){
//     case 0b01111111:
//       AzShift(180); // 0
//       break;
//     case 0b01111110:
//       AzShift(202); // 22
//       break;
//     case 0b11111110:
//       AzShift(225); // 45
//       break;
//     case 0b11111100:
//       AzShift(247); // 67
//       break;
//     case 0b11111101:
//       AzShift(270); // 90
//       break;
//     case 0b11111001:
//       AzShift(292); // 112
//       break;
//     case 0b11111011:
//       AzShift(315); // 135
//       break;
//     case 0b11110011:
//       AzShift(337); // 157
//       break;
//     case 0b11110111:
//       AzShift(0); // 180
//       break;
//     case 0b11100111:
//       AzShift(22);
//       break;
//     case 0b11101111:
//       AzShift(45);
//       break;
//     case 0b11001111:
//       AzShift(67);
//       break;
//     case 0b11011111:
//       AzShift(90);
//       break;
//     case 0b10011111:
//       AzShift(112);
//       break;
//     case 0b10111111:
//       AzShift(135);
//       break;
//     case 0b00111111:
//       AzShift(157);
//       break;
//     default:
//       WindDir=-1;
//       break;
//   }
//   return rxShiftInByte;
// }

//-------------------------------------------------------------------------------------------------------
void AzShift(int AZ){
  WindDir=AZ+WindDirShift;
  if(WindDir>360){
    WindDir=WindDir-360;
  }
}
//-------------------------------------------------------------------------------------------------------
// todo
// if(BMP280enable==true){
// if(HTU21Denable==true){

void AprsWxIgate() {
  #if defined(BMP280) && defined(HTU21D)
  if(AprsON==true){
    // digitalWrite(EnablePin,1);
    // http://czech.aprs2.net:14501/
    // if (!AprsClient.connect(aprs_server_ip, AprsPort))
    if (!AprsClient.connect("czech.aprs2.net", 14580)) {  // client.connect(URL, port);  char URL[]="google.com"
      if(EnableSerialDebug>0){
        Prn(3, 1,"APRS client connect failed!");
      }
      return;
    }

    // login
    if(EnableSerialDebug>0){
      Prn(3, 1,"APRS-TX | user "+YOUR_CALL+" pass "+AprsPassword+" vers esp32wx "+REV+" filter m/1");
    }
    AprsClient.println("user "+YOUR_CALL+" pass "+AprsPassword+" vers esp32wx "+REV+" filter m/1");
    delay (250);

    // send data
    if(EnableSerialDebug>0){
      Prn(3, 1,"APRS-TX | "+YOUR_CALL+">APRSWX,TCPIP*,qAS,:="
      +AprsCoordinates+"_"
      +LeadingZero(3,WindDir)
      +"/"+LeadingZero(3,PulseToMetterBySecond(RpmAverage[1]/RpmAverage[0])/0.447)
      +"g"+LeadingZero(3,PulseToMetterBySecond(PeriodMinRpmPulse)/0.447)
      +"t"+LeadingZero(3,htu.readTemperature()*1.8+32)
      +"h"+LeadingZero(3,constrain(htu.readHumidity(), 0, 100))
      +"b"+LeadingZero(6,Babinet(double(bmp.readPressure()), double(htu.readTemperature()*1.8+32)))
      +"P"+LeadingZero(3,RainPulseToMM(RainCount)/0.254) );
    }
    String StrBuf;
    // StrBuf=LeadingZero(3,htu.readTemperature()*1.8+32);
    StrBuf=LeadingZero(3,bmp.readTemperature()*1.8+32);

    // #if defined(DS18B20)
    //   if(ExtTemp==true){
    //     sensors.requestTemperatures();
    //     // float temperatureF = sensors.getTempFByIndex(0);
    //     float temperatureF = sensors.getTempC(insideThermometer);
    //     StrBuf=LeadingZero(3,temperatureF*1.8+32);
    //   }
    // #endif
    //
    // #if defined(RF69_EXTERNAL_SENSOR)
    //   if(temp_radio.toInt()>0 && RF69enable==true){
    //     StrBuf=LeadingZero(3,temp_radio.toInt()*1.8+32);
    //   }
    // #endif

    AprsClient.println(YOUR_CALL+">APRSWX,TCPIP*,qAS,:="
    +AprsCoordinates+"_"
    +LeadingZero(3,WindDir)
    // Prn(OUT, 1, "             avg ("+String(RpmAverage[1])+"/"+String(RpmAverage[0])+") "+String(RpmAverage[1]/RpmAverage[0])+"ms | "+String(PulseToMetterBySecond(RpmAverage[1]/RpmAverage[0]))+"m/s | "+String(PulseToMetterBySecond(RpmAverage[1]/RpmAverage[0])*3.6)+" km/h");
    // Prn(OUT, 1, "             MAX in period "+String(PeriodMinRpmPulse)+"ms | "+String(PulseToMetterBySecond(PeriodMinRpmPulse))+"m/s | "+String(PulseToMetterBySecond(PeriodMinRpmPulse)*3.6)+" km/h ("+String(PeriodMinRpmPulseTimestamp)+")");
    // mps/0.447 = mph
    +"/"+LeadingZero(3,PulseToMetterBySecond(RpmAverage[1]/RpmAverage[0])/0.447)
    +"g"+LeadingZero(3,PulseToMetterBySecond(PeriodMinRpmPulse)/0.447)
    // #if defined(DS18B20)
    //   +"t"+LeadingZero(3,temperatureF)
    // #else
    //   +"t"+LeadingZero(3,htu.readTemperature()*1.8+32)
    // #endif
    +"t"+StrBuf
    +"h"+LeadingZero(3,constrain(htu.readHumidity(), 0, 100))
    +"b"+LeadingZero(6,Babinet(double(bmp.readPressure()), double(htu.readTemperature()*1.8+32)))
    +"P"+LeadingZero(3,RainPulseToMM(RainCount)/0.254) );

    if(EnableSerialDebug>0){
      Prn(3, 1,"APRS-TX | "+YOUR_CALL+">APRSWX,TCPIP*,qAC:> remoteqth.com 3D-printed WX station");
    }
    AprsClient.println(YOUR_CALL+">APRSWX,TCPIP*,qAC:> remoteqth.com 3D-printed WX station");

    // digitalWrite(EnablePin,0);
  }
  #endif
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

//-------------------------------------------------------------------------------------------------------
// void Print_SDCard_Info ()
// {
//     uint8_t cardType = SD_MMC.cardType();
//
//     if(cardType == CARD_NONE){
//         Serial.println("No SD_MMC card attached");
//         return;
//     }
//
//     Serial.print("SD_MMC Card Type: ");
//     if(cardType == CARD_MMC){
//         Serial.println("MMC");
//     } else if(cardType == CARD_SD){
//         Serial.println("SDSC");
//     } else if(cardType == CARD_SDHC){
//         Serial.println("SDHC");
//     } else {
//         Serial.println("UNKNOWN");
//     }
//
//     uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
//     Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);
//
//     listDir(SD_MMC, "/", 0);
//     createDir(SD_MMC, "/mydir");
//     listDir(SD_MMC, "/", 0);
//     removeDir(SD_MMC, "/mydir");
//     listDir(SD_MMC, "/", 2);
//     writeFile(SD_MMC, "/hello.txt", "Hello ");
//     appendFile(SD_MMC, "/hello.txt", "World!\n");
//     readFile(SD_MMC, "/hello.txt");
//     deleteFile(SD_MMC, "/foo.txt");
//     renameFile(SD_MMC, "/hello.txt", "/foo.txt");
//     readFile(SD_MMC, "/foo.txt");
//     testFileIO(SD_MMC, "/test.txt");
//     Serial.printf("Total space: %lluMB\n", SD_MMC.totalBytes() / (1024 * 1024));
//     Serial.printf("Used space: %lluMB\n", SD_MMC.usedBytes() / (1024 * 1024));
//
// }
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

    #if !defined(BMP280) && !defined(HTU21D)
      // 2
      }else if(incomingByte==50){
        I2cScanner();
    #endif

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

void EnterIntOld(int OUT){
  Prn(OUT, 0,"> ");
  if(OUT==0){
    while(!Serial.available()) {
    }
    delay(3000);
    CompareInt = Serial.parseInt();
  }else if(OUT==1){
    if (TelnetServerClients[0] && TelnetServerClients[0].connected()){
      delay(5000);
      if(TelnetServerClients[0].available()){
        // while(TelnetServerClients[0].available()){
        //   Prn(OUT, 1, "4" );
        //   // incomingByte=TelnetServerClients[0].read();
        // }
        CompareInt = TelnetServerClients[0].parseInt();
      }
    }
  }
  Prn(OUT, 1, String(CompareInt) );
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
    Prn(3, 0,"  Azimuth ");
    Prn(3, 0,String(Azimuth));
    if(WindDir==-1){
      Prn(OUT, 1,"|sensor malformed!" );
    }else{
      Prn(OUT, 1, "|"+String(WindDir)+"° [with shift "+String(WindDirShift)+"°]");
    }
    Prn(OUT, 0,"  RpmPin ");
      Prn(OUT, 0,String(digitalRead(RpmPin)));
    Prn(OUT, 1, "|Wind speed last   "+String(RpmPulse)+" ms|"+String(PulseToMetterBySecond(RpmPulse))+" m/s|"+String(PulseToMetterBySecond(RpmPulse)*3.6)+" km/h");
    Prn(OUT, 1, "             avg ("+String(RpmAverage[1])+"/"+String(RpmAverage[0])+") "+String(RpmAverage[1]/RpmAverage[0])+" ms|"+String(PulseToMetterBySecond(RpmAverage[1]/RpmAverage[0]))+" m/s|"+String(PulseToMetterBySecond(RpmAverage[1]/RpmAverage[0])*3.6)+" km/h");
    Prn(OUT, 1, "             MAX in period   "+String(PeriodMinRpmPulse)+" ms|"+String(PulseToMetterBySecond(PeriodMinRpmPulse))+" m/s|"+String(PulseToMetterBySecond(PeriodMinRpmPulse)*3.6)+" km/h("+String(PeriodMinRpmPulseTimestamp)+")");
    Prn(OUT, 1, "             lifetime MAX    "+String(MinRpmPulse)+" ms|"+String(PulseToMetterBySecond(MinRpmPulse))+" m/s|"+String(PulseToMetterBySecond(MinRpmPulse)*3.6)+" km/h("+String(MinRpmPulseTimestamp)+")");
    #if defined(HTU21D)
      if(HTU21Denable==true){
        Prn(OUT, 1, "  HTU21D Humidity relative "+String(constrain(htu.readHumidity(), 0, 100))+"% ["+String(htu.readTemperature())+"°C]");
      }else{
        Prn(OUT, 1, "  HTU21D Humidity relative n/a");
      }
    #endif
    #if defined(SHT21)
      internal.read();
      // external.read();
      Serial.print("  SHT21 Humidity relative ");
      Serial.print(internal.getTemperature(), 1);
      Serial.print("\t");
      Serial.println(internal.getHumidity(), 1);
      // Serial.print("\t");
      // Serial.print(external.getTemperature(), 1);
      // Serial.print("\t");
      // Serial.println(external.getHumidity(), 1);
    #endif
    #if defined(BMP280)
      if(BMP280enable==true){
        Prn(OUT, 1, "  BMP280 Pressure "+String(Babinet(double(bmp.readPressure()), double(htu.readTemperature()*1.8+32))/100)+" hPa ["+String(bmp.readPressure()/100)+" raw] "+String(bmp.readTemperature())+"°C");
      }else{
        Prn(OUT, 1, "  BMP280 Pressure n/a");
      }
    #endif
    #if defined(DS18B20)
      if(ExtTemp==true){
        if(OUT==0){
          Serial.flush();
          Serial.end();
        }
        sensors.requestTemperatures();
        // float temperatureC = sensors.getTempCByIndex(0);
        float temperatureC = sensors.getTempC(insideThermometer);
        if(OUT==0){
          Serial.begin(SERIAL_BAUDRATE);
          while(!Serial) {
            ; // wait for serial port to connect. Needed for native USB port only
          }
        }
        Prn(OUT, 1, "  DS18B20 Temperature "+String(temperatureC)+"°C");
      }
    #endif
    #if defined(RF69_EXTERNAL_SENSOR)
      if(RF69enable==true){
        Prn(OUT, 1, "  RF sensor: "+String(temp_radio)+"°C, "+String(humidity_radio)+"%, "+String(vbat_radio)+"V");
      }
    #endif

    // Prn(OUT, 1, "  Humidity "+String()+"%");
    // if(cardType!=CARD_NONE){
    //   uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    //   // Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);
    //   Prn(OUT, 1," | size "+String(printf("%lluMB", cardSize)));
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
/*
ID FROM TO : BROADCAST ;
ID FROM TO : CONFIRM ;
ID FROM TO : A B C ;
ID FROM TO : A B C ;

TX  0ms:b;
RX  0sm:c;
TX  0ms:123;
RX  0sm:123;
*/

void RX_UDP(){
  UDPpacketSize = UdpCommand.parsePacket();    // if there's data available, read a packet
  if (UDPpacketSize){
    UdpCommand.read(packetBuffer, 10);      // read the packet into packetBufffer
    // Print RAW
    if(EnableSerialDebug>0){
      Serial.println();
      Serial.print("RXraw [");
      Serial.print(packetBuffer[0], HEX);
      for(int i=1; i<8; i++){
        Serial.print(char(packetBuffer[i]));
      }
      Serial.print(F("] "));
      Serial.print(UdpCommand.remoteIP());
      Serial.print(":");
      Serial.print(UdpCommand.remotePort());
      Serial.println();
    }

    // ID-FROM-TO filter
    if(
      (EnableGroupPrefix==false
      && String(packetBuffer[0], DEC).toInt()==0
      && packetBuffer[1]==TxUdpBuffer[2]  // FROM Switch
      && packetBuffer[2]== 's'  // TO
      && packetBuffer[3]== B00000000
      // && packetBuffer[3]== ':'
      && packetBuffer[7]== ';')
      ||
      (EnableGroupPrefix==true
      && IdSufix(packetBuffer[0])==IdSufix(0)
      && packetBuffer[1]==TxUdpBuffer[2]  // FROM Switch
      && packetBuffer[2]== 's'  // TO
      && packetBuffer[3]== B00000000
      // && packetBuffer[3]== ':'
      && packetBuffer[7]== ';')
    ){

      if( EnableGroupPrefix==true && (
        DetectedRemoteSwPort [IdPrefix(packetBuffer[0])] == 0
        || (packetBuffer[4]== 'b' && packetBuffer[5]== 'r' && packetBuffer[6]== 'o')
        || (packetBuffer[4]== 'c' && packetBuffer[5]== 'f' && packetBuffer[6]== 'm')
        )
      ){
        IPAddress TmpAddr = UdpCommand.remoteIP();
        DetectedRemoteSw [hexToDecBy4bit(IdPrefix(packetBuffer[0]))] [0]=TmpAddr[0];     // Switch IP addres storage to array
        DetectedRemoteSw [hexToDecBy4bit(IdPrefix(packetBuffer[0]))] [1]=TmpAddr[1];
        DetectedRemoteSw [hexToDecBy4bit(IdPrefix(packetBuffer[0]))] [2]=TmpAddr[2];
        DetectedRemoteSw [hexToDecBy4bit(IdPrefix(packetBuffer[0]))] [3]=TmpAddr[3];
        DetectedRemoteSwPort [hexToDecBy4bit(IdPrefix(packetBuffer[0]))]=UdpCommand.remotePort();
        if(EnableSerialDebug>0){
          Serial.print("Detect controller ID ");
          Serial.print(IdPrefix(packetBuffer[0]), HEX);
          Serial.print(" on IP ");
          Serial.print(DetectedRemoteSw [hexToDecBy4bit(IdPrefix(packetBuffer[0]))] [0]);
          Serial.print(".");
          Serial.print(DetectedRemoteSw [hexToDecBy4bit(IdPrefix(packetBuffer[0]))] [1]);
          Serial.print(".");
          Serial.print(DetectedRemoteSw [hexToDecBy4bit(IdPrefix(packetBuffer[0]))] [2]);
          Serial.print(".");
          Serial.print(DetectedRemoteSw [hexToDecBy4bit(IdPrefix(packetBuffer[0]))] [3]);
          Serial.print(":");
          Serial.println(DetectedRemoteSwPort [hexToDecBy4bit(IdPrefix(packetBuffer[0]))] );
        }
        if(TxUdpBuffer[2] == 'm'){
          TxUDP('s', packetBuffer[2], ShiftOutByte[0], ShiftOutByte[1], ShiftOutByte[2], 1);
        }
      }

      // RX Broadcast / CFM
      if((packetBuffer[4]== 'b' && packetBuffer[5]== 'r' && packetBuffer[6]== 'o')
        || (packetBuffer[4]== 'c' && packetBuffer[5]== 'f' && packetBuffer[6]== 'm')
        ){
        if(EnableSerialDebug>0){
          Serial.print("RX [");
          Serial.print(packetBuffer[0], HEX);
          for(int i=1; i<8; i++){
            Serial.print(char(packetBuffer[i]));
          }
          Serial.print(F("] "));
          Serial.print(UdpCommand.remoteIP());
          Serial.print(":");
          Serial.println(UdpCommand.remotePort());
        }
        if(packetBuffer[4]== 'b' && packetBuffer[5]== 'r' && packetBuffer[6]== 'o'){
          TxUDP('s', packetBuffer[2], 'c', 'f', 'm', 1);    // 0=broadcast, 1= direct to RX IP
          if(TxUdpBuffer[2] == 'm'){
            TxUDP('s', packetBuffer[2], ShiftOutByte[0], ShiftOutByte[1], ShiftOutByte[2], 1);
          }
        }

      // RX DATA
      }else{
        if(EnableGroupButton==true){
          CheckGroup();
        }else{
          ShiftOutByte[0] = String(packetBuffer[4], DEC).toInt();    // Bank0
        }
        ShiftOutByte[1] = String(packetBuffer[5], DEC).toInt();    // Bank1
        ShiftOutByte[2] = String(packetBuffer[6], DEC).toInt();    // Bank2

        // SHIFT OUT
        #if defined(ShiftOut)
          // digitalWrite(ShiftOutLatchPin, LOW);  // když dáme latchPin na LOW mužeme do registru poslat data
          // shiftOut(ShiftOutDataPin, ShiftOutClockPin, LSBFIRST, ShiftOutByte[2]);
          // shiftOut(ShiftOutDataPin, ShiftOutClockPin, LSBFIRST, ShiftOutByte[1]);
          // shiftOut(ShiftOutDataPin, ShiftOutClockPin, LSBFIRST, ShiftOutByte[0]);
          // digitalWrite(ShiftOutLatchPin, HIGH);    // jakmile dáme latchPin na HIGH data se objeví na výstupu
          if(EnableSerialDebug>0){
            Serial.println("ShiftOut");
          }
        #endif

        if(EnableSerialDebug>0){
          // Serial.println();
          Serial.print(F("RX ["));
          Serial.print(packetBuffer[0], HEX);
          for(int i=1; i<4; i++){
            Serial.print(char(packetBuffer[i]));
          }
          Serial.print((byte)packetBuffer[4], BIN);
          Serial.print(F("|"));
          Serial.print((byte)packetBuffer[5], BIN);
          Serial.print(F("|"));
          Serial.print((byte)packetBuffer[6], BIN);
          Serial.print(F(";] "));
          Serial.print(UdpCommand.remoteIP());
          Serial.print(F(":"));
          Serial.println(UdpCommand.remotePort());
        }
        if(UdpCommand.remotePort() != DetectedRemoteSwPort[hexToDecBy4bit(IdPrefix(packetBuffer[0]))] && EnableGroupPrefix==true){
          // if(EnableSerialDebug>0){
            Serial.print(F("** Change ip-port ID "));
            Serial.print(IdPrefix(packetBuffer[0]), HEX);
            Serial.print(F(" (OLD-"));
            Serial.print(DetectedRemoteSwPort[hexToDecBy4bit(IdPrefix(packetBuffer[0]))]);
            Serial.print(F(" NEW-"));
            Serial.print(UdpCommand.remotePort());
            Serial.println(F(") **"));
          // }
          DetectedRemoteSwPort[hexToDecBy4bit(IdPrefix(packetBuffer[0]))]=UdpCommand.remotePort();
        }
        TxUDP('s', packetBuffer[2], ShiftOutByte[0], ShiftOutByte[1], ShiftOutByte[2], 0);
      }
      WatchdogTimer=millis();
      // activate
      if(OutputWatchdog==123456){
        OutputWatchdog=EEPROM.readUInt(27);
      }
    } // filtered end
    else{
      if(EnableSerialDebug>0){
        Serial.println(F("   Different NET-ID, or bad packet format"));
      }
    }
    memset(packetBuffer, 0, sizeof(packetBuffer));   // Clear contents of Buffer
  } //end IfUdpPacketSice
}
//-------------------------------------------------------------------------------------------------------

void CheckGroup(){
  int ChangeBit=9;
  int NumberOfChange=0;
  for (int i=0; i<8; i++){
    if(bitRead(packetBuffer[4], i)!=bitRead(ShiftOutByte[0], i)){
      ChangeBit=i;
      NumberOfChange++;
    }
  }
  // Serial.print("ChangeBit|NumberOfChange ");
  // Serial.print(ChangeBit+1);
  // Serial.print(" ");
  // Serial.println(NumberOfChange);

  ShiftOutByte[0] = String(packetBuffer[4], DEC).toInt();    // Bank0
  if(NumberOfChange==1){
    // Serial.println("clearGroup");
    NumberOfChange=0;
    for (int i=0; i<8; i++){
      if(GroupButton[ChangeBit]==GroupButton[i] && ChangeBit!=i){
        bitClear(ShiftOutByte[0], i);
        NumberOfChange++;
        // Serial.print("Bitclear ");
        // Serial.println(i);
      }
    }
    if(NumberOfChange>0){
      bitSet(ShiftOutByte[0], ChangeBit);
    }
  }
}
//-------------------------------------------------------------------------------------------------------

unsigned char hexToDecBy4bit(unsigned char hex)
// convert a character representation of a hexidecimal digit into the actual hexidecimal value
{
  if(hex > 0x39) hex -= 7; // adjust for hex letters upper or lower case
  return(hex & 0xf);
}

//-------------------------------------------------------------------------------------------------------

void TxUDP(byte FROM, byte TO, byte A, byte B, byte C, int DIRECT){

  // TxUdpBuffer[0] = NET_ID;
  TxUdpBuffer[1] = FROM;
  // TxUdpBuffer[2] = TO;

  // if(TxUdpBuffer[2]=='m' && ( EnableGroupPrefix==true || EnableGroupButton==true ) ){
  //   TxUdpBuffer[3] = B00101101;           // -  multi control || GroupButton
  // }else{
  //   TxUdpBuffer[3] = B00111010;           // :
  // }

  TxUdpBuffer[3] = B00000000;
    // multi control
    if(TxUdpBuffer[2]=='m' && EnableGroupPrefix==true){
      bitSet(TxUdpBuffer[3], 0);
    }
    // group button
    if(TxUdpBuffer[2]=='m' && EnableGroupButton==true){
      bitSet(TxUdpBuffer[3], 1);
    }

  TxUdpBuffer[4] = A;
  TxUdpBuffer[5] = B;
  TxUdpBuffer[6] = C;
  TxUdpBuffer[7] = B00111011;           // ;

  // BROADCAST
  if(A=='b' && B=='r' && C=='o'){  // b r o
    if(TxUdpBuffer[2] == 'm'){
      TxUdpBuffer[6] = NumberOfEncoderOutputs;
    }
    // direct
    if(DIRECT==0){
      RemoteSwIP = ~ETH.subnetMask() | ETH.gatewayIP();
      if(EnableSerialDebug>0){
        Serial.print(F("TX broadcast ["));
      }
    }else{
      RemoteSwIP = UdpCommand.remoteIP();
      if(EnableSerialDebug>0){
        Serial.print(F("TX direct ["));
      }
    }
    UdpCommand.beginPacket(RemoteSwIP, BroadcastPort);

  // CFM
  }else if(A=='c' && B=='f' && C=='m'){  // cfm
      if(TxUdpBuffer[2] == 'm'){
        TxUdpBuffer[6] = NumberOfEncoderOutputs;
      }
      if(EnableSerialDebug>0){
        Serial.print(F("TX direct ["));
      }
      UdpCommand.beginPacket(UdpCommand.remoteIP(), UdpCommand.remotePort());

  // DATA
  }else{
    RemoteSwIP = UdpCommand.remoteIP();
    if(EnableSerialDebug>0){
      Serial.print(F("TX ["));
    }
    UdpCommand.beginPacket(RemoteSwIP, UdpCommand.remotePort());
  }

  // send
  if(EnableGroupPrefix==false){
    UdpCommand.write(TxUdpBuffer, sizeof(TxUdpBuffer));   // send buffer
    UdpCommand.endPacket();
    if(EnableSerialDebug>0){
      Serial.print(TxUdpBuffer[0], HEX);
      Serial.print(char(TxUdpBuffer[1]));
      Serial.print(char(TxUdpBuffer[2]));
      Serial.print(F("|"));
      Serial.print(TxUdpBuffer[3], BIN);
      Serial.print(F("|"));
      Serial.print(TxUdpBuffer[4], BIN);
      Serial.print(F("|"));
      Serial.print(TxUdpBuffer[5], BIN);
      Serial.print(F("|"));
      Serial.print(TxUdpBuffer[6], BIN);
      Serial.print(char(TxUdpBuffer[7]));
      Serial.print(F("] "));
      Serial.print(RemoteSwIP);
      Serial.print(F(":"));
      Serial.print(UdpCommand.remotePort());
      #if defined(WIFI)
        Serial.print(" | dBm: ");
        Serial.print(WiFi.RSSI());
      #endif
      Serial.println();
    }

  // send EnableGroupPrefix
  }else{
    // answer to RX ip
    TxUdpBuffer[0]=packetBuffer[0];   // NET_ID by RX NET_ID
    UdpCommand.write(TxUdpBuffer, sizeof(TxUdpBuffer));   // send buffer
    UdpCommand.endPacket();
    if(EnableSerialDebug>0){
      Serial.print(TxUdpBuffer[0], HEX);
      for (int i=1; i<4; i++){
        Serial.print(char(TxUdpBuffer[i]));
      }
      Serial.print(TxUdpBuffer[4], BIN);
      Serial.print(F("|"));
      Serial.print(TxUdpBuffer[5], BIN);
      Serial.print(F("|"));
      Serial.print(TxUdpBuffer[6], BIN);
      Serial.print(char(TxUdpBuffer[7]));
      Serial.print(F("] "));
      Serial.print(RemoteSwIP);
      Serial.print(F(":"));
      Serial.print(UdpCommand.remotePort());
      #if defined(WIFI)
        Serial.print(" | dBm: ");
        Serial.print(WiFi.RSSI());
      #endif
      Serial.println();
    }
    // send to all ip from storage
    IPAddress ControllerIP = UdpCommand.remoteIP();
    for (int i=0; i<16; i++){
      if(DetectedRemoteSwPort[i]!=0){
        TxUdpBuffer[0]=IdSufix(0) | i<<4;       // NET_ID by destination device
        RemoteSwIP = DetectedRemoteSw[i];
        RemoteSwPort = DetectedRemoteSwPort[i];
        if(ControllerIP!=RemoteSwIP){
          UdpCommand.beginPacket(RemoteSwIP, RemoteSwPort);
            UdpCommand.write(TxUdpBuffer, sizeof(TxUdpBuffer));   // send buffer
          UdpCommand.endPacket();

          if(EnableSerialDebug>0){
            Serial.print(F("TX direct ID-"));
            Serial.print(i, HEX);
            Serial.print(IdSufix(0), HEX);
            Serial.print(F(" "));
            Serial.print(RemoteSwIP);
            Serial.print(F(":"));
            Serial.print(RemoteSwPort);
            Serial.print(F(" ["));
            Serial.print(TxUdpBuffer[0], HEX);
            for (int i=1; i<8; i++){
              Serial.print(char(TxUdpBuffer[i]));
              // Serial.print(F(" "));
            }
            Serial.print("]");
            #if defined(WIFI)
              Serial.print(" WiFi dBm: ");
              Serial.print(WiFi.RSSI());
            #endif
            Serial.println();
          }
        }else{
          if(EnableSerialDebug>0){
            Serial.print(F("noTX - RX prefix "));
            Serial.print(i, HEX);
            Serial.print(F(" "));
            Serial.print(RemoteSwIP);
            Serial.print(F(":"));
            Serial.println(RemoteSwPort);
          }
        }
      }
    }
    // broadcast all prefix
    if(A=='b' && B=='r' && C=='o' && DIRECT==0 && TxUdpBuffer[2] == 'm'){
      if(EnableSerialDebug>0){
        Serial.print("TX all prefix ");
        Serial.print(RemoteSwIP);
        Serial.print(":");
        Serial.print(BroadcastPort);
        Serial.print(F(" ["));
        Serial.print("*");
        for (int i=1; i<8; i++){
          Serial.print(char(TxUdpBuffer[i]));
          // Serial.print(F(" "));
        }
        Serial.println("]");
      }
      Serial.print("*) ");
      for (int i=0; i<16; i++){
        TxUdpBuffer[0]=IdSufix(0) | (i<<4);
        if(EnableSerialDebug>0){
          Serial.print(TxUdpBuffer[0], HEX);
          Serial.print(" ");
        }
        UdpCommand.beginPacket(RemoteSwIP, BroadcastPort);
        UdpCommand.write(TxUdpBuffer, sizeof(TxUdpBuffer));   // send buffer
        UdpCommand.endPacket();
      }
      if(EnableSerialDebug>0){
        Serial.println();
      }
    }   // end b r o

  } // end EnableGroupPrefix
}
//-------------------------------------------------------------------------------------------------------

void http(){
  // listen for incoming clients
  WiFiClient webClient = server.available();
  if (webClient) {
    Interrupts(false);
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
          webClient.print(F(" | version: "));
          webClient.println(REV);
          // webClient.print(F(" | PCB: "));
          // webClient.println(HWREV);
          webClient.print(F(" | eth mac: "));
          webClient.print(MACString);
          webClient.println();

          webClient.print(F(" | dhcp: "));
          if(DHCP_ENABLE==1){
            webClient.print(F("ON"));
          }else{
            webClient.print(F("OFF"));
          }
          webClient.print(F(" | ip: "));
          webClient.println(ETH.localIP());
          // webClient.print(F(" | utc from ntp: "));
          // webClient.println(F("timeClient.getFormattedTime()"));
          // webClient.println(F("<br>MQTT subscribe command: $ mosquitto_sub -v -h mqttstage.prusa -t prusa-debug/prusafil/extrusionline/+/#"));
          webClient.print(F(" | Broker ip: "));
          webClient.print(mqtt_server_ip[0]);
          webClient.print(F("."));
          webClient.print(mqtt_server_ip[1]);
          webClient.print(F("."));
          webClient.print(mqtt_server_ip[2]);
          webClient.print(F("."));
          webClient.print(mqtt_server_ip[3]);
          webClient.print(F(":"));
          webClient.print(MQTT_PORT);
          #if defined(OTAWEB)
            webClient.print(F(" | <a href=\"http://"));
            webClient.println(ETH.localIP());
            webClient.print(F(":82/update\" target=_blank>Upload FW</a> | <a href=\"https://github.com/ok1hra/IP-rotator/releases\" target=_blank>Releases</a> | <a href=\"http://"));
            webClient.println(ETH.localIP());
            webClient.print(F(":88\" target=_blank>html preview</a>"));
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
   Interrupts(true);
  }
}
//-------------------------------------------------------------------------------------------------------

void http2(){
  #if defined(WWWtwo)
  // listen for incoming clients
  WiFiClient webClient2 = server2.available();
  if (webClient2) {
    Interrupts(false);
    if(EnableSerialDebug>0){
      Serial.println("WIFI New webClient2");
    }
    memset(linebuf,0,sizeof(linebuf));
    charcount=0;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (webClient2.connected()) {
      if (webClient2.available()) {
        char c = webClient2.read();
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
          webClient2.println(F("HTTP/1.1 200 OK"));
          webClient2.println(F("Content-Type: text/html"));
          webClient2.println(F("Connection: close"));  // the connection will be closed after completion of the response
          webClient2.println();
          webClient2.print(F("<!doctype html><html><head><title>"));
          webClient2.print(YOUR_CALL);
          webClient2.print(F(" ROT</title><meta http-equiv=\"refresh\" content=\"1800\"><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"><style type=\"text/css\">body {font-family: 'Roboto Condensed',sans-serif,Arial,Tahoma,Verdana; background: #444;}</style><link href='http://fonts.googleapis.com/css?family=Roboto+Condensed:300italic,400italic,700italic,400,700,300&subset=latin-ext' rel='stylesheet' type='text/css'></head><body><p style=\"color: #ccc; margin: 0 0 0 0; text-align: center;\"><span style=\"color: #999; font-size: 800%;\">"));
          // if(ExtTemp==true){
          //   sensors.requestTemperatures();
          //   int temperatureC = sensors.getTempCByIndex(0);
          //   // dtostrf(temperatureC, 1, 0, buf);  //1 is mininum width, 0 is precision
          //   webClient2.print(String(temperatureC));
          // }else{
          //   int temperatureC = htu.readTemperature();
          //   webClient2.print(String(temperatureC));
          //   Serial.println(temperatureC);
          // }

          webClient2.print(String(Azimuth));
          webClient2.println(F("&deg;</span><br><span style=\"color: #000; background: #080; padding: 4px 6px 4px 6px; -webkit-border-radius: 5px; -moz-border-radius: 5px; border-radius: 5px;\">"));
          #if defined(HTU21D)
            if(HTU21Denable==true){
              webClient2.print(String(constrain(htu.readHumidity(), 0, 100)));
            }
          #endif
          webClient2.print(F("% | "));
          #if defined(BMP280)
            if(BMP280enable==true){
              webClient2.print(String(Babinet(double(bmp.readPressure()), double(htu.readTemperature()*1.8+32))/100));
            }
          #endif
          webClient2.print(F(" hPa "));
          if(RainCount>0){
            webClient2.print(F("| <strong style=\"color: #008;\">"));
            webClient2.print(String(RainPulseToMM(RainCount)));
            webClient2.print(F(" mm </strong>"));
          }
          webClient2.print(F("| <strong style=\"color: #fff;\">"));
          webClient2.print(String(PulseToMetterBySecond(PeriodMinRpmPulse)));
          webClient2.print(F(" m/s</strong></span><br><span style=\"font-size: 600%; transform: rotate("));
          webClient2.print(String(Azimuth));
          webClient2.print(F("deg); display: inline-block;\">&#10138;</span><br><a href=\""));
          webClient2.print( ETH.localIP() );
          webClient2.print(F(":88\" onclick=\"window.open( this.href, this.href, 'width=270,height=330,left=0,top=0,menubar=no,location=no,status=no' ); return false;\" style=\"color:#666;text-decoration:none;\">UTC "));
          webClient2.print(UtcTime(1));
          webClient2.println(F("</span></p></body></html>"));

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
    webClient2.stop();
   if(EnableSerialDebug>0){
     Serial.println("WIFI webClient2 disconnected");
     MeasureTimer[0]=millis()+5000-MeasureTimer[1];
   }
   Interrupts(true);
  }
  #endif
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
      Serial.println("===============================");
      Serial.print("   IPv4: ");
      Serial.println(ETH.localIP());
      Serial.println("===============================");
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
            Prn(3, 1, String(MACchar));
            mqttReconnect();
            AfterMQTTconnect();
          }
        }
      #endif
      ListCommands(0);

      // EnableSerialDebug=1;
      TxUDP('s', packetBuffer[2], 'b', 'r', 'o', 0);    // 0=broadcast, 1= direct to RX IP
      if(TxUdpBuffer[2] == 'm'){
        TxUDP('s', packetBuffer[2], ShiftOutByte[0], ShiftOutByte[1], ShiftOutByte[2], 0);
      }
      // EnableSerialDebug=0;
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
        Status = -3;
      }else if(Status>0){
        Status = 3;
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

        MqttPubString("StartAzimuth", String(StartAzimuth), true);
        MqttPubString("Name", String(RotName), true);

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
  Interrupts(false);
  // if(EnableEthernet==1 && MQTT_ENABLE==1 && EthLinkStatus==1 && mqttClient.connected()==true){
  if(mqttClient.connected()==true){
    if (mqttClient.connect(MACchar)) {
      String topic = String(YOUR_CALL) + "/" + String(NET_ID) + "/ROT/"+TOPIC;
      topic.toCharArray( mqttPath, 50 );
      DATA.toCharArray( mqttTX, 50 );
      mqttClient.publish(mqttPath, mqttTX, RETAIN);
    }
  }
  Interrupts(true);
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

void SerialToIp(){
  #if defined(Ser2net)
  uint8_t i;
  // if (wifiMulti.run() == WL_CONNECTED) {
  if (eth_connected==true) {
    //check if there are any new clients
    if (SerialServer.hasClient()){
      for(i = 0; i < MAX_SRV_CLIENTS; i++){
        //find free/disconnected spot
        if (!SerialServerClients[i] || !SerialServerClients[i].connected()){
          if(SerialServerClients[i]) SerialServerClients[i].stop();
          SerialServerClients[i] = SerialServer.available();
          if (!SerialServerClients[i]) Serial.println("available broken");
          if(EnableSerialDebug>0){
            Serial.println();
            Serial.print("New Ser2net client: ");
            Serial.print(i); Serial.print(' ');
            Serial.println(SerialServerClients[i].remoteIP());
          }
          break;
        }
      }
      if (i >= MAX_SRV_CLIENTS) {
        //no free/disconnected spot so reject
        SerialServer.available().stop();
      }
    }
    //check clients for data
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      if (SerialServerClients[i] && SerialServerClients[i].connected()){
        if(SerialServerClients[i].available()){
          //get data from the telnet client and push it to the UART
          // while(SerialServerClients[i].available()) Serial_one.write(SerialServerClients[i].read());
          if(EnableSerialDebug>0){
            Serial.println();
            Serial.print("rx-");
          }

/*
Zapinaci pakety
< 2019/04/17 22:05:29.670771  length=1 from=2009 to=2009
 0d
< 2019/04/17 22:05:29.734563  length=3 from=2010 to=2012
 30 31 0d
> 2019/04/17 22:05:29.924267  length=2 from=3338 to=3339
 0d 0d
> 2019/04/17 22:05:29.972263  length=3 from=3340 to=3342
 30 31 0d
< 2019/04/17 22:05:30.086153  length=1 from=2013 to=2013
 0d
> 2019/04/17 22:05:30.249406  length=3 from=3343 to=3345
 30 31 0d
< 2019/04/17 22:05:30.261738  length=2 from=2014 to=2015
 ff 0d

Udrzovaci pakety
> 2019/04/17 21:59:00.549347  length=2 from=3319 to=3320
 ff 0d
< 2019/04/17 21:59:00.559100  length=2 from=1998 to=1999
 ff 0d
< 2019/04/17 21:59:02.555837  length=1 from=2000 to=2000
 0d
< 2019/04/17 21:59:04.568329  length=1 from=2001 to=2001
 0d
> 2019/04/17 21:59:05.549832  length=2 from=3321 to=3322
 ff 0d
< 2019/04/17 21:59:05.558790  length=2 from=2002 to=2003
 ff 0d

Vypinaci Paket
 < 2019/04/17 21:59:10.382425  length=3 from=2006 to=2008
 30 30 0d
> 2019/04/17 21:59:10.388125  length=15 from=3323 to=3337
 0d 0d 0d 0d 0d 0d 0d 0d 0d 0d 0d 0d 30 30 0d

*/


          while(SerialServerClients[i].available()){
            byte RX;
            RX=SerialServerClients[i].read();
            Serial_one.write(RX);
            if(EnableSerialDebug>0){
              Serial.write(RX);
            }
          }
        }
      }
      else {
        if (SerialServerClients[i]) {
          SerialServerClients[i].stop();
        }
      }
    }
    //check UART for data
    if(Serial_one.available()){
      size_t len = Serial_one.available();
      uint8_t sbuf[len];
      Serial_one.readBytes(sbuf, len);
      //push UART data to all connected telnet clients
      for(i = 0; i < MAX_SRV_CLIENTS; i++){
        if (SerialServerClients[i] && SerialServerClients[i].connected()){
          SerialServerClients[i].write(sbuf, len);
          // delay(1);
          if(EnableSerialDebug>0){
            Serial.println();
            Serial.print("tx-");
            Serial.write(sbuf, len);
          }
        }
      }
    }
  }
  else {
    if(EnableSerialDebug>0){
      Serial.println("Ser2net not connected!");
    }
    for(i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (SerialServerClients[i]) SerialServerClients[i].stop();
    }
    delay(1000);
  }
  #endif
}
//-------------------------------------------------------------------------------------------------------
// void bmp280(){
//   unsigned int b1[24];
//   unsigned int data[8];
//   for (int i = 0; i < 24; i++)
//   {
//     // Start I2C Transmission
//     Wire.beginTransmission(BMP280Addr);
//     // Select data register
//     Wire.write((136 + i));
//     // Stop I2C Transmission
//     Wire.endTransmission();
//
//     // Request 1 byte of data
//     Wire.requestFrom(BMP280Addr, 1);
//
//     // Read 1 byte of data
//     if (Wire.available() == 1)
//     {
//       b1[i] = Wire.read();
//     }
//   }
//   // Convert the data
//   // temp coefficients
//   unsigned int dig_T1 = (b1[0] & 0xFF) + ((b1[1] & 0xFF) * 256);
//   int dig_T2 = b1[2] + (b1[3] * 256);
//   int dig_T3 = b1[4] + (b1[5] * 256);
//
//   // pressure coefficients
//   unsigned int dig_P1 = (b1[6] & 0xFF) + ((b1[7] & 0xFF) * 256);
//   int dig_P2 = b1[8] + (b1[9] * 256);
//   int dig_P3 = b1[10] + (b1[11] * 256);
//   int dig_P4 = b1[12] + (b1[13] * 256);
//   int dig_P5 = b1[14] + (b1[15] * 256);
//   int dig_P6 = b1[16] + (b1[17] * 256);
//   int dig_P7 = b1[18] + (b1[19] * 256);
//   int dig_P8 = b1[20] + (b1[21] * 256);
//   int dig_P9 = b1[22] + (b1[23] * 256);
//
//   // Start I2C Transmission
//   Wire.beginTransmission(BMP280Addr);
//   // Select control measurement register
//   Wire.write(0xF4);
//   // Normal mode, temp and pressure over sampling rate = 1
//   Wire.write(0x27);
//   // Stop I2C Transmission
//   Wire.endTransmission();
//
//   // Start I2C Transmission
//   Wire.beginTransmission(BMP280Addr);
//   // Select config register
//   Wire.write(0xF5);
//   // Stand_by time = 1000ms
//   Wire.write(0xA0);
//   // Stop I2C Transmission
//   Wire.endTransmission();
//
//   for (int i = 0; i < 8; i++)
//   {
//     // Start I2C Transmission
//     Wire.beginTransmission(BMP280Addr);
//     // Select data register
//     Wire.write((247 + i));
//     // Stop I2C Transmission
//     Wire.endTransmission();
//
//     // Request 1 byte of data
//     Wire.requestFrom(BMP280Addr, 1);
//
//     // Read 1 byte of data
//     if (Wire.available() == 1)
//     {
//       data[i] = Wire.read();
//     }
//   }
//
//   // Convert pressure and temperature data to 19-bits
//   long adc_p = (((long)(data[0] & 0xFF) * 65536) + ((long)(data[1] & 0xFF) * 256) + (long)(data[2] & 0xF0)) / 16;
//   long adc_t = (((long)(data[3] & 0xFF) * 65536) + ((long)(data[4] & 0xFF) * 256) + (long)(data[5] & 0xF0)) / 16;
//
//   // Temperature offset calculations
//   double var1 = (((double)adc_t) / 16384.0 - ((double)dig_T1) / 1024.0) * ((double)dig_T2);
//   double var2 = ((((double)adc_t) / 131072.0 - ((double)dig_T1) / 8192.0) *
//                  (((double)adc_t) / 131072.0 - ((double)dig_T1) / 8192.0)) * ((double)dig_T3);
//   double t_fine = (long)(var1 + var2);
//   double cTemp = (var1 + var2) / 5120.0;
//   double fTemp = cTemp * 1.8 + 32;
//
//   // Pressure offset calculations
//   var1 = ((double)t_fine / 2.0) - 64000.0;
//   var2 = var1 * var1 * ((double)dig_P6) / 32768.0;
//   var2 = var2 + var1 * ((double)dig_P5) * 2.0;
//   var2 = (var2 / 4.0) + (((double)dig_P4) * 65536.0);
//   var1 = (((double) dig_P3) * var1 * var1 / 524288.0 + ((double) dig_P2) * var1) / 524288.0;
//   var1 = (1.0 + var1 / 32768.0) * ((double)dig_P1);
//   double p = 1048576.0 - (double)adc_p;
//   p = (p - (var2 / 4096.0)) * 6250.0 / var1;
//   var1 = ((double) dig_P9) * p * p / 2147483648.0;
//   var2 = p * ((double) dig_P8) / 32768.0;
//   double pressure = (p + (var1 + var2 + ((double)dig_P7)) / 16.0) / 100;
//
//   float height = 44330 * (1 - pow((pressure / 1013.25), 0.1903));
//   float h = height * 0.3048;
//
//   // Output data to serial monitor
//  Serial.print("Altitude : ");
//  Serial.print(height);
//  Serial.println(" m");
//  Serial.print("Altitude in Feet : ");
//  Serial.println(h);
//
//  Serial.print("Pressure : ");
//  Serial.print(pressure);
//  Serial.println(" hPa");
//  Serial.print("Temperature in Celsius : ");
//  Serial.print(cTemp);
//  Serial.println(" C");
//  Serial.print("Temperature in Fahrenheit : ");
//  Serial.print(fTemp);
//  Serial.println(" F");
//
//   //volatile float tempc, tempf, presr, altim, altif
//
//   // tempc = cTemp;
//   // tempf = fTemp;
//   // presr = pressure;
//   // altm = height;
//   // altf = h;
//   // delay(1000);
// }
//-------------------------------------------------------------------------------------------------------
// void printBME() {
//   Serial.print("Temperature = ");
//   Serial.print(bme.readTemperature());
//   Serial.println(" *C");
//
//   // Convert temperature to Fahrenheit
//   /*Serial.print("Temperature = ");
//   Serial.print(1.8 * bme.readTemperature() + 32);
//   Serial.println(" *F");*/
//
//   Serial.print("Pressure = ");
//   Serial.print(bme.readPressure() / 100.0F);
//   Serial.println(" hPa");
//
//   // Serial.print("Approx. Altitude = ");
//   // Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
//   // Serial.println(" m");
//
//   Serial.println();
// }
//-------------------------------------------------------------------------------------------------------
void I2cScanner() {
  byte error, address;
  int nDevices;
  // Serial.println("Scanning...");
  Prn(3, 1,"Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      // Serial.print("I2C device found at address 0x");
      Prn(3, 0,"I2C device found at address 0x");
      if (address<16) {
        // Serial.print("0");
        Prn(3, 0,"0");
      }
      // Serial.println(address,HEX);
      Prn(3, 1,String(address,HEX));
      nDevices++;
    }
    else if (error==4) {
      // Serial.print("Unknow error at address 0x");
      Prn(3, 1,"Unknow error at address 0x");
      if (address<16) {
        // Serial.print("0");
        Prn(3, 0,"0");
      }
      // Serial.println(address,HEX);
      Prn(3, 1,String(address,HEX));
    }
  }
  if (nDevices == 0) {
    // Serial.println("No I2C devices found\n");
    Prn(3, 1,"No I2C devices found");
  }
  else {
    // Serial.println("done\n");
    Prn(3, 1,"done");
  }
  // delay(5000);
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

//--SD-----------------------------------------------------------------------------------------------------

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }


    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}

#if defined(DS18B20)
  // function to print a device address
  void printAddress(DeviceAddress deviceAddress)
  {
    for (uint8_t i = 0; i < 8; i++)
    {
      // zero pad the address if necessary
      if (deviceAddress[i] < 16) Serial.print("0");
      Serial.print(deviceAddress[i], HEX);
    }
  }

  // function to print the temperature for a device
  void printTemperature(DeviceAddress deviceAddress)
  {
    float tempC = sensors.getTempC(deviceAddress);
    if(tempC == DEVICE_DISCONNECTED_C)
    {
      Serial.println("DS18B20 Error: Could not read temperature data");
      return;
    }
    // Serial.print("Temp C: ");
    Serial.print(tempC);
  //  Serial.print(" Temp F: ");
  //  Serial.print(DallasTemperature::toFahrenheit(tempC));
  }

  // function to print a device's resolution
  void printResolution(DeviceAddress deviceAddress)
  {
    Serial.print("Resolution: ");
    Serial.print(sensors.getResolution(deviceAddress));
    Serial.println();
  }

  // main function to print information about a device
  void printData(DeviceAddress deviceAddress)
  {
    Serial.print("DS18B20 address: ");
    printAddress(deviceAddress);
    Serial.print(" ");
    printTemperature(deviceAddress);
    Serial.println();
  }
#endif

// ajax
void handlePostRot() {
 // String s = MAIN_page; //Read HTML contents
 String str = ajaxserver.arg("ROT");
 AzimuthTarget = str.toInt() + StartAzimuth;
 if(AzimuthTarget>359){
   AzimuthTarget = AzimuthTarget - 360;
 }
 RotCalculate();
 // ajaxserver.send(200, "text/html", s);



 // MqttPubString("Debug 4 ", String(ajaxserver.arg("STOP")), false);
 // if(ajaxserver.hasArg("on") && (ajaxserver.arg("on").length()>0)){
    // ajaxserver.sendHeader("Location", String("/"), true); //how to do a redirect, next two lines
    // ajaxserver.send ( 302, "text/plain", "");
  // } else {
  //   ajaxserver.send(400, "text/html", "<html><body><h1>HTTP Error 400</h1><p>Bad request. Please enter a value.</p></body></html>");
  // }
}
// MqttPubString("Debug 3 ", String(ajaxserver.hasArg("STOP")), false);
void handleSet() {
  // MqttPubString("Debug 1 ", String(ajaxserver.arg("yourcall")), false);
  // MqttPubString("Debug 1b ", String(ajaxserver.hasArg("yourcall")), false);
  // MqttPubString("Debug 2 ", String(ajaxserver.arg("rotid")), false);
  // MqttPubString("Debug 3 ", String(ajaxserver.arg("rotname")), false);
  // MqttPubString("Debug 5 ", String(ajaxserver.arg("maxrotatedegree")), false);
  // MqttPubString("Debug 6 ", String(ajaxserver.arg("mapurl")), false);
  // MqttPubString("Debug 7 ", String(ajaxserver.arg("antradiationangle")), false);
  // MqttPubString("Debug 8 ", String(ajaxserver.arg("edstops")), false);
  // MqttPubString("Debug startAZ", String(EEPROM.readInt(23)), false);
  // MqttPubString("Debug endstop eeprom", String(EEPROM.read(26)), false);

  String yourcallERR= "";
  String rotidERR= "";
  String rotnameERR= "";
  String startazimuthERR= "";
  String maxrotatedegreeERR= "";
  String antradiationangleERR= "";
  String mapurlERR= "";
  String mqttERR= "";
  String mqttportERR= "";
  String edstopsCHECKED= "";
  String acmotorCHECKED= "";

  if ( ajaxserver.hasArg("yourcall") == false \
    && ajaxserver.hasArg("rotid") == false \
    && ajaxserver.hasArg("rotname") == false \
    && ajaxserver.hasArg("startazimuth") == false \
    && ajaxserver.hasArg("maxrotatedegree") == false \
    && ajaxserver.hasArg("mapurl") == false \
    && ajaxserver.hasArg("antradiationangle") == false \
  ) {
    MqttPubString("Debug", "Form not valid", false);
  }else{
    MqttPubString("Debug", "Form valid", false);

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
        // MqttPubString("Debug StartAzimuth changed", String(StartAzimuth), false);
        EEPROM.writeUShort(23, StartAzimuth);
        // EEPROM.commit();
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

    // 29  - Endstop
    if(ajaxserver.arg("edstops").toInt()==1 && Endstop==false){
      Endstop = true;
      EEPROM.writeBool(29, 1);
      // EEPROM.commit();
    }else if(ajaxserver.arg("edstops").toInt()!=1 && Endstop==true){
      Endstop = false;
      EEPROM.writeBool(29, 0);
      // EEPROM.commit();
    }

    // 30  - ACmotor
    if(ajaxserver.arg("edstops").toInt()==1 && ACmotor==false){
      ACmotor = true;
      EEPROM.writeBool(30, 1);
      // EEPROM.commit();
    }else if(ajaxserver.arg("edstops").toInt()!=1 && ACmotor==true){
      ACmotor = false;
      EEPROM.writeBool(30, 0);
      // EEPROM.commit();
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

  // MqttPubString("Debug Endstop", String(Endstop), false);
if(Endstop==true){
  edstopsCHECKED= "checked";
}else{
  // edstopsCHECKED= "disabled";
  edstopsCHECKED= "";
}

if(ACmotor==true){
  acmotorCHECKED= "checked";
}else{
  // edstopsCHECKED= "disabled";
  acmotorCHECKED= "";
}



// MqttPubString("Debug startazimuth", String(ajaxserver.arg("startazimuth")), false);
// MqttPubString("Debug StartAzimuth", String(StartAzimuth), false);
// MqttPubString("Debug StartAzimuth EEPROM", String(EEPROM.readUShort(23)), false);
// MqttPubString("Debug MaxRotateDegree EEPROM", String(EEPROM.readUShort(24)), false);
// MqttPubString("Debug AntRadiationAngle EEPROM", String(EEPROM.readUShort(25)), false);



  String HtmlSrc = "<!DOCTYPE html><html><head><title>SETUP</title>\n";
  HtmlSrc +="<meta http-equiv='Content-Type' content='text/html; charset=UTF-8'><meta http-equiv = 'refresh' content = '600; url = /'>\n";
  HtmlSrc +="<style type='text/css'> table, th, td {color: #fff; border:0px } .tdr {color: #0c0; height: 40px; text-align: right; vertical-align: middle;} html,body {background-color: #000; text-color: #ccc; font-family: 'Roboto Condensed',sans-serif,Arial,Tahoma,Verdana;} a:hover {color: #fff;} a { color: #ccc; text-decoration: underline;} ";
  HtmlSrc +=".tooltip-text {visibility: hidden; position: absolute; z-index: 1; width: 300px; color: white; font-size: 12px; background-color: #DE3163; border-radius: 10px; padding: 10px 15px 10px 15px; } .hover-text:hover .tooltip-text { visibility: visible; } #right { top: -30px; left: 200%; } #top { top: -60px; left: -150%; } #left { top: -8px; right: 120%;}";
  HtmlSrc +=".hover-text {position: relative; background: #888; padding: 5px 12px; margin: 5px; font-size: 15px; border-radius: 100%; color: #FFF; display: inline-block; text-align: center; }</style>\n";
  HtmlSrc +="<link href='http://fonts.googleapis.com/css?family=Roboto+Condensed:300italic,400italic,700italic,400,700,300&subset=latin-ext' rel='stylesheet' type='text/css'></head><body>\n";
  HtmlSrc +="<H1 style='color: #fff; text-align: center;'>Setup</H1>\n";
  HtmlSrc +="<div style='display: flex; justify-content: center;'><table><form action='/set' method='post' style='color: #ccc; margin: 50 0 0 0; text-align: center;'>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='yourcall'>Your callsign:</label></td><td><input type='text' id='yourcall' name='yourcall' size='10' value='";
  HtmlSrc += YOUR_CALL;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += yourcallERR;
  HtmlSrc +="</span></td></tr>\n<tr><td class='tdr'><label for='rotid'>Rotator ID:</label></td><td><input type='text' id='rotid' name='rotid' size='2' value='";
  HtmlSrc += NET_ID;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += rotidERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 50px;'>1-2 chars</span></span></td></tr>\n<tr><td class='tdr'><label for='rotname'>Rotator name:</label></td><td><input type='text' id='rotname' name='rotname' size='20' value='";
  HtmlSrc += RotName;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += rotnameERR;
  HtmlSrc +="</span></td></tr>\n<tr><td class='tdr'><label for='startazimuth'>Start CCW azimuth:</label></td><td><input type='text' id='startazimuth' name='startazimuth' size='3' value='";
  HtmlSrc += StartAzimuth;
  HtmlSrc +="'>&deg; <span style='color:red;'>";
  HtmlSrc += startazimuthERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 100px;'>Allowed range 0-359&deg;</span></span></td></tr>\n<tr><td class='tdr'><label for='maxrotatedegree'>Rotation range in degrees:</label></td><td><input type='text' id='maxrotatedegree' name='maxrotatedegree' size='3' value='";
  HtmlSrc += MaxRotateDegree;
  HtmlSrc +="'>&deg; <span style='color:red;'>";
  HtmlSrc += maxrotatedegreeERR;
  HtmlSrc +="</span></td></tr>\n<tr><td class='tdr'><label for='mapurl'>Background azimuth map URL:</label></td><td><input type='text' id='mapurl' name='mapurl' size='30' value='";
  HtmlSrc += MapUrl;
  HtmlSrc +="'><span style='color:red;'>";
  HtmlSrc += mapurlERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='left'>DXCC generated every quarter hour is available at https://remoteqth.com/xplanet/. If you need another, please contact OK1HRA.</span></span> <a href='https://remoteqth.com/xplanet/' target='_blank'>Available list</a></td></tr>\n<tr><td class='tdr'><label for='antradiationangle'>Antenna radiation angle in degrees:</label></td><td><input type='text' id='antradiationangle' name='antradiationangle' size='3' value='";
  HtmlSrc += AntRadiationAngle;
  HtmlSrc +="'>&deg; <span style='color:red;'>";
  HtmlSrc += antradiationangleERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 100px;'>Allowed range 1-180&deg;</span></span></td></tr>\n<tr><td class='tdr'><label for='edstops'>Endstops:</label></td><td><input type='checkbox' id='edstops' name='edstops' value='1' ${postData.edstops?'checked':''} ";
  HtmlSrc += edstopsCHECKED;
  HtmlSrc +="><span class='hover-text'>?<span class='tooltip-text' id='top'>If disabled, it reduces the range of the potentiometer by the forbidden zone on edges</span></span></td></tr>\n";
  HtmlSrc +="<tr><td class='tdr'><label for='acmotor'>AC motor:</label></td><td><input type='checkbox' id='acmotor' name='acmotor' value='1' ${postData.acmotor?'checked':''} ";
  HtmlSrc += acmotorCHECKED;
  HtmlSrc +="><span class='hover-text'>?<span class='tooltip-text' id='top'>Disables the PWM and activates the other two relays.</span></span></td></tr>\n";


  HtmlSrc +="<tr><td class='tdr'><label for='mqttip0'>MQTT broker IP:</label></td><td>";
  HtmlSrc +="<input type='text' id='mqttip0' name='mqttip0' size='1' value='" + String(mqtt_server_ip[0]) + "'>&nbsp;.&nbsp;<input type='text' id='mqttip1' name='mqttip1' size='1' value='" + String(mqtt_server_ip[1]) + "'>&nbsp;.&nbsp;<input type='text' id='mqttip2' name='mqttip2' size='1' value='" + String(mqtt_server_ip[2]) + "'>&nbsp;.&nbsp;<input type='text' id='mqttip3' name='mqttip3' size='1' value='" + String(mqtt_server_ip[3]) + "'>";
  HtmlSrc +="<span style='color:red;'>";
  HtmlSrc += mqttERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 250px;'>Default public broker 54.38.157.134</span></span></td></tr>\n";

  HtmlSrc +="<tr><td class='tdr'><label for='mqttport'>MQTT broker PORT:</label></td><td>";
  HtmlSrc +="<input type='text' id='mqttport' name='mqttport' size='2' value='" + String(MQTT_PORT) + "'>\n";
  HtmlSrc +="<span style='color:red;'>";
  HtmlSrc += mqttportERR;
  HtmlSrc +="</span><span class='hover-text'>?<span class='tooltip-text' id='top' style='width: 150px;'>Default public broker port 1883</span></span></td></tr>\n";



  // // MQTT broker IP
  // for(int i=0; i<4; i++){
  //   mqtt_server_ip[i]=EEPROM.readByte(i+161);
  // }
  // MQTT_PORT = EEPROM.readInt(165);
  // if(mqtt_server_ip[0]==255 && mqtt_server_ip[1]==255 && mqtt_server_ip[2]==255 && mqtt_server_ip[3]==255 && MQTT_PORT==-1){
  //   mqtt_server_ip[0]=54;
  //   mqtt_server_ip[1]=38;
  //   mqtt_server_ip[2]=157;
  //   mqtt_server_ip[3]=134;
  //   MQTT_PORT=1883;
  // }

  HtmlSrc +="<tr><td class='tdr'><button>CHANGE</button></form></td><td style='text-align: center;'><a href='/'><button>CANCEL</button></a></td></tr>\n";
  HtmlSrc +="</table></div><div style='display: flex; justify-content: center;'><span><p style='font-size: 20px; color: #000; text-align: center; font-weight: normal; background: #666; margin: 73px 0 0 0; padding: 4px 6px 4px 6px; -webkit-border-radius: 5px; -moz-border-radius: 5px; border-radius: 5px;'><a href='/'>&#8617; Back to CONTROL</a> | <a href='/cal'>CALIBRATE</a></p>\n";
  HtmlSrc +="<p style='text-align: center;'><a href='https://remoteqth.com/w/' target='_blank' alt='Rotator Wiki page'>More on Wiki &#10138;</a></p></span></div>\n";
  HtmlSrc +="</body></html>\n";

  String s = MAIN_page; //Read HTML contents
  ajaxserver.send(200, "text/html", HtmlSrc); //Send web page
}
void handlePostStop() {
  // MqttPubString("Debug 3 ", String(ajaxserver.hasArg("on")), false);
  // MqttPubString("Debug 4 ", String(ajaxserver.arg("on")), false);
  if(Status<0){
    Status = -3;
  }else if(Status>0){
    Status = 3;
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
