
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
