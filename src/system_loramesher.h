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
#include "LoraMesher.h"
#include "system_base.h"

class MeshSubscription {
  public:
    const String topicPath;
    const uint16_t src; // The node id subscribing 
    MeshSubscription(const String topicPath, const uint16_t src);
};

class System_LoraMesher : public System_Base {
  public:
    System_LoraMesher();
    bool connected();
    // Match mqtt.client profile
    bool publish(const String &topicPath, const String &payload, const bool retain, const int qos);
    // Called from processReceivedPackets
    LoraMesher& radio;
    void processReceivedPacket(AppPacket<uint8_t>* appPacket);
  private:
    LoraMesher::LoraMesherConfig config = LoraMesher::LoraMesherConfig();
    uint16_t gatewayNodeAddress;
    uint16_t rcvdPacketCounter = 0;
    uint16_t sentPacketCounter = 0; 
    bool findGatewayNode();
    void setup() override;
    //void periodically() override;
    void prepareForSleep();
    std::forward_list<MeshSubscription> meshSubscriptions;
    void dispatchPath(const String &topicPath, const String &payload) override;
    void buildAndSend(uint16_t destn, const String &topic, const String &payload, bool retain, int qos);
    void relayDownstream(uint16_t destn, const String &topic, const String &payload);
};

#endif // SYSTEM_LORAMESHER_WANT
#endif // SYSTEM_LORAMESHER_H

