// Example usage in main.cpp

#include <Arduino.h>
#include <WiFi.h>
#include "knx_ip_module.h"

const char* ssid = "coolbox_down";
const char* password = "1313131313131";
IPAddress knxGateway(192, 168, 178, 2); // Replace with your KNX gateway IP

KNXIPModule knxModule;

// Callback function for temperature group address
void onTemperatureUpdate(const KNXTelegram& telegram) {
    // Process the telegram data
    if (telegram.data.size() >= 2) {
        // Convert the received data to a temperature value
        float temperature = KNXIPModule::decodeDPT9(telegram.data.data(), telegram.data.size());
        Serial.print("Temperature updated: ");
        Serial.println(temperature);
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Set debug level (0=minimal, 1=normal, 2=verbose)
    knxModule.setDebugLevel(2);

    // Choose mode based on needs:
    // For debug/monitoring:
    if (knxModule.beginMulticast(1, 1, 0)) {
       Serial.println("KNX multicast mode started");
    // }
    
    // For normal operation:
    //if (knxModule.begin(knxGateway, 1, 1, 0)) {
    //    Serial.println("KNX module started");
        
        // Register callback for group address 3/2/1 (temperature)
        int temperatureGroupAddr = (3 << 11) | (2 << 8) | 1;
        knxModule.onGroupAddress(temperatureGroupAddr, onTemperatureUpdate);
    } else {
        Serial.println("KNX module failed to start");
    }
}

void loop() {
    static unsigned long lastSendTime = 0;
    if (millis() - lastSendTime > 5000) { // Send every 5 seconds
        lastSendTime = millis();
        
        // Example: Send temperature value (22.5°C) to group address 1/0/1
        int groupAddr = (1 << 11) | (0 << 8) | 1;
        knxModule.sendTemperature(groupAddr, 22.5);
        
        // Example: Send boolean value (ON) to group address 2/1/5
        groupAddr = (2 << 11) | (1 << 8) | 5;
        knxModule.sendBool(groupAddr, true);
    }
    
    delay(10);
}