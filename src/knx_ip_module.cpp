//==== src/knx_ip_module.cpp ====

#include "knx_ip_module.h"

KNXIPModule::KNXIPModule() 
    : physicalAddress(0), 
      connectionType(KNX_CONNECTION_UNICAST),
      debugLevel(1) {}

bool KNXIPModule::begin(const IPAddress& gatewayIP, int knxArea, int knxLine, int knxMember) {
    this->gatewayIP = gatewayIP;
    this->knxArea = knxArea;
    this->knxLine = knxLine;
    this->knxMember = knxMember;
    this->connectionType = KNX_CONNECTION_UNICAST;

    physicalAddress = (knxArea << 12) | (knxLine << 8) | knxMember;

    if (udp.listen(KNX_PORT)) {
        udp.onPacket([this](AsyncUDPPacket packet) {
            this->processUdpData(packet);
        });
        
        if (debugLevel > 0) {
            Serial.print("KNX UDP listener started on port ");
            Serial.print(KNX_PORT);
            Serial.print(", gateway IP: ");
            Serial.println(gatewayIP);
        }
        return true;
    } else {
        if (debugLevel > 0) {
            Serial.println("Failed to start KNX UDP listener");
        }
        return false;
    }
}

bool KNXIPModule::beginMulticast(int knxArea, int knxLine, int knxMember) {
    this->knxArea = knxArea;
    this->knxLine = knxLine;
    this->knxMember = knxMember;
    this->connectionType = KNX_CONNECTION_MULTICAST;

    physicalAddress = (knxArea << 12) | (knxLine << 8) | knxMember;

    if (udp.listenMulticast(KNX_MULTICAST_IP, KNX_PORT)) {
        udp.onPacket([this](AsyncUDPPacket packet) {
            this->processUdpData(packet);
        });
        
        if (debugLevel > 0) {
            Serial.print("KNX Multicast listener started on ");
            Serial.print(KNX_MULTICAST_IP);
            Serial.print(":");
            Serial.println(KNX_PORT);
        }
        return true;
    } else {
        if (debugLevel > 0) {
            Serial.println("Failed to start KNX Multicast listener");
        }
        return false;
    }
}

void KNXIPModule::setDebugLevel(uint8_t level) {
    debugLevel = level;
}

