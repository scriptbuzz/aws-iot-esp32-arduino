# aws-iot-esp32-arduino

**Development Environment Setting**

NOTE: Update the program global variables to reflect your AWS resource names. 

**Prerequisite**
* AWS Account
* WiFi connection SSID and password. 
* Arduino IDE and USB drivers
* USB cable to connect the ESP32 to your development computer.   
* ESP32 microcontroller development board. I am using the Adafruit HUZZAH32 but you can use other models so long as you find a compatible board setting in the Arduino IDE Board Manager. 
* Sensors and/or actuators, if you wish send messages and commands back and forth between AWS IoT and the ESP32 to read sensor values and control pins on the ESP32 via MQTT. I am using the ESP32's built in hall effects sensor to pass those readings to AWS IoT. I am also using the ESP32's built in LED to send commands from the cloud via MQTT to the ESP32. 
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


**Microcontroller Development Workstation Configuration**
* Install the Arduino IDE on your development workstation. 
* Install USB drivers to support your ESP32 microcontroller.
* From the Board Manager, Update Arduino IDE Boards to include ESP32
* Update Arduino IDE Libraries to support ESP32 
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
