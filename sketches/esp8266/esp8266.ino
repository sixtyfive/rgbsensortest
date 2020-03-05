#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#ifndef STASSID
#define STASSID "Zuhause"
#define STAPSK  "endlichwiederdrin"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

#include "EspMQTTClient.h"

EspMQTTClient mqttclient(
  "10.0.0.1",
  1883,
  "rgbsensortest"
);

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    /* do things before rebooting */
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("rgbsensortest");
  ArduinoOTA.setPassword("admin");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem"; // U_FS
    }
    /* unmount here if needed */
  });
  ArduinoOTA.onEnd([]() {
    /* ... */
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    /* ... */
  });
  ArduinoOTA.onError([](ota_error_t error) {
    /* ... */
  });
  ArduinoOTA.begin();
}

void onConnectionEstablished()
{
  mqttclient.publish("RGBSensorTest/status", "online");
}

void setupMQTT() {
  mqttclient.enableLastWillMessage("RGBSensorTest/status", "offline");
}

void setup() {
  Serial.begin(9600);
  setupWiFi();
  setupMQTT();
}

String values;

void readSerialWriteMQTT() {
  values = Serial.readString();
  values.trim();
  if (values != NULL)
    mqttclient.publish("RGBSensorTest/values", values);
}

void loop() {
  ArduinoOTA.handle();
  mqttclient.loop();
  readSerialWriteMQTT();
}
