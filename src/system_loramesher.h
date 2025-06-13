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
#ifdef SYSTEM_LORAMESHER_WANT

#include "LoraMesher.h"
#include "_base.h"

class System_LoraMesher : public Frugal_Base {
  public:
    LoraMesher& radio;
    LoraMesher::LoraMesherConfig config = LoraMesher::LoraMesherConfig();
    uint16_t gatewayNodeAddress;
    System_LoraMesher();
    bool findGatewayNode();
    void setup();
    void periodically();
    #if !defined(SYSTEM_LORAMESHER_SENDER_TEST) && !defined(SYSTEM_LORAMESHER_RECEIVER_TEST)
      // Match mqtt.client profile
      publish(const String &topicPath, const String &payload, const bool retain, const int qos);
    #endif
    void prepareForSleep();
};

extern System_LoraMesher* loramesher;

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

