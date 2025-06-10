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

    System_LoraMesher();
    void setup();
    void periodically();
    #if !defined(SYSTEM_LORAMESHER_SENDER_TEST) && !defined(SYSTEM_LORAMESHER_RECEIVER_TEST)
      // Match mqtt.client profile
      publish(const String &topicPath, const String &payload, const bool retain, const int qos);
    #endif
    void prepareForSleep();
};
#endif // SYSTEM_LORAMESHER_WANT
#endif // SYSTEM_LORAMESHER_H
extern System_LoraMesher* loramesher;
