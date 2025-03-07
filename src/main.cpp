// src/main.cpp
#include <Arduino.h>
#include <WiFi.h>
#include "knx_ip_module.h"

const char* ssid = "coolbox_down";
const char* password = "password";
IPAddress knxGateway(192, 168, 178, 2); // Replace with your KNX gateway IP

KNXIPModule knxModule;

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    if (knxModule.begin(knxGateway, 1, 1, 0)) {
        Serial.println("KNX module started");
    } else {
        Serial.println("KNX module failed to start");
    }
}

void loop() {

    static unsigned long lastSendTime = 0;
    if (millis() - lastSendTime > 5000) { // Send every 5 seconds
        lastSendTime = millis();
        uint8_t data[] = {0x81}; // Declare data as an array to pass a pointer.
        knxModule.sendKNXMessage(1, data, sizeof(data)); // Send to group address 1
    }
    delay(10);
}