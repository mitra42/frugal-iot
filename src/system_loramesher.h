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


enum LoraMesherMode {
    LORAMESHER_GATEWAY,
    LORAMESHER_NODE,
    LORAMESHER_UNCONNECTED,
};


class System_LoraMesher : public System_Base {
  public:
    System_LoraMesher();
    LoraMesher& radio;  // Accessed from processReceivedPackets so has to be public
    // == INCOMING (up or downstream)
    // == OUTGOING (up or downstream)
      bool connected();
      // == UPSTREAM 
      LoraMesherMode checkRole();
      const __FlashStringHelper* checkRoleString();
      // Match mqtt.client profile
      bool publish(const String &topicPath, const String &payload, const bool retain, const int qos);
      // == DOWNSTREAM 
      // public only because called from the callback - do not use externally
      void processReceivedPacket(AppPacket<uint8_t>* appPacket); 
      AppPacket<uint8_t>* lastPacket = nullptr; // Used by printAppData
  protected:
    LoraMesher::LoraMesherConfig config = LoraMesher::LoraMesherConfig();
    uint16_t gatewayNodeAddress = BROADCAST_ADDR;
    uint16_t rcvdPacketCounter = 0;
    uint16_t sentPacketCounter = 0; 
    unsigned long lostMQTTat;
    std::forward_list<MeshSubscription> meshSubscriptions;
    void setup() override;
    void periodically() override;
    void prepareForSleep();
    // == INCOMING (up or downstream)
    // == OUTGOING (up or downstream)
      bool findGatewayNode();
      void buildAndSend(uint16_t destn, const String &topic, const String &payload, bool retain, int qos);
    // == UPSTREAM 
    // == DOWNSTREAM 
      void relayDownstream(uint16_t destn, const String &topic, const String &payload);
      void dispatchPath(const String &topicPath, const String &payload) override;
};

#endif // SYSTEM_LORAMESHER_WANT
#endif // SYSTEM_LORAMESHER_H

