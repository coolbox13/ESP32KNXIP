//==== src/knx_ip_core.cpp ====

#include "knx_ip_core.h"

KNXIPCore::KNXIPCore() {}

uint16_t KNXIPCore::assembleGroupAddress(uint8_t mainGroup, uint8_t midGroup, uint8_t subGroup) {
    // KNX group address format: AAAA ABBB CCCC CCCC
    // A = main group (0-31), B = middle group (0-7), C = sub group (0-255)
    return ((mainGroup & 0x1F) << 11) | ((midGroup & 0x07) << 8) | (subGroup & 0xFF);
}

uint16_t KNXIPCore::assemblePhysicalAddress(uint8_t area, uint8_t line, uint8_t device) {
    // KNX physical address format: AAAA LLLL DDDD DDDD
    // A = area (0-15), L = line (0-15), D = device (0-255)
    return ((area & 0x0F) << 12) | ((line & 0x0F) << 8) | (device & 0xFF);
}

void KNXIPCore::parseGroupAddress(uint16_t groupAddress, uint8_t& mainGroup, uint8_t& midGroup, uint8_t& subGroup) {
    mainGroup = (groupAddress >> 11) & 0x1F;
    midGroup = (groupAddress >> 8) & 0x07;
    subGroup = groupAddress & 0xFF;
}

void KNXIPCore::parsePhysicalAddress(uint16_t physicalAddress, uint8_t& area, uint8_t& line, uint8_t& device) {
    area = (physicalAddress >> 12) & 0x0F;
    line = (physicalAddress >> 8) & 0x0F;
    device = physicalAddress & 0xFF;
}

std::vector<uint8_t> KNXIPCore::createRoutingIndication(uint16_t sourceAddr, uint16_t targetAddr, 
                                                        bool isGroupAddress, const std::vector<uint8_t>& apdu) {
    std::vector<uint8_t> packet;
    
    // KNXnet/IP header
    packet.push_back(0x06);                      // Header length
    packet.push_back(0x10);                      // Protocol version 1.0
    packet.push_back(KNX_ST_ROUTING_INDICATION >> 8);   // Service type high byte
    packet.push_back(KNX_ST_ROUTING_INDICATION & 0xFF); // Service type low byte
    
    // Total length placeholder (will be filled later)
    packet.push_back(0x00);                      
    packet.push_back(0x00);                      
    
    // cEMI frame
    packet.push_back(KNX_CEMI_L_DATA_IND);       // Message code
    packet.push_back(0x00);                      // Additional info length (0)
    
    // Control field 1 - Frame type, repeat, broadcast, priority, ack, confirm
    packet.push_back(0xBC);  // Standard frame, no repeat, normal priority
    
    // Control field 2 - Destination address type, hop count, extended frame format
    uint8_t ctrl2 = 0x60;    // Hop count = 6, standard frame
    if (isGroupAddress) {
        ctrl2 |= 0x80;       // Set high bit for group address
    }
    packet.push_back(ctrl2);
    
    // Source address
    packet.push_back((sourceAddr >> 8) & 0xFF);
    packet.push_back(sourceAddr & 0xFF);
    
    // Destination address
    packet.push_back((targetAddr >> 8) & 0xFF);
    packet.push_back(targetAddr & 0xFF);
    
    // APDU length
    packet.push_back(apdu.size() + 1);           // +1 for TPCI/APCI
    
    // Append APDU
    for (size_t i = 0; i < apdu.size(); i++) {
        packet.push_back(apdu[i]);
    }
    
    // Fill in total length
    uint16_t totalLength = packet.size();
    packet[4] = (totalLength >> 8) & 0xFF;
    packet[5] = totalLength & 0xFF;
    
    return packet;
}

std::vector<uint8_t> KNXIPCore::createTunnelingRequest(uint8_t channelId, uint8_t seqNumber, 
                                                       uint16_t sourceAddr, uint16_t targetAddr, 
                                                       bool isGroupAddress, const std::vector<uint8_t>& apdu) {
    std::vector<uint8_t> packet;
    
    // KNXnet/IP header
    packet.push_back(0x06);                      // Header length
    packet.push_back(0x10);                      // Protocol version 1.0
    packet.push_back(KNX_ST_TUNNELING_REQUEST >> 8);   // Service type high byte
    packet.push_back(KNX_ST_TUNNELING_REQUEST & 0xFF); // Service type low byte
    
    // Total length placeholder (will be filled later)
    packet.push_back(0x00);                      
    packet.push_back(0x00);                      
    
    // Tunneling header
    packet.push_back(0x04);                      // Structure length
    packet.push_back(channelId);                 // Communication channel ID
    packet.push_back(seqNumber);                 // Sequence counter
    packet.push_back(0x00);                      // Reserved
    
    // cEMI frame
    packet.push_back(KNX_CEMI_L_DATA_REQ);       // Message code
    packet.push_back(0x00);                      // Additional info length (0)
    
    // Control field 1 - Frame type, repeat, broadcast, priority, ack, confirm
    packet.push_back(0xBC);  // Standard frame, no repeat, normal priority
    
    // Control field 2 - Destination address type, hop count, extended frame format
    uint8_t ctrl2 = 0x60;    // Hop count = 6, standard frame
    if (isGroupAddress) {
        ctrl2 |= 0x80;       // Set high bit for group address
    }
    packet.push_back(ctrl2);
    
    // Source address
    packet.push_back((sourceAddr >> 8) & 0xFF);
    packet.push_back(sourceAddr & 0xFF);
    
    // Destination address
    packet.push_back((targetAddr >> 8) & 0xFF);
    packet.push_back(targetAddr & 0xFF);
    
    // APDU length
    packet.push_back(apdu.size() + 1);           // +1 for TPCI/APCI
    
    // Append APDU
    for (size_t i = 0; i < apdu.size(); i++) {
        packet.push_back(apdu[i]);
    }
    
    // Fill in total length
    uint16_t totalLength = packet.size();
    packet[4] = (totalLength >> 8) & 0xFF;
    packet[5] = totalLength & 0xFF;
    
    return packet;
}

void KNXIPCore::logBuffer(const char* prefix, const uint8_t* buffer, size_t length, uint8_t debugLevel) {
    if (debugLevel > 0) {
        Serial.print(prefix);
        for (size_t i = 0; i < length; i++) {
            Serial.printf("%02X ", buffer[i]);
        }
        Serial.println();
    }
}