// src/knx_ip_module.cpp
#include "knx_ip_module.h"

KNXIPModule::KNXIPModule() : physicalAddress(0) {}

bool KNXIPModule::begin(const IPAddress& gatewayIP, int knxArea, int knxLine, int knxMember) {
    this->gatewayIP = gatewayIP;
    this->knxArea = knxArea;
    this->knxLine = knxLine;
    this->knxMember = knxMember;

    physicalAddress = (knxArea << 12) | (knxLine << 8) | knxMember;

    if (udp.listen(3671)) { // KNXnet/IP port
        udp.onPacket([this](AsyncUDPPacket packet) {
            this->processUdpData(packet);
        });
        Serial.print("KNX UDP listener started on port 3671, gateway IP: ");
        Serial.println(gatewayIP);
        return true;
    } else {
        Serial.println("Failed to start KNX UDP listener");
        return false;
    }
}

bool KNXIPModule::sendKNXMessage(int groupAddress, const uint8_t* data, size_t dataLength) {
    uint8_t buffer[64]; // Adjust buffer size as needed
    size_t bufferLength = 0; // Use size_t for consistency

    // KNXnet/IP tunneling request
    buffer[bufferLength++] = 0x06; // Header length
    buffer[bufferLength++] = 0x10; // Protocol version
    buffer[bufferLength++] = 0x02; // Service type (tunneling request)
    buffer[bufferLength++] = 0x00; // Reserved
    buffer[bufferLength++] = 0x08; // Total length high byte
    buffer[bufferLength++] = 0x38; // Total length low byte

    // Tunneling request
    buffer[bufferLength++] = 0x04; // Connection type (tunnel)
    buffer[bufferLength++] = 0x00; // Reserved
    buffer[bufferLength++] = 0x00; // Connection ID
    buffer[bufferLength++] = 0x00; // Sequence counter

    // KNX/IP data
    buffer[bufferLength++] = 0x29; // Control field
    buffer[bufferLength++] = (physicalAddress >> 8) & 0xFF; // Source address high byte
    buffer[bufferLength++] = physicalAddress & 0xFF; // Source address low byte
    buffer[bufferLength++] = (groupAddress >> 8) & 0xFF; // Destination address high byte
    buffer[bufferLength++] = groupAddress & 0xFF; // Destination address low byte
    buffer[bufferLength++] = dataLength; // APDU length

    // Copy data
    memcpy(buffer + bufferLength, data, dataLength);
    bufferLength += dataLength;

    // Use correct types for broadcastTo
    if (udp.broadcastTo(buffer, bufferLength, 3671, TCPIP_ADAPTER_IF_STA)) {
        Serial.println("KNX message sent (awaiting acknowledgment)");
    } else {
        Serial.println("Failed to send KNX message");
    }

    return true;
}

void KNXIPModule::processUdpData(AsyncUDPPacket packet) {
    Serial.print("UDP Packet received from: ");
    Serial.print(packet.remoteIP());
    Serial.print(":");
    Serial.print(packet.remotePort());
    Serial.print(", length: ");
    Serial.print(packet.length());
    Serial.println(" bytes");

    // Print the received data in hexadecimal format
    Serial.print("Data: ");
    for (size_t i = 0; i < packet.length(); i++) { // Use size_t for consistency
        Serial.printf("%02X ", packet.data()[i]);
    }
    Serial.println();

    // Check for acknowledgment
    if (packet.length() >= 6 && packet.data()[0] == 0x06 && packet.data()[2] == 0x04) { // Check for ack header
        Serial.println("Acknowledgment received");
    } else {
        Serial.println("Non-acknowledgment message received");
        // ... (process other message types if needed) ...
    }
}