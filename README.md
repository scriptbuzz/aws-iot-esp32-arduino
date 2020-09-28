# aws-iot-esp32-arduino (draft)

**Overview of Solution**

The ESP32 and ESP8266 microcontrollers by Espressif sit at the heart of many IoT devices in smart home appliances and controllers. In this solution, I will prototype an ESP32-based IoT network that transmits sensor readings with a date/time stamp from an NTP server to the AWS cloud, and accepts commands to turn an LED on/off, over the internet from the AWS cloud. To view how the completed solution functions, please view this video of the final integration: https://youtu.be/6Ir94C9GlVk

The ESP32 is sold under different names and variety of models. For this solution, I have the Adafruit HUZZAH32, based on the popular WROOM32 variant. It has USB-Serial converter, automatic bootloader reset, lipo battery input and charger. But you can use any generic ESP32 development board, many of which can can be purchased for less than $10 from Amazon, as long as it's supported by your firmware development toolchain. 

![esp32](./assets/esp32-devkitc-functional-overview.jpg)

The ESP32 has many built-in sensors. I will be using the built-in hall effect sensor to transmit sensor values as well as the built-in programmable LED wired to GPIO 13 to turn on/off based on remote commands. 

This is Part I of a series of IoT solutions. This solution assumes some knowledge of the Arduino IDE development tool, the ESP32, and the AWS ecosystem.  

![esp32 to aws iot ](./assets/mbx-aws-iot-esp32.jpg)

**Prerequisites**
* AWS Account to create an AWS IoT Thing
* Install the Arduino IDE and USB-serial driver for your computer and ESP32. Consult the ESP32 vendor docs to determmine which driver is needed for your model.
* Attach sensors and/or actuators to your ESP32.  I am using the ESP32's built in hall effect sensor. I am also using the ESP32's built-in programable LED to send commands from the cloud via MQTT to the ESP32 to turn the LED on/off. You can use your own sensors and control devices and attach them to the ESP32.
* Plug your ESP32 microcontroller development board to your workstation. I am using the Adafruit HUZZAH32 but you can use other models so long as you find a compatible board setting in the Arduino IDE Board Manager. 

**AWS Configuration**

* Sign in to the https://console.aws.amazon.com/iot/
* Browse to the AWS IoT Core console
* From the left navigation pane, select **Settings**
* Next to **Custom** endpoint, make a note of the endpoint value. The endpoint should have this format **“xxxxxxxxxxxxxx.iot.us-east-1.amazonaws.com”**, in my case, the AWS region is us-east-1, but feel free to change to your region of choice.  Your microcontroller firmware will be updated with this endpoint value to communicate with AWS IoT. 
* From the left navigation pane, select **Secure > Policies > Create**
* If you do not have any IoT policy created, select **Create a policy**
* Enter a name for your policy. Example: "esp32_iot_policy"
* In the Add statements section, select **Advanced mode**. Copy and paste the following JSON into the policy editor window. This policy is too broad. Please trim down this policy's permissions based on best practices. 
```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:*",
      "Resource": "*"
    }
  ]
}
```
* Choose Create.
* From the left navigation pane, select **Manage > Things > Create > Create a single thing** 
* Enter a name for your Thing and click  **Next**. This name will also be needed for the microcontroller firmware.
* From the **Add a certificate for your thing** panel **Create certificate**
* Download all four certificates: certificate for this thing, the public key, and the private key, and the root CA for AWS IoT. You will need the content of those files for the microcontroller firmware to communicate securely over the internet with AWS IoT. 
* Select **Activate**
* Select **Attach Policy**. Select the policy that you have created from the previous steps. 
* Select **Register Thing**
* Now that you have the IoT certificates, endpoint, Thing Name, you are ready to update the microcontroller firmware. 

**Arduino Development Workstation Configuration**

* Download and install the Arduino IDE on your development workstation
* Install the USB-serial drivers to support your ESP32 microcontroller
* Launch the Arduino IDE and open the Preferences window
* In Additional Board Manager URLs, add https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

<img src="./assets/mbx-arduino-add-board-json-setting.jpg" width="800" title="esp32 arduino board setting">

* Select Tools > Board > Boards Manager. Search esp32 (by Espressif Systems) and install the latest version.
* Select Tools > Board > ESP32 Arduino > ESP32 Dev Module

