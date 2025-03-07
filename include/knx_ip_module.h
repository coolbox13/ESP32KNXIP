//==== include/knx_ip_module.h ====

#ifndef KNX_IP_MODULE_H
#define KNX_IP_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>
#include <functional>
#include <map>
#include <vector>

// KNX Constants
#define KNX_PORT 3671
#define KNX_MULTICAST_IP IPAddress(224, 0, 23, 12)

// Communication Modes
enum KNXConnectionType {
    KNX_CONNECTION_UNICAST,   // Direct communication with a KNX/IP gateway
    KNX_CONNECTION_MULTICAST  // Multicast mode for monitoring
};

// DPT Types (partial list)
enum KNXDataPointType {
    KNX_DPT_1_001,  // 1-bit boolean
    KNX_DPT_5_001,  // 8-bit unsigned (percentage)
    KNX_DPT_9_001   // 2-byte float (temperature)
    // Add more as needed
};

// KNX Telegram structure for better parsing
struct KNXTelegram {
    uint16_t sourceAddress;
    uint16_t targetAddress;
    bool isGroupAddress;
    uint8_t routingCounter;
    uint8_t command;
    std::vector<uint8_t> data;
    
    String toString() const;
};

// Callback definition for group address notifications
using KNXGroupAddressCallback = std::function<void(const KNXTelegram& telegram)>;

class KNXIPModule {
public:
    KNXIPModule(); 
    
    // Setup functions
    bool begin(const IPAddress& gatewayIP, int knxArea, int knxLine, int knxMember);
    bool beginMulticast(int knxArea, int knxLine, int knxMember);
    void setDebugLevel(uint8_t level);
    
    // Communication functions
    bool sendKNXMessage(int groupAddress, const uint8_t* data, size_t dataLength);
    
    // Higher-level functions with DPT support
    bool sendBool(int groupAddress, bool value);  // DPT 1.001
    bool sendPercentage(int groupAddress, uint8_t percentage);  // DPT 5.001
    bool sendTemperature(int groupAddress, float temperature);  // DPT 9.001
    
    // Callback registration
    void onGroupAddress(int groupAddress, KNXGroupAddressCallback callback);
    void removeCallback(int groupAddress);
    
    // DPT conversion utilities
    static std::vector<uint8_t> encodeDPT1(bool value);
    static std::vector<uint8_t> encodeDPT5(uint8_t value);
    static std::vector<uint8_t> encodeDPT9(float value);
    
    static bool decodeDPT1(const uint8_t* data, size_t length);
    static uint8_t decodeDPT5(const uint8_t* data, size_t length);
    static float decodeDPT9(const uint8_t* data, size_t length);

private:
    AsyncUDP udp;
    IPAddress gatewayIP;
    int knxArea;
    int knxLine;
    int knxMember;
    uint16_t physicalAddress;
    KNXConnectionType connectionType;
    uint8_t debugLevel; // 0=minimal, 1=normal, 2=verbose
    
    std::map<int, std::vector<KNXGroupAddressCallback>> callbacks;
    
    void processUdpData(AsyncUDPPacket packet);
    KNXTelegram parseTelegram(const uint8_t* data, size_t length);
    void notifyCallbacks(const KNXTelegram& telegram);
    void logTelegram(const KNXTelegram& telegram, bool outgoing);
};

#endif // KNX_IP_MODULE_H