bool KNXIPModule::sendKNXMessage(int groupAddress, const uint8_t* data, size_t dataLength) {
    uint8_t buffer[64]; // Adjust buffer size as needed
    size_t bufferLength = 0;

    if (connectionType == KNX_CONNECTION_UNICAST) {
        // KNXnet/IP tunneling request for unicast
        buffer[bufferLength++] = 0x06; // Header length
        buffer[bufferLength++] = 0x10; // Protocol version
        buffer[bufferLength++] = 0x02; // Service type (tunneling request)
        buffer[bufferLength++] = 0x00; // Reserved
        
        // Total length (will be filled later)
        buffer[bufferLength++] = 0x00; // Total length high byte
        buffer[bufferLength++] = 0x00; // Total length low byte

        // Tunneling request
        buffer[bufferLength++] = 0x04; // Connection type (tunnel)
        buffer[bufferLength++] = 0x00; // Reserved
        buffer[bufferLength++] = 0x00; // Connection ID
        buffer[bufferLength++] = 0x00; // Sequence counter

        // KNX cEMI data (starts here)
        buffer[bufferLength++] = 0x29; // Message code
        buffer[bufferLength++] = 0x00; // Additional info length (0)
        
        buffer[bufferLength++] = 0x80 | 0x00; // Control field 1 - Group address + no repeat
        buffer[bufferLength++] = 0x00; // Control field 2 - Priority & routing counter
        
        buffer[bufferLength++] = (physicalAddress >> 8) & 0xFF; // Source address high byte
        buffer[bufferLength++] = physicalAddress & 0xFF; // Source address low byte
        buffer[bufferLength++] = (groupAddress >> 8) & 0xFF; // Destination address high byte
        buffer[bufferLength++] = groupAddress & 0xFF; // Destination address low byte
        
        buffer[bufferLength++] = dataLength + 1; // TPDU length (data + 1 for transport control)
        buffer[bufferLength++] = 0x00; // TPCI/APCI (group value write)
        
        // Copy data
        memcpy(buffer + bufferLength, data, dataLength);
        bufferLength += dataLength;
        
        // Fill in total length
        uint16_t totalLength = bufferLength;
        buffer[4] = (totalLength >> 8) & 0xFF;
        buffer[5] = totalLength & 0xFF;
    }
    else {
        // KNXnet/IP routing request for multicast
        buffer[bufferLength++] = 0x06; // Header length
        buffer[bufferLength++] = 0x10; // Protocol version
        buffer[bufferLength++] = 0x05; // Service type (routing indication)
        buffer[bufferLength++] = 0x00; // Reserved
        
        // Total length (will be filled later)
        buffer[bufferLength++] = 0x00; // Total length high byte
        buffer[bufferLength++] = 0x00; // Total length low byte

        // KNX cEMI data (starts here)
        buffer[bufferLength++] = 0x29; // Message code (L_Data.req)
        buffer[bufferLength++] = 0x00; // Additional info length (0)
        
        buffer[bufferLength++] = 0x80 | 0x00; // Control field 1 - Group address + no repeat
        buffer[bufferLength++] = 0x00; // Control field 2 - Priority & routing counter
        
        buffer[bufferLength++] = (physicalAddress >> 8) & 0xFF; // Source address high byte
        buffer[bufferLength++] = physicalAddress & 0xFF; // Source address low byte
        buffer[bufferLength++] = (groupAddress >> 8) & 0xFF; // Destination address high byte
        buffer[bufferLength++] = groupAddress & 0xFF; // Destination address low byte
        
        buffer[bufferLength++] = dataLength + 1; // TPDU length (data + 1 for transport control)
        buffer[bufferLength++] = 0x00; // TPCI/APCI (group value write)
        
        // Copy data
        memcpy(buffer + bufferLength, data, dataLength);
        bufferLength += dataLength;
        
        // Fill in total length
        uint16_t totalLength = bufferLength;
        buffer[4] = (totalLength >> 8) & 0xFF;
        buffer[5] = totalLength & 0xFF;
    }

    bool success = false;
    if (connectionType == KNX_CONNECTION_UNICAST) {
        success = udp.writeTo(buffer, bufferLength, gatewayIP, KNX_PORT);
    } else {
        success = udp.writeTo(buffer, bufferLength, KNX_MULTICAST_IP, KNX_PORT);
    }

    if (success) {
        if (debugLevel > 0) {
            int offset = (connectionType == KNX_CONNECTION_UNICAST) ? 10 : 6;
            KNXTelegram telegram = parseTelegram(&buffer[offset], bufferLength - offset);
            logTelegram(telegram, true);
        }
    } else if (debugLevel > 0) {
        Serial.println("Failed to send KNX message");
    }

    return success;
}

bool KNXIPModule::sendBool(int groupAddress, bool value) {
    std::vector<uint8_t> data = encodeDPT1(value);
    return sendKNXMessage(groupAddress, data.data(), data.size());
}

bool KNXIPModule::sendPercentage(int groupAddress, uint8_t percentage) {
    std::vector<uint8_t> data = encodeDPT5(percentage);
    return sendKNXMessage(groupAddress, data.data(), data.size());
}

bool KNXIPModule::sendTemperature(int groupAddress, float temperature) {
    std::vector<uint8_t> data = encodeDPT9(temperature);
    return sendKNXMessage(groupAddress, data.data(), data.size());
}

void KNXIPModule::onGroupAddress(int groupAddress, KNXGroupAddressCallback callback) {
    callbacks[groupAddress].push_back(callback);
}

void KNXIPModule::removeCallback(int groupAddress) {
    callbacks.erase(groupAddress);
}

