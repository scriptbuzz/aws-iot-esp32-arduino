# aws-iot-esp32-arduino

**Overview of Solution**

The ESP32 and ESP8266 microcontrollers by Espressif sit at the heart of many IoT devices in smart home appliances and controllers. In this solution, I will prototype an ESP32-based IoT network that transmits sensor readings to the AWS cloud and accepts commands over the internet from the AWS cloud. 

The ESP32 comes in many flavors and models. For this solution, I have the HUZZAH 32, which is an ESP32-WROOM-32 module from Adafruit. It has USB-to-Serial converter, automatic bootloader reset, lipo battery input and charger. But you can use generic ESP32 development boards from Amazon for about $10. 

The ESP32 has many built in sensors. I will be using the built-in hall effect sensor as well as the built-in programmable LED wired to GPIO 13. This solution assumes some knowledge of the Arduino IDE development tool and the ESP32. 

**Prerequisites**
* AWS Account
* Create an AWS IoT Thing
* Install the Arduino IDE and USB drivers
* ESP32 microcontroller development board. I am using the Adafruit HUZZAH32 but you can use other models so long as you find a compatible board setting in the Arduino IDE Board Manager. 
* Sensors and/or actuators, if you wish send messages and commands back and forth between AWS IoT and the ESP32 to read sensor values and control pins on the ESP32 via MQTT. I am using the ESP32's built in hall effects sensor to pass those readings to AWS IoT. I am also using the ESP32's built in LED to send commands from the cloud via MQTT to the ESP32. You can use your own sensors and control devices and attach then to the ESP32.
* ESP32 firmware to be updated then uploaded/flashed
  * The C++ firmware code file: https://github.com/scriptbuzz/aws-iot-esp32-arduino/blob/master/mbx-iot-esp32.ino
  * The associated secrets.h file: https://github.com/scriptbuzz/aws-iot-esp32-arduino/blob/master/secrets.h

**AWS Configuration**

* Sign in to the https://console.aws.amazon.com/iot/
* Browse to the AWS IoT Core console
* From the left navigation pane, select Settings
* In Custom endpoint, make a note of the endpoint value. The endpoint should have this format “xxxxxxxxxxxxxx.iot.us-east-1.amazonaws.com”, in my case, the AWS region is us-east-1, but feel free to change to your region of choice.  Your microcontroller will need this endpoint to communicate with AWS IoT. 
* From the left navigation pane, select Secure > Policies > Create.
* If you do not have any IoT policy created, select Create a policy.
* Enter a name for your policy. Example: "esp32_iot_policy"
* In the Add statements section, select Advanced mode. Copy and paste the following JSON into the policy editor window. This policy is too broad. Please spend some time to trim down this policy based on best practices. 
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
* From the left navigation pane, select Manage > Things > Create > Create a single thing. 
* Enter a name for your Thing > Next. You will need this name for your microcontroller firmware.
* From the Add a certificate for your thing panel > Create certificate.
* Download all four certificates: certificate for this thing, the public key, and the private key, and the root CA for AWS IoT. You will need the content of those files for the microcontroller to communicate securely over the internet with AWS IoT. 
* Select Activate
* Select Attach Policy. Select the policy that you have created from the previous steps. 
* Select Register Thing.
* Now that you have the IoT certificates, endpoint, Thing Name, you are ready to update the microcontroller firmware. 


**Arduino Development Workstation Configuration**
* Download and install the Arduino IDE on your development workstation.
* Install the USB-serial drivers to support your ESP32 microcontroller.
* Launch the Arduino IDE and open the Preferences window.
* In Additional Board Manager URLs, add https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
* Select Tools > Board > Boards Manager. Search esp32 (by Espressif Systems) and install the latest version.
* Select Tools > Board > ESP32 Arduino > ESP32 Dev Module
* Choose Sketch > Include Library > Manage Libraries.
  * Search and install ArduinoJson by Benoit Blanchon
  * Search and install NTPClient by Fabrice Weinberg
  * Search and install arduino-mqtt by Joel Gaehwiler
  * If for whetever reason you can't find any of the libraries, you can always go to their github repo (see References section below), downlowd the zip file, then select Sketch > Include Library > Add .ZIP file.

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

