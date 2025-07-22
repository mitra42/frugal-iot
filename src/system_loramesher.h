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
#ifdef SYSTEM_LORAMESHER_WANT  // defined in platformio.ini

#include "LoraMesher.h"
#include "system_base.h"

class System_LoraMesher : public System_Base {
  public:
    LoraMesher& radio;
    LoraMesher::LoraMesherConfig config = LoraMesher::LoraMesherConfig();
    uint16_t gatewayNodeAddress;
    uint16_t rcvdPacketCounter = 0;
    uint16_t sentPacketCounter = 0; 
    System_LoraMesher();
    bool findGatewayNode();
    void setup() override;
    //void periodically() override;
    // Match mqtt.client profile
    void publish(const String &topicPath, const String &payload, const bool retain, const int qos);
    void processReceivedPacket(AppPacket<uint8_t>* appPacket);
    void prepareForSleep();
};

// Adapted From LoRaChat/src/loramesh/loraMeshMessage.h

#pragma pack(push, 1)

/*
enum LoRaMeshMessageType: uint8_t {
    sendMessage = 1,
    getRoutingTable = 2,
};
*/
class FrugalIoTMessage {
public:
    //appPort appPortDst;  // TODO-152 where is appPort defined
    //appPort appPortSrc;
    uint8_t messageId;
    uint8_t message[];
};
#pragma pack(pop)

#endif // SYSTEM_LORAMESHER_WANT
#endif // SYSTEM_LORAMESHER_H

