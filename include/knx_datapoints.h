//==== include/knx_datapoints.h ====

#ifndef KNX_DATAPOINTS_H
#define KNX_DATAPOINTS_H

#include <Arduino.h>
#include <vector>

// KNX Datapoint Types
enum KNXDatapointType {
    // Basic types (main type 1-20)
    KNX_DPT_1 = 1,   // 1-bit boolean value
    KNX_DPT_2 = 2,   // 2-bit control
    KNX_DPT_3 = 3,   // 3-bit controlled
    KNX_DPT_4 = 4,   // Character (8-bit)
    KNX_DPT_5 = 5,   // 8-bit unsigned value
    KNX_DPT_6 = 6,   // 8-bit signed value
    KNX_DPT_7 = 7,   // 2-byte unsigned value
    KNX_DPT_8 = 8,   // 2-byte signed value
    KNX_DPT_9 = 9,   // 2-byte float
    KNX_DPT_10 = 10, // Time
    KNX_DPT_11 = 11, // Date
    KNX_DPT_12 = 12, // 4-byte unsigned value
    KNX_DPT_13 = 13, // 4-byte signed value
    KNX_DPT_14 = 14, // 4-byte float
    KNX_DPT_15 = 15, // Access data
    KNX_DPT_16 = 16, // String (max 14 chars)
    KNX_DPT_17 = 17, // Scene number
    KNX_DPT_18 = 18, // Scene control
    KNX_DPT_19 = 19, // Date and time
    KNX_DPT_20 = 20  // 1-byte enumeration
};

// Common sub-types
enum KNXDatapointSubtype {
    // DPT1 subtypes
    KNX_DPT_1_001 = 1001, // Switch (on/off)
    KNX_DPT_1_002 = 1002, // Boolean (true/false)
    KNX_DPT_1_003 = 1003, // Enable
    KNX_DPT_1_008 = 1008, // Up/Down
    KNX_DPT_1_009 = 1009, // Open/Close
    
    // DPT5 subtypes
    KNX_DPT_5_001 = 5001, // Percentage (0-100%)
    KNX_DPT_5_003 = 5003, // Angle (0-360°)
    KNX_DPT_5_004 = 5004, // Percentage (0-255%)
    KNX_DPT_5_010 = 5010, // Counter pulses
    
    // DPT9 subtypes
    KNX_DPT_9_001 = 9001, // Temperature (°C)
    KNX_DPT_9_002 = 9002, // Temperature difference (K)
    KNX_DPT_9_003 = 9003, // Temperature gradient (K/h)
    KNX_DPT_9_004 = 9004, // Light illuminance (lux)
    KNX_DPT_9_005 = 9005, // Wind speed (m/s)
    KNX_DPT_9_006 = 9006, // Air pressure (Pa)
    KNX_DPT_9_007 = 9007, // Humidity (%)
    KNX_DPT_9_008 = 9008, // Air quality (ppm)
    KNX_DPT_9_010 = 9010, // Time (s)
    KNX_DPT_9_011 = 9011, // Time (ms)
    KNX_DPT_9_020 = 9020, // Voltage (mV)
    KNX_DPT_9_021 = 9021  // Current (mA)
};

class KNXDatapoints {
public:
    // Format data for KNX telegrams based on DPT
    static std::vector<uint8_t> encode(KNXDatapointType dpt, const void* value);
    static std::vector<uint8_t> encodeBool(bool value);
    static std::vector<uint8_t> encodeUInt8(uint8_t value);
    static std::vector<uint8_t> encodeFloat(float value);
    
    // Decode KNX telegram data to native types
    static bool decodeBool(const std::vector<uint8_t>& data);
    static uint8_t decodeUInt8(const std::vector<uint8_t>& data);
    static float decodeFloat(const std::vector<uint8_t>& data);
    
    // Check if a value is within range for a given DPT
    static bool isValidValue(KNXDatapointType dpt, const void* value);

private:
    // Helper functions for DPT9 (2-byte float)
    static uint16_t floatToKnxFloat(float value);
    static float knxFloatToFloat(uint16_t knxFloat);
};

#endif // KNX_DATAPOINTS_H