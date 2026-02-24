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
#include "system_base.h"

class System_Messages; // to allow forward reference

class System_Message { // Only used for outgoing queued messages
  public:
    System_Message(const String& topicPath, const String& payload, const bool retain, const int qos, const bool isSubscription = false);
    System_Message(const String& topicPath); // For subscriptions
    //~System_Message();
  protected:
    friend class System_Messages;
    bool send(); 
    const String topicPath;
    String payload;    // Retained payload
    // Only relevant/accurate on outgoing
    const bool isSubscription;
    const bool retain;
    const int qos;
    void dispatch();
    bool queuedMessage();
    bool queuedSubscribe();
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
    String path(char const * const topicTwig);
    String path(const String topicTwig); 
    //String twig(const String &topicPath); // unused
    bool reSubscribeAll(); // Called by MQTT after reconnection
    void queueIncoming(const String &topicPath, const String &payload); // Called by MQTT and LoRaMesher
    void queueFromCaptive(const String &twig, const String &payload);
    void queueLoopback(const String &topicPath, const String &payload);
    void sendRemote(const String topicPath, const String payload, bool retain, uint8_t qos); // Only remote send, no loopback
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
    void buildTopicPrefix(); // TODO-205 may need to be public for call by frugal_iot.dispatch
};

#endif