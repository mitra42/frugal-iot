#ifndef SYSTEM_MQTT_H
#define SYSTEM_MQTT_H

// From MQTT.h copied here so rest of files dont need to include MQTT.h just to subscribe
//typedef void (*MQTTClientCallbackSimple)(String &, String &);
//typedef void (*InputReceivedCallback)(String &);


#include "_settings.h"

#if (!defined(SYSTEM_MQTT_USER) || !defined(SYSTEM_MQTT_PASSWORD))
  error system_discover does not have all requirements: SYSTEM_MQTT_USER SYSTEM_MQTT_PASSWORD 
#endif
#ifndef SYSTEM_MQTT_MS
  #define SYSTEM_MQTT_MS (10) // Every 10 ms
#endif
#ifndef SYSTEM_MQTT_SERVER
  // Where your MQTT server is - you can use frugaliot.naturalinnovation.org for testing, but talk to us if you are deploying live sites using it.
  #define SYSTEM_MQTT_SERVER "frugaliot.naturalinnovation.org"
#endif

#if ESP8266 // Note ESP8266 and ESP32 are defined for respective chips - unclear if anything like that for other Arduinos
  #include <ESP8266WiFi.h>  // for WiFiClient
#else
  #include <WiFi.h> // This will be platform dependent, will work on ESP32 but most likely want configurration for other chips/boards
#endif
#include <MQTT.h> // https://github.com/256dpi/arduino-mqtt
// If configred not to use Wifi (or in future BLE) then will just operate locally, sending MQTT between components on this node, but 
// not elsewhere.
// TODO-49 add support for BLE if it makes sense for MQTT
#include <Arduino.h>
#include "system_wifi.h"   // xWifi
#include "system_discovery.h"
#include <forward_list>
#include "_base.h"

class Subscription {
  // This is both a subscription and a record of a message for retention purposes
  public:
    const String* topicPath;
    String const* payload;    // Retained payload
    Subscription(const String* tp);
    Subscription(const String* tp, String const * payload);
    bool operator==(const String& topicPath);
};
class Message : public Subscription {
  public:
    const bool retain;
    const int qos;
    Message(const String &tp,const String &pl, const bool r, const int q);
};
class MqttManager : public Frugal_Base {
  public:
    WiFiClient net;
    MQTTClient client; //was using (512,128) as discovery message was bouncing back, but no longer subscribing to "device" topic.
    bool inReceived = false;
    bool subscriptionsDone = false; // True when server has reported a session - so dont need to subscribe OR have resubscribed.
    unsigned long nextLoopTime; // TODO-25 may move into superclass
    unsigned long ms;
    MqttManager();
    void setup();
    void loop();
    bool connect(); // Connect to MQTT broker and - if necessary - resubscribe to all topics
    void blockTillConnected(); // Connect to MQTT, loop until succeed
    Subscription* find(const String &topicPath);
    void subscribe(const String& topicPath);
    void subscribe(const char* topicTwig);
    void dispatch(const String &topicPath, const  String &payload);
    bool resubscribeAll();
    void retainPayload(const String &topicPath, const String &payload);
    void messageReceived(const String &topicPath, const String &payload);
    void messageSendInner(const String &topicPath, const String &payload, const bool retain, const int qos);
    // Note, there are many of messageSend to make sensor code simple and not duplicate conversions.
    void messageSend(const String &topicPath, const String &payload, const bool retain, const int qos);
    void messageSend(const char* topicTwig, const String &payload, const bool retain, const int qos);
    void messageSend(const String &topicPath, const float &value, const int width, const bool retain, const int qos);
    void messageSend(const char* topicTwig, const float &value, const int width, const bool retain, const int qos);
    void messageSend(const String &topicPath, const int value, const bool retain, const int qos);
    void messageSend(const char* topicTwig, const int value, const bool retain, const int qos);
    void messageSend(const String &topicPath, const bool value, const bool retain, const int qos);
    void messageSend(const char* topicTwig, const bool value, const bool retain, const int qos);
    void messageSendQueued();
    String* path(char const * const topicTwig);
    String* twig(const String &topicPath);
  
  protected: // TODO - some of the other methods should probably be protected
    std::forward_list<Subscription> subscriptions;
    std::forward_list<Message> queued;
};

namespace xMqtt {
  void MessageReceived(String &topicPath, String &payload);
  void setup();
  void loop();
} // namespace xMqtt

extern MqttManager* Mqtt; // Will get initialized by setup in frugalIot.ino

#endif // SYSTEM_MQTT_H
