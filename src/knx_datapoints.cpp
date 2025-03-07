//==== src/knx_datapoints.cpp ====

#include "knx_datapoints.h"
#include <Arduino.h>
#include <cmath>

std::vector<uint8_t> KNXDatapoints::encode(KNXDatapointType dpt, const void* value) {
    switch (dpt) {
        case KNX_DPT_1:
            return encodeBool(*static_cast<const bool*>(value));
        case KNX_DPT_5:
            return encodeUInt8(*static_cast<const uint8_t*>(value));
        case KNX_DPT_9:
            return encodeFloat(*static_cast<const float*>(value));
        default:
            // Unsupported DPT, return empty vector
            return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> KNXDatapoints::encodeBool(bool value) {
    std::vector<uint8_t> data;
    // TPCI (0x00) + first 2 bits of APCI (GroupValue_Write = 0x02 >> 2 = 0x00)
    data.push_back(0x00);
    // Last 4 bits of APCI (GroupValue_Write = 0x02 & 0x03 = 0x02) shifted to bits 7,6 + value in bit 0
    data.push_back(0x80 | (value ? 0x01 : 0x00));
    return data;
}

std::vector<uint8_t> KNXDatapoints::encodeUInt8(uint8_t value) {
    std::vector<uint8_t> data;
    // TPCI (0x00) + first 2 bits of APCI (GroupValue_Write = 0x02 >> 2 = 0x00)
    data.push_back(0x00);
    // Last 4 bits of APCI (GroupValue_Write = 0x02 & 0x03 = 0x02) shifted to bits 7,6
    data.push_back(0x80);
    // Additional byte for the value
    data.push_back(value);
    return data;
}

std::vector<uint8_t> KNXDatapoints::encodeFloat(float value) {
    std::vector<uint8_t> data;
    // TPCI (0x00) + first 2 bits of APCI (GroupValue_Write = 0x02 >> 2 = 0x00)
    data.push_back(0x00);
    // Last 4 bits of APCI (GroupValue_Write = 0x02 & 0x03 = 0x02) shifted to bits 7,6
    data.push_back(0x80);
    
    // Convert float to KNX DPT9 format (2-byte float)
    uint16_t knxFloat = floatToKnxFloat(value);
    data.push_back((knxFloat >> 8) & 0xFF);
    data.push_back(knxFloat & 0xFF);
    
    return data;
}

bool KNXDatapoints::decodeBool(const std::vector<uint8_t>& data) {
    if (data.size() < 2) return false;
    return (data[1] & 0x01) == 0x01;
}

uint8_t KNXDatapoints::decodeUInt8(const std::vector<uint8_t>& data) {
    if (data.size() < 3) return 0;
    return data[2];
}

float KNXDatapoints::decodeFloat(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return 0.0f;
    
    uint16_t knxFloat = (data[2] << 8) | data[3];
    return knxFloatToFloat(knxFloat);
}

bool KNXDatapoints::isValidValue(KNXDatapointType dpt, const void* value) {
    switch (dpt) {
        case KNX_DPT_1:
            // Boolean is always valid
            return true;
            
        case KNX_DPT_5: {
            // 8-bit unsigned integer (0-255)
            return true; // All uint8_t values are valid
        }
            
        case KNX_DPT_9: {
            // 2-byte float value
            float val = *static_cast<const float*>(value);
            // DPT9 can represent values from -671088.64 to 670760.96
            return (val >= -671088.64f && val <= 670760.96f);
        }
            
        default:
            return false; // Unsupported DPT
    }
}

uint16_t KNXDatapoints::floatToKnxFloat(float value) {
    // Convert float to KNX DPT9 format (2-byte float)
    // Scale the float value by 100 and calculate mantissa/exponent
    int16_t mantissa = (int16_t)(value * 100.0f);
    uint8_t exponent = 0;
    
    // Adjust exponent and mantissa
    while (mantissa > 2047 || mantissa < -2048) {
        mantissa /= 2;
        exponent++;
    }
    
    // Create the DPT9 value
    uint16_t knxValue = (exponent << 11) | (mantissa & 0x07FF);
    if (value < 0) {
        knxValue |= 0x8000; // Set sign bit
    }
    
    return knxValue;
}

float KNXDatapoints::knxFloatToFloat(uint16_t knxFloat) {
    // Extract components
    bool sign = (knxFloat & 0x8000) != 0;
    uint8_t exponent = (knxFloat & 0x7800) >> 11;
    int16_t mantissa = knxFloat & 0x07FF;
    
    // Sign extend the mantissa if necessary
    if (sign && mantissa != 0) {
        mantissa |= 0xF800; // Extend with 1s
    }
    
    // Calculate the float value
    float value = (float)mantissa * powf(2.0f, exponent) / 100.0f;
    
    return value;
}