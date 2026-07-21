/* Frugal IoT - message class
 *
 * This is a common class used by MQTT and LoraMesher 
 */
#ifndef SYSTEM_MESSAGE_H
#define SYSTEM_MESSAGE_H

#include <Arduino.h>
#include "_settings.h"
//#ifdef ESP8266
  #include <forward_list> // Doesnt seem to be required (and may be problmatic) for ESP32
//#endif
#include "system/base.h"
#include "system/io.h"

// If need an extra bit, can assume WakeOnTimerBit = LightSleepBit
#define MsgIsSubscription 0x01
#define MsgFromFS 0x02
#define MsgIsSet 0x04
#define MsgIsThisNode 0x08
#define MsgFromCaptive 0x10
#define MsgFromMQTT 0x20
#ifdef SYSTEM_LORAMESHER_WANT
  #define MsgFromLoRaMesher 0x40
#endif
#define MsgIsLoopback 0x80        // Outgoing message, but looped back in case any wired topics listening
#ifdef SYSTEM_MDNS_WANT
  #define MsgFromMDNS 0x100
  #define MsgMdnsDelivered 0x200 // Outgoing: mDNS peer delivery (publishToSubscribers or publishToPeer)
                                 // has already succeeded for the current payload - set in attemptMdns(),
                                 // cleared in sendRemote() when the payload changes. Until it's set,
                                 // attemptMdns() keeps retrying every loop() (independent of whether
                                 // MQTT/LoRaMesher have accepted the message yet).
#endif

class System_Messages; // to allow forward reference

// Types of messages ... fromFS; fromLoRa; fromMQTT; 

class System_Message { // Only used for outgoing queued messages
  public:
    const String topicPath;
    String payload;    // Retained payload
    System_Message(const String& topicPath, const String& payload, const bool retain, const int qos, const uint16_t flags);
    System_Message(const String& topicPath); // For subscriptions
    //~System_Message();
    // Only currently relevant/accurate on incoming
    bool isThisNode(); // True if message topic matches the 'org/project/node/'
    String topicTwig();  // Return e.g. temperature or temperature/max
    void parse(); // Preload local variables
    String module();
    String leaf();
    bool isSet();
    bool isSubscription();
    void maybeWriteToFS(bool appendValue = false);
    void maybeEcho();
    void maybeWriteToFSandEcho(bool appendValue = false);
  protected:
    friend class System_Messages;
    // Only relevant/accurate on outgoing
    const bool retain;
    const int qos;
    bool send();
    void dispatch();
    bool queuedMessage();
    bool queuedSubscribe();
    #ifdef SYSTEM_MDNS_WANT
      // Attempt mDNS peer delivery for this message: notifyPeersOfSubscription for a
      // subscription message, or publishToSubscribers + publishToPeer for a data
      // message's current payload. Retries every loop() until it succeeds
      // (MsgMdnsDelivered), independently of MQTT/LoRaMesher - called from
      // sendOutgoingQueued() for every queued message, each tick, regardless of
      // queue position. Purely a side channel: its result does not affect whether
      // queuedMessage()/queuedSubscribe() considers this message sent.
      void attemptMdns();
    #endif
  private:
      // Access through functions
      uint16_t flags_;
      String twig_;
      String module_;
      String leaf_;
};

class System_Messages : public System_Base {
  public:
    String topicPrefix;  // Also used by OTA
    System_Messages();
    void subscribe(const String topicPath);
    // This will be re-overloaded as send, but keeping separate as deal with some mem leaks
    void send(const String topicPath, const String payload, bool retain, uint8_t qos);     // send and loopback
    String path(const char* id, const char* const leaf, const char* const leafparm);
    String setPath(char const * const topicTwig);
    String path(const char* id, const char* const leaf);
    String path(const char* id, const String& leaf);
    String path(char const * const topicTwig);
    String path(const String topicTwig); 
    //String twig(const String &topicPath); // unused
    bool reSubscribeAll(); // Called by MQTT after reconnection
    void queueIncoming(const String &topicPath, const String &payload, uint16_t flags); // Called by MQTT and LoRaMesher
    void queueFromCaptive(const String &twig, const String &payload);
    void queueLoopback(const String &topicPath, const String &payload);
    void sendRemote(const String topicPath, const String payload, bool retain, uint8_t qos, uint16_t flags); // Only remote send, no loopback
    #ifdef SYSTEM_MDNS_WANT
      // Send HTTP subscribe POSTs to a newly-discovered mDNS peer for every
      // locally-registered subscription whose topic path starts with that peer's prefix.
      void subscribeViaMdns(const String& peerNodeId, IPAddress ip, uint16_t port);
    #endif
  protected:
    friend class System_Message;
    std::list<System_Message> outgoing;
    std::list<System_Message> incoming;
    std::forward_list<System_Message> subscriptions;
    void sendOutgoingQueued();
    void dispatchIncomingQueued();
    void setup();
    void loop();
  private:
    void buildTopicPrefix();
};

#endif