void KNXIPModule::processUdpData(AsyncUDPPacket packet) {
    if (debugLevel > 1) {
        Serial.print("UDP Packet received from: ");
        Serial.print(packet.remoteIP());
        Serial.print(":");
        Serial.print(packet.remotePort());
        Serial.print(", length: ");
        Serial.print(packet.length());
        Serial.println(" bytes");
    }

    if (debugLevel > 1) {
        Serial.print("Raw data: ");
        for (size_t i = 0; i < packet.length(); i++) {
            Serial.printf("%02X ", packet.data()[i]);
        }
        Serial.println();
    }

    // Check if it's a KNX packet
    if (packet.length() < 6) return;
    
    // Check for acknowledgment
    if (packet.data()[0] == 0x06 && packet.data()[2] == 0x04) {
        if (debugLevel > 0) {
            Serial.println("Acknowledgment received");
        }
        return;
    }

    // Service type is at byte 2
    uint8_t serviceType = packet.data()[2];
    
    // Check packet type
    if (packet.data()[0] == 0x06 && serviceType == 0x02) {
        // Tunneling request (typical KNX telegram)
        const uint8_t* knxData = packet.data() + 10; // Skip KNXnet/IP header and tunneling header
        KNXTelegram telegram = parseTelegram(knxData, packet.length() - 10);
        
        if (debugLevel > 0) {
            logTelegram(telegram, false);
        }
        
        notifyCallbacks(telegram);
    }
    else if (packet.data()[0] == 0x06 && serviceType == 0x05) {
        // KNXnet/IP Routing packet (common in multicast)
        // Header is 6 bytes, then cEMI data starts
        const uint8_t* knxData = packet.data() + 6; 
        KNXTelegram telegram = parseTelegram(knxData, packet.length() - 6);
        
        if (debugLevel > 0) {
            logTelegram(telegram, false);
        }
        
        notifyCallbacks(telegram);
    }
    else if (packet.data()[0] == 0x06 && serviceType == 0x15) {
        // Core services packet (search request/response, description, etc.)
        if (debugLevel > 0) {
            Serial.printf("<<< KNX Core Service packet (0x%02X 0x%02X)\n", 
                packet.data()[2], packet.data()[3]);
        }
    }
    else if (debugLevel > 1) {
        Serial.printf("<<< Unknown KNX packet, service type: 0x%02X\n", serviceType);
    }
}

KNXTelegram KNXIPModule::parseTelegram(const uint8_t* data, size_t length) {
    KNXTelegram telegram;
    
    if (length < 8) return telegram; // Not enough data for a valid telegram
    
    // Check if this is cEMI format with message code and additional info length
    int offset = 0;
    if (data[0] == 0x29) {  // L_Data.req message code
        uint8_t addInfoLen = data[1];
        offset = 2 + addInfoLen;  // Skip message code, addInfoLen, and any additional info
    }
    
    // Not enough remaining data
    if (length < offset + 6) return telegram;
    
    // Extract control fields
    uint8_t ctrl1 = data[offset];
    uint8_t ctrl2 = data[offset + 1];
    
    // Get routing counter from control field 2
    telegram.routingCounter = (ctrl2 & 0x70) >> 4;
    
    // Extract source address
    telegram.sourceAddress = (data[offset + 2] << 8) | data[offset + 3];
    
    // Extract target address and address type
    telegram.targetAddress = (data[offset + 4] << 8) | data[offset + 5];
    telegram.isGroupAddress = (ctrl1 & 0x80) != 0;
    
    // Extract command and data
    uint8_t tpduLength = data[offset + 6];
    if (tpduLength > 0 && length >= offset + 7 + tpduLength - 1) {
        // The command is in the APCI (first 6 bits of APCI which is across 2 bytes)
        uint8_t tpci = data[offset + 7];
        uint8_t apci = 0;
        
        if (length >= offset + 8) {
            apci = data[offset + 8];
        }
        
        // Extract command from APCI
        telegram.command = ((tpci & 0x03) << 2) | ((apci & 0xC0) >> 6);
        
        // Extract data payload (skipping TPCI/APCI)
        for (size_t i = offset + 8; i < offset + 7 + tpduLength; i++) {
            // First byte of data may contain APCI bits
            if (i == offset + 8) {
                telegram.data.push_back(apci & 0x3F);  // Mask out APCI bits
            } else {
                telegram.data.push_back(data[i]);
            }
        }
    }
    
    return telegram;
}