<img src="./assets/mbx-arduino-add-esp32-board-package.jpg" width="800" title="esp32 arduino board setting">

* Choose Sketch > Include Library > Manage Libraries.
  * Search and install ArduinoJson by Benoit Blanchon
  
  <img src="./assets/mbx-arduino-arduinoJson-library.jpg" width="800" title="esp32 arduino board setting">
 
  * Search and install NTPClient by Fabrice Weinberg
  
  <img src="./assets/mbx-arduino-ntp-client-library.jpg" width="800" title="esp32 arduino board setting">
  
  * Search and install arduino-mqtt by Joel Gaehwiler
  * If for whetever reason you can't find any of the libraries, you can always go to their github repo (see References section below), downlowd the zip file, then select Sketch > Include Library > Add .ZIP file.
  
  <img src="./assets/mbx-arduino-add-zip-library.jpg" width="800" title="esp32 arduino board setting">
  
* Create a new Arduino sketch: File > New
* Paste below code into the blank sketch:
```cpp

// updated: 9/26/2020 by MBX
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

#define LED_PIN 13

WiFiClientSecure iot = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "north-america.pool.ntp.org"); // prep NTP server connectivity


void connectToWiFi() // connect to local wifi network
{
  Serial.println("\nConnecting to Wi-Fi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print("..");
  }

  Serial.println("\nSUCCESS: Connected to Wi-Fi");
  Serial.print("Wi-Fi SSID:  ");
  Serial.println(WIFI_SSID );

}

void connectToAWSIoT() // connect to AWS IoT
{
  Serial.print("\n\nConnecting to AWS IoT");

  iot.setCertificate(AWS_CRT);
  iot.setPrivateKey(AWS_PRIVATE);
  iot.setCACert(AWS_CA);

  client.begin(AWS_ENDPOINT, 8883, iot);
  client.onMessage(messageHandler);

  while (!client.connect(AWS_THINGNAME)) {
    Serial.print(".");
    delay(200);
  }

  if (!client.connected()) {
    Serial.println("\nERROR: AWS IoT Connection Timeout");
    return;
  }

  client.subscribe(MQTT_SUB_TOPIC); // subscribe to MQTT topic

  // display AWS IoT resource names in console
  Serial.println("\nSUCCESS: AWS IoT Endpoint Connected\n");
  Serial.print("\nAWS IoT Publish Topic:  ");
  Serial.println(MQTTT_PUB_TOPIC);
  Serial.print("\nAWS IoT Subscribe Topic:  ");
  Serial.println(MQTT_SUB_TOPIC);
  Serial.print("\nAWS IoT Thing Name:  ");
  Serial.println(AWS_THINGNAME);
  Serial.print("\nAWS IoT Endpoint:  ");
  Serial.println(AWS_ENDPOINT);
  Serial.println("\n");
}


void publishPayload() // format json payload then publish to MQTT topic
{
  StaticJsonDocument<200> doc;
  doc["time"] = timeClient.getEpochTime(); // get current NPT time/date in epoch format
  doc["sensor01"] = hallRead(); // read ESP32 built-in hall effect sensor
  char jsonBuffer[512];

  serializeJson(doc, jsonBuffer);
  Serial.println(jsonBuffer);
  client.publish(MQTTT_PUB_TOPIC, jsonBuffer); // publish to MQTT topic
}

// receive payload from subscribed MQTT topic and test for message ON or OFF to turn LED on/off accordingly
void messageHandler(String &topic, String &payload) {
  Serial.println("\nAWS incoming MQTT topic: " + topic);
  Serial.println("\nAWS incoming MQTT payload: "  + payload);

  if (payload == "ON")  digitalWrite(LED_PIN, HIGH);
  if (payload == "OFF")  digitalWrite(LED_PIN, LOW);
}

// establish WiFi and AWS connectivity
void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);

  delay(2000); // some delay

  connectToWiFi();
  connectToAWSIoT();
  timeClient.begin();

}


void loop() {
  timeClient.update(); // get latest time from NTP server
  publishPayload(); // publish  payload to MQTT topic
  client.loop();
  delay(1000);

}
```
* Create a new Tab in your sketch and name it secrets.h
* Paste the code below into secrets.h
```cpp
#include <pgmspace.h>

// generated by the AWS IoT service
#define AWS_THINGNAME "yourThingName"

// https://docs.aws.amazon.com/iot/latest/developerguide/topics.html
#define MQTTT_PUB_TOPIC "yourPublishTopic"
#define MQTT_SUB_TOPIC "yourSubscribeTopic"

const char WIFI_SSID[] = "YourSSID";
const char WIFI_PASSWORD[] = "YourSSIDpassword";
const char AWS_ENDPOINT[] = "YourAWSIoTEndpoint-ats.iot.us-east-1.amazonaws.com";


// Insert IoT Device Certificate generated by the AWS IoT service
static const char AWS_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
insert your device cert here
-----END CERTIFICATE-----
)KEY";

// Insert IoT Device Private Key generated by the AWS IoT service
static const char AWS_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
insert your device private key here
-----END RSA PRIVATE KEY-----
)KEY";

// Insert Amazon Root CA 1
// https://docs.aws.amazon.com/iot/latest/developerguide/server-authentication.html#server-authentication-certs
  
static const char AWS_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";
```
* Update the Arduino secrets.h code with your WiFi and AWS IoT info you gathered from the previous steps including
  * WiFi ssid/password
  * AWS IoT certificates
  * AWS IoT Endpoint
  * AWS IoT Thing name
* Update the MQTTT_PUB_TOPIC and MQTTT_SUB_TOPIC topics with your own desired topics. 
* Connect your ESP32 development board to your computer
* Upload/Flash your ESP32 with the updated code (firmware)
* Open the Arduino IDE Serial Monitor and set the speed to match your code settings
* From the Serial Monitor output, ensure WiFi and AWS connectivity is established.

**Testing MQTT Connectivity**
* Sign in to the https://console.aws.amazon.com/iot/
* Browse to the AWS IoT Core console
* Open the Test panel, from the left. Select Subscribe to a topic.
* In the Subscription topic field, enter the same topic you have assigned to MQTTT_PUB_TOPIC in your ESP firmware. You will be listening on messages/payloads published by the ESP32. 
* You should see a payload containing a date/time stamp and a sensor reading scrolling below in real time.
* From the left panel, select Publish to a topic. Enter the topic you have assigned to MQTTT_SUB_TOPIC in your ESP32 firmware. This is the topic that the ESP32 controller will be subscribed to listen for messages/payload from AWS IoT. 
* In the black box below, clear everything. Enter ON or OFF (uppercase) then select Publish. You should see the commands showing up in the Arduino IDE serial console. 
* The LED on the ESP32 should turn on or off according to the command you send from AWS IoT.

**Solution Summary**

This solution establishes publish/subscribe connectivity between an ESP32 microcontroller and AWS IoT to read sensor values from the ESP32 and to send commands to the ESP32.  With this foundational solution, you can now apply IoT Rules to store sensor readings, take action on sensor values, visualize sensor data, perform advances analytics, feed machine learning models, and more. 

**References**
* ESP32 https://www.espressif.com/en/products/socs/esp32
* Arduino IDE https://www.arduino.cc/en/main/software
* Arduino Packages and Libraries
  * ArduinoJSON: Builds JSON payloads https://github.com/arduino-libraries/Arduino_JSON
  * ArduinoMQTT: Lightweight MQTT library for the Arduino IDE https://github.com/256dpi/arduino-mqtt
  * NTPClient: Connect to an NTP server to obtain real time readings https://github.com/arduino-libraries/NTPClient
* AWS IoT https://aws.amazon.com/iot/
* Moheeb Zara's AWS IoT guide https://aws.amazon.com/blogs/compute/building-an-aws-iot-core-device-using-aws-serverless-and-an-esp32/
* Gourav Das's Cloud Home Automation guide https://hackernoon.com/cloud-home-automation-series-part-1-connect-esp32-to-aws-iot-with-arduino-code-lkhp36vx
* Mudassar Tamboli's guide https://medium.com/@mudassar.tamboli/esp32-aws-iot-and-secured-wifi-leds-e9dc9d2d70b3
* Adafruit HUZZAH32 https://www.adafruit.com/product/3405
* SparkFun ESP32 Thing https://www.sparkfun.com/products/13907


