#ifndef KNX_IP_MODULE_H
#define KNX_IP_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>

class KNXIPModule {
public:
    KNXIPModule(); // Constructor
    bool begin(const IPAddress& gatewayIP, int knxArea, int knxLine, int knxMember);
    bool sendKNXMessage(int groupAddress, const uint8_t* data, size_t dataLength);
    // Removed the loop() method to keep the module importable and decoupled.
private:
    AsyncUDP udp;
    IPAddress gatewayIP;
    int knxArea;
    int knxLine;
    int knxMember;
    uint16_t physicalAddress;
    void processUdpData(AsyncUDPPacket packet);
};

#endif // KNX_IP_MODULE_H