void KNXIPModule::notifyCallbacks(const KNXTelegram& telegram) {
    if (!telegram.isGroupAddress) return; // Only process group addresses
    
    // Find callbacks for this group address
    auto it = callbacks.find(telegram.targetAddress);
    if (it != callbacks.end()) {
        // Call all registered callbacks
        for (const auto& callback : it->second) {
            callback(telegram);
        }
    }
}

void KNXIPModule::logTelegram(const KNXTelegram& telegram, bool outgoing) {
    if (debugLevel < 1) return;
    
    Serial.print(outgoing ? ">>> " : "<<< ");
    Serial.print("KNX telegram: src=");
    Serial.printf("%d.%d.%d", 
        (telegram.sourceAddress >> 12) & 0x0F,
        (telegram.sourceAddress >> 8) & 0x0F,
        telegram.sourceAddress & 0xFF);
    
    Serial.print(", dst=");
    if (telegram.isGroupAddress) {
        // Format as group address (x/y/z)
        Serial.printf("%d/%d/%d", 
            (telegram.targetAddress >> 11) & 0x1F,
            (telegram.targetAddress >> 8) & 0x07,
            telegram.targetAddress & 0xFF);
    } else {
        // Format as physical address
        Serial.printf("%d.%d.%d", 
            (telegram.targetAddress >> 12) & 0x0F,
            (telegram.targetAddress >> 8) & 0x0F,
            telegram.targetAddress & 0xFF);
    }
    
    Serial.print(", cmd=");
    Serial.printf("0x%02X", telegram.command);
    
    Serial.print(", data=");
    for (const auto& byte : telegram.data) {
        Serial.printf("%02X ", byte);
    }
    
    Serial.println();
}

// DPT Encoding/Decoding Methods

std::vector<uint8_t> KNXIPModule::encodeDPT1(bool value) {
    std::vector<uint8_t> data;
    // First byte: 0x80 for write command + 0x00 for DPT1
    data.push_back(0x80);
    // Second byte: value (0 or 1)
    data.push_back(value ? 0x01 : 0x00);
    return data;
}

std::vector<uint8_t> KNXIPModule::encodeDPT5(uint8_t value) {
    std::vector<uint8_t> data;
    // First byte: 0x80 for write command + 0x00 for DPT5
    data.push_back(0x80);
    // Second byte: percentage value (0-100)
    data.push_back(value);
    return data;
}

std::vector<uint8_t> KNXIPModule::encodeDPT9(float value) {
    std::vector<uint8_t> data;
    // First byte: 0x80 for write command + 0x00 for DPT9
    data.push_back(0x80);
    
    // Convert float to DPT9 format (2-byte float)
    int16_t mantissa = (int16_t)(value * 100.0f);
    uint8_t exponent = 0;
    
    // Adjust exponent and mantissa
    while (mantissa > 2047 || mantissa < -2048) {
        mantissa /= 2;
        exponent++;
    }
    
    // Create the DPT9 value
    uint16_t dpt9Value = (exponent << 11) | (mantissa & 0x07FF);
    if (value < 0) {
        dpt9Value |= 0x8000; // Set sign bit
    }
    
    // Add the two DPT9 bytes
    data.push_back((dpt9Value >> 8) & 0xFF);
    data.push_back(dpt9Value & 0xFF);
    
    return data;
}

bool KNXIPModule::decodeDPT1(const uint8_t* data, size_t length) {
    if (length < 2) return false;
    return (data[1] & 0x01) == 0x01;
}

uint8_t KNXIPModule::decodeDPT5(const uint8_t* data, size_t length) {
    if (length < 2) return 0;
    return data[1];
}

float KNXIPModule::decodeDPT9(const uint8_t* data, size_t length) {
    if (length < 3) return 0.0f;
    
    // Extract the DPT9 value from the data
    uint16_t dpt9Value = (data[1] << 8) | data[2];
    
    // Extract components
    bool sign = (dpt9Value & 0x8000) != 0;
    uint8_t exponent = (dpt9Value & 0x7800) >> 11;
    int16_t mantissa = dpt9Value & 0x07FF;
    
    // Sign extend the mantissa if necessary
    if (sign && mantissa != 0) {
        mantissa |= 0xF800; // Extend with 1s
    }
    
    // Calculate the float value
    float value = (float)mantissa * powf(2.0f, exponent) / 100.0f;
    
    return value;
}
