## MQTT  - Introducing Topics and Messages

(Last updated for firmware version 20241011)

**Topics** in MQTT are like channels for messages. They are used to organize and filter messages. For example, a device might publish its temperature data to the topic "home/temperature".

**Messages** are the actual data being transmitted. They contain the payload content. A message might be "72.5" for the temperature on the "home/temperature" topic, or it could be a binary object such as a picture.

Topics and messages work together to create a flexible and scalable messaging system.

## IP-Rotator and MQTT

The IP-Rotator derives the variables to configure the Topic string from the web configuration interface. An example of a Topic string which displays the current bearing is:

`Topic: OK1HRA/1/ROT/Azimuth/`

`Message: 54`

Where, from the Setup web interface:

`OK1HRA = Your callsign (up to 20 characters)`

`1 = Rotator ID (1-2 characters)`

If an application subscribes with a topic string of `OK1HRA/1/ROT/#` all topic will be received for the specific device.

## IP-Rotator - Changing the MQTT broker
When changing any of the settings in the web interface related to MQTT you should recycle the power on your IP-Rotator device. This is because the firmware writes these new values to non-volatile storage which is then read at startup. Changing the MQTT broker IP address does not force the router to re-connect to the new broker.

## IP-Rotator Topics and Messages

| Topic string      | Typical Value                   | Description                               | Retained? | via Web Setup? | MQTT Write? |
| ----------------- | ------------------------------- | ----------------------------------------- | --------- | -------------- | ----------- |
| Name              | Tower1                          | Rotator name                              | Yes       | Yes            |             |
| ip                | 192.168.0.201                   | IP address of device                      | Yes       | No             |             |
| mac               | 01:02:03:AA:BB:CC               | MAC address of device                     | Yes       | No             |             |
| CcwRaw            | 3086                            | Raw voltage limit CCW (mV)                | Yes       | No             |             |
| CwRaw             | 209                             | Raw voltage limit CW (mV)                 | Yes       | No             |             |
| StartAzimuth      | 256                             | Start CCW azimuth (0-360)                 | Yes       | Yes            |             |
| AntRadiationAngle | 45                              | Antenna Radiation Angle in degees (1-180) | Yes       | Yes            |             |
| Azimuth           | 55                              | Current azimuth                           | No        | No             |             |
| AzimuthStop       | 55                              |                                           | No        | No             |             |
| AzimuthTarget     | 50                              | Target azimuth                            | No        | No             | W           |
| Motor             | 'AC'\|'DC'                      |                                           | Yes       | Yes            |             |
| PWMenable         | OFF or ON                       | PWM Enable                                | Yes       | Yes            |             |
| PWMdegree         | 10                              | PWM Ramp Start Distance                   | Yes       | Yes            |             |
| PWMRampSteps      | 5                               | PWM ramp length                           | Yes       | Yes            |             |
| PulsePerDegree    | 1-100                           | Pulse count Per Degree (1-100)            | Yes       | Yes            |             |
| EndstopUse        | true\|false                     | Hardware Endstops Installed               | Yes       | Yes            |             |
| Status            | -7 to +15 Note[1]               |                                           | No        | No             |             |
| StatusHuman       | Note[1]                         |                                           | No        | No             |             |
| VoltageValue      | 11.1                            | DC Supply voltage                         | No        | No             |             |
| NoEndStopHighZone | int                             | CCW forbidden zone(tenths of a volt)      | Yes       | Yes            |             |
| NoEndStopLowZone  | int                             | CW forbidden zone (tenths of a volt)      | Yes       | Yes            |             |
| OneTurnLimitSec   | int (20 - 600)                  | Watchdog Speed                            | Yes       | Yes            |             |
| MaxRotateDegree   | 0-719                           | Rotation Range in Degrees                 | Yes       | Yes            |             |
| AZsource          | 'Potentiometer'\|'CW/CCW pulse' | Azimuth Source                            | Yes       | Yes            |             |
| AZpotentiometer   | '2-wire'\|'3-wire'              | Azimuth Potentiometer                     | Yes       | Yes            |             |
| AzimuthTargetPot  |                                 |                                           | No        | No             |             |
| AZpreamp          | 'OFF'\|'ON'                     | Azimuth gain/shift op-amp                 | Yes       | Yes            |             |
| Debug             | Note[2]                         | Debug message from Controller             | No        | No             |             |
| USB-BaudRate      |                                 | USB serial BaudRate                       | Yes       | Yes            |             |
| Target            | Note[3]                         | Target azimuth                            | No        | No             | W           |
| stop              | -                               | Stops rotator                             | No        | No             | W           |
| get               | Azimuth                         | [3]                                       | No        | No             | W           |

Notes:
- Topic string names are case sensitive.
- Note - most topic/values are only published when values are changed via the user web interface. The values cannot be queried via MQTT

[1] Status codes and Status Human values. Note the Status value shared by the MQTT interface is a value of 4 greater than used internally in IP-Rotator firmware.

| Status | StatusHuman |
| ------ | ----------- |
| 1      | PwmDwn-CCW  |
| 2      | CCW         |
| -7     | PwmUp-CCW   |
| 3      | START-CCW   |
| 4      | STOP        |
| 5      | START-CW    |
| 15     | PwmUp-CW    |
| 6      | CW          |
| 7      | PwmDwn-CW   |

[2] e.g. 'Stopped by 120s timeout' , 'Stopped by under voltage 11,5V POE'
[3] Return Azimuth, If rotator status is 0 also return AzimuthStop, StartAzimuth and Name values. 

Note calculating the actual Azimuth requires taking into account the StartAzimuth value. Here's an example of Python code:

`if (StartAzimuth + Azimuth) > 360:`
	`actualAzimuth = (StartAzimuth + Azimuth) - 360`
`else:`
	`actualAzimith = Azimuth + StartAzimuth`

When calculating the Target azimuth a similar approach needs to be taken.
