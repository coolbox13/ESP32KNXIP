//==== include/knx_ip_core.h ====

#ifndef KNX_IP_CORE_H
#define KNX_IP_CORE_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>
#include <vector>
#include "knx_ip_module.h" // For KNXTelegram definition

// KNXnet/IP Service Types
enum KNXServiceType {
    KNX_ST_SEARCH_REQUEST = 0x0201,
    KNX_ST_SEARCH_RESPONSE = 0x0202,
    KNX_ST_DESCRIPTION_REQUEST = 0x0203,
    KNX_ST_DESCRIPTION_RESPONSE = 0x0204,
    KNX_ST_CONNECT_REQUEST = 0x0205,
    KNX_ST_CONNECT_RESPONSE = 0x0206,
    KNX_ST_CONNECTIONSTATE_REQUEST = 0x0207,
    KNX_ST_CONNECTIONSTATE_RESPONSE = 0x0208,
    KNX_ST_DISCONNECT_REQUEST = 0x0209,
    KNX_ST_DISCONNECT_RESPONSE = 0x020A,
    KNX_ST_TUNNELING_REQUEST = 0x0420,
    KNX_ST_TUNNELING_ACK = 0x0421,
    KNX_ST_ROUTING_INDICATION = 0x0530,
    KNX_ST_ROUTING_LOST_MESSAGE = 0x0531
};

// CEMI Message Codes
enum KNXMessageCode {
    KNX_CEMI_L_DATA_REQ = 0x11, // Request to send a data frame
    KNX_CEMI_L_DATA_IND = 0x29, // Data frame received notification
    KNX_CEMI_L_DATA_CON = 0x2E  // Confirmation of a data frame send
};

// APCI Codes
enum KNXAPCI {
    KNX_APCI_GROUP_VALUE_READ = 0x00,
    KNX_APCI_GROUP_VALUE_RESPONSE = 0x01,
    KNX_APCI_GROUP_VALUE_WRITE = 0x02,
    KNX_APCI_PHYSICAL_ADDRESS_WRITE = 0x03,
    KNX_APCI_PHYSICAL_ADDRESS_READ = 0x04,
    KNX_APCI_PHYSICAL_ADDRESS_RESPONSE = 0x05,
    KNX_APCI_ADC_READ = 0x06,
    KNX_APCI_ADC_RESPONSE = 0x07,
    KNX_APCI_MEMORY_READ = 0x08,
    KNX_APCI_MEMORY_RESPONSE = 0x09,
    KNX_APCI_MEMORY_WRITE = 0x0A
};

class KNXIPCore {
public:
    KNXIPCore();
    
    // Core functions for payload construction
    static uint16_t assembleGroupAddress(uint8_t mainGroup, uint8_t midGroup, uint8_t subGroup);
    static uint16_t assemblePhysicalAddress(uint8_t area, uint8_t line, uint8_t device);
    
    static void parseGroupAddress(uint16_t groupAddress, uint8_t& mainGroup, uint8_t& midGroup, uint8_t& subGroup);
    static void parsePhysicalAddress(uint16_t physicalAddress, uint8_t& area, uint8_t& line, uint8_t& device);
    
    // KNX/IP packet construction
    static std::vector<uint8_t> createRoutingIndication(uint16_t sourceAddr, uint16_t targetAddr, 
                                                        bool isGroupAddress, const std::vector<uint8_t>& apdu);
    
    static std::vector<uint8_t> createTunnelingRequest(uint8_t channelId, uint8_t seqNumber, 
                                                      uint16_t sourceAddr, uint16_t targetAddr, 
                                                      bool isGroupAddress, const std::vector<uint8_t>& apdu);
                                                      
    // Helper method for debugging
    static void logBuffer(const char* prefix, const uint8_t* buffer, size_t length, uint8_t debugLevel);
};

#endif // KNX_IP_CORE_H