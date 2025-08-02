/* Frugal IoT - message class
 *
 * This is a common class used by MQTT and LoraMesher 
 */
#ifndef SYSTEM_MESSAGE_H
#define SYSTEM_MESSAGE_H

#include <Arduino.h>
#include "system_base.h"

class System_Messages; // to allow forward reference

class System_Message { // Only used for outgoing queued messages
  public:
    System_Message(const String* topicPath, const String* payload, const bool retain, const int qos, const bool isSubscription = false);
    System_Message(const String* topicPath); // For subscriptions
    //~System_Message();
  protected:
    friend class System_Messages;
    bool send();
    const String* topicPath;
    const String* payload;    // Retained payload
    const bool isSubscription;
    const bool retain;
    const int qos;

    bool queuedMessage();
    bool queuedSubscribe();
};

class System_Messages : System_Base {
  public:
    System_Messages();
    String* topicPrefix;  // Also used by OTA
    void subscribe(const String* topicPath);
    void send(const String* topicPath, const String* payload, bool retain, uint8_t qos);
    String* path(char const * const topicTwig); // returns a new String* 
    String* path(const String* topicTwig); // returns a new String* 
    String* twig(const String &topicPath);
    bool reSubscribeAll(); // Called by MQTT after reconnection
    void dispatch(const String &topicPath, const String &payload); // Called by MQTT and LoRaMesher
  protected:
    friend class System_Message;
    std::list<System_Message> outgoing;
    std::forward_list<System_Message> subscriptions;
    void sendQueued();
    void setup();
    void loop();
};

#endif