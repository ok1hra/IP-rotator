
## Introduction to MQTT
MQTT is a lightweight 'publish/subscribe' Internet protocol designed to interconnect devices with resource constraints or limited network bandwidth. A device can publish a message to a topic, one or more applications can subscribe to that topic and receive the message. MQTT is widely used to interconnect devices on the Internet. MQTT functionality is built into the IP-Rotator firmware allowing a user to develop their own software applications to control and read status of the IP-Rotator device.

MQTT includes two concepts we see implemented in the IP-Rotator firmware: QoS - Quality of Service, and 'Last Retained Message'. QoS provides 3 levels of guarantee a message will be delivered. If a message is published to a topic with the retained message flag set then any client subsequently subscribing to that topic will receive the the retained message. An MQTT client application runs on the device and connects to a server - known in MQTT terms as a broker. Many clients can connect to a single broker.

MQTT on IP-Rotator works in parallel to the excellent web interface, you can safely ignore MQTT and use IP-Rotator in this manner. However if you are interested in controlling your rotator from another application then MQTT should be of interest, why? - because it allows you to develop your own application to control one or many IP-Rotators. This could be through software developed, say, in Python, or via a simple web interface such as Node-Red. The route you choose is up to you, what we will do now show you is how to setup a MQTT broker to support IP-Rotator, and explain the topic and message structure.

### Installing a MQTT Broker
You can of course leave the IP address of the MQTT broker setting to the default value set the Web config screen. This defaults to a broker hosted by RemoteQTH.com

For many reasons you may choose to install your own MQTT broker. There are several technical options and personal preferences as to how to proceed. We'll describe a simple solution based on a Raspberry Pi running Linux upon which we will install a de-facto industry standard MQTT broker. We will assume you are comfortable enough to install Linux on a Raspberry Pi and are familiar with installing applications via the command line instruction 'apt-get'. For simplicity the following describes a MQTT Broker installed on your own private LAN. Please don't deploy this configuration to an Internet facing device - that is an advanced topic beyond the scope of this guidance.

`sudo apt-get update && sudo apt upgrade`
`sudo apt-get install -y mosquitto mosquitto-clients`

To make Mosquitto auto start with the Raspberry Pi boots run the following command:

`sudo systemctl enable mosquitto.service`

Create and edit the following Mosquitto configuration file:

`sudo nano /etc/mosquitto/conf.d/iprotator.conf`

add these lines to this file. This will provide the basic format to support the IP-Rotator device.

`listener 1883`  
`allow_anonymous true`  
`listener 1884`  
`protocol websockets`  
`allow_anonymous true`  

`# Global variables follow`  
`persistence true`  
`connection_messages true`  
`log_timestamp true`  
`log_timestamp_format %Y-%m-%dT%H:%M:%S`  

The `listener` value defined the port that the MQTT broker uses. Note the default for MQTT is port 1883. The IP-Rotator firmware requires that port 1884 is accessible using the websockets protocol. This supports the MQTT web interface accessible via the device GUI.

The IP-Rotator firmware does not currently support device access authentication

Now restart the MQTT broker (or reboot).

`sudo systemctl restart mosquitto.service`

You should also set the IP address of your MQTT Broker and port (1883) via the IP-Rotator Web GUI (config page).