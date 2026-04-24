/* Frugal IoT - LoRaMesher wrapper 
 *
 * Based on examples at https://github.com/LoRaMesher/LoRaMesher
 * 
 * Issues: https://github.com/mitra42/frugal-iot/issues/137
 * 
 * Notes:
 *    This uses the "radio" library for LoRa rather than Sandeep Mishra's 
 * 
*/
#ifndef SYSTEM_LORAMESHER_H
#define SYSTEM_LORAMESHER_H

#include "_settings.h"
// defined in _settings.h if board has LoRa, can also define in platformio.ini if e.g. have a LoRa shield
#ifdef SYSTEM_LORAMESHER_WANT  // defined in platformio.ini

#include <forward_list>
#include "loramesher.hpp" // defines namespace loramesher 
#include "system_base.h"

class MeshSubscription {
  public:
    const String topicPath;
    const uint16_t src; // The node id subscribing 
    MeshSubscription(const String topicPath, const uint16_t src);
};


enum LoraMesherMode {
    LORAMESHER_GATEWAY,
    LORAMESHER_NODE,
    LORAMESHER_UNCONNECTED,
};

class System_LoraMesher : public System_Base {
  public:
    System_LoraMesher();
    std::unique_ptr<loramesher::LoraMesher> mesher = nullptr;
    #ifdef SYSTEM_LORAMESHER_DEBUG
      void printRouteTable();
      void printNetworkStatus();
    #endif
    // == INCOMING (up or downstream)
    // public only because called from the callback - do not use externally
    #ifdef SYSTEM_LORAMESHER_DEBUG
      String lastTopicPath = String(); // Used by printAppData
      String lastPayload = String(); // Used by printAppData
    #endif
    void processReceivedPacket(loramesher::AddressType source, const std::vector<uint8_t>& data); 
    // == OUTGOING (up or downstream)
    bool connected();
    // Match mqtt.client profile
    bool publish(const String &topicPath, const String &payload, const bool retain, const int qos);
    // == UPSTREAM 
    LoraMesherMode checkRole();
    const __FlashStringHelper* checkRoleString();
    // == DOWNSTREAM 

  protected:
    unsigned long lostMQTTat;
    bool initialize(); // Try and create the LoraMesher instance
    void setup() override;
    void prepareForLightSleep();
    void periodically() override;
    bool isGateway(); 
    void createReceiveMessages();
    void PromoteToNetworkManager();
    // == INCOMING (up or downstream)
    uint16_t rcvdPacketCounter = 0;
    std::forward_list<MeshSubscription> meshSubscriptions;
    // == OUTGOING (up or downstream)
    uint16_t sentPacketCounter = 0; 
    bool findGatewayNode();
    void buildAndSend(uint16_t destn, const String &topic, const String &payload, bool retain, int qos);
    // == UPSTREAM 
    uint16_t gatewayNodeAddress = loramesher::kBroadcastAddress;
    // == DOWNSTREAM 
    void relayDownstream(uint16_t destn, const String &topic, const String &payload);
    void dispatchPath(const String &topicPath, const String &payload) override;
 
    #ifdef TODO_189_NOT_NEEDED
      LoraMesher::LoraMesherConfig config = LoraMesher::LoraMesherConfig();
    #endif
};

#endif // SYSTEM_LORAMESHER_WANT
#endif // SYSTEM_LORAMESHER_H

