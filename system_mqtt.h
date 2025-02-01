#ifndef SYSTEM_MQTT_H
#define SYSTEM_MQTT_H

// From MQTT.h copied here so rest of files dont need to include MQTT.h just to subscribe
//typedef void (*MQTTClientCallbackSimple)(String &, String &);
//typedef void (*InputReceivedCallback)(String &);


#include "_settings.h"

#if (!defined(SYSTEM_MQTT_USER) || !defined(SYSTEM_MQTT_PASSWORD) || !defined(SYSTEM_MQTT_MS))
  error system_discover does not have all requirements in _configuration.h: SYSTEM_DISCOVERY_MS 
#endif
#if ESP8266 // Note ESP8266 and ESP32 are defined for respective chips - unclear if anything like that for other Arduinos
  #include <ESP8266WiFi.h>  // for WiFiClient
#else
  #include <WiFi.h> // This will be platform dependent, will work on ESP32 but most likely want configurration for other chips/boards
#endif
#include <MQTT.h>
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
    const String* topicpath;
    String const* payload;    // Retained payload
    Subscription(const String* tp);
    Subscription(const String* tp, String const * payload);
    bool operator==(const String& topicpath);
};
class Message : public Subscription {
  public:
    const bool retain;
    const int qos;
    Message(const String &tp, const String &pl, const bool r, const int q);
};
class MqttManager : public Frugal_Base {
  public:
    WiFiClient net;
    MQTTClient client; //was using (512,128) as discovery message was bouncing back, but no longer subscribing to "device" topic.
    bool inReceived = false;
    unsigned long nextLoopTime; // TODO-25 may move into superclass
    unsigned long ms;
    MqttManager();
    void setup();
    void loop();
    bool connect();
    Subscription* find(const String &topicpath);
    void subscribe(const String& topicpath);
    void subscribe(const char* topicleaf);
    void dispatch(const String &topicpath, const  String &payload);
    void resubscribeAll();
    void retainPayload(const String &topicpath, const String &payload);
    void messageReceived(const String &topic, const String &payload);
    void messageSendInner(const String &topicpath, const String &payload, const bool retain, const int qos);
    // Note, there are many of messageSend to make sensor code simple and not duplicate conversions.
    void messageSend(const String &topicpath, const String &payload, const bool retain, const int qos);
    void messageSend(const char* topicleaf, const String &payload, const bool retain, const int qos);
    void messageSend(const String &topicpath, const float &value, const int width, const bool retain, const int qos);
    void messageSend(const char* topicleaf, const float &value, const int width, const bool retain, const int qos);
    void messageSend(const String &topicpath, const int value, const bool retain, const int qos);
    void messageSend(const char* topicleaf, const int value, const bool retain, const int qos);
    void messageSendQueued();
    String* topicPath(char const * const topicleaf);
    String* topicLeaf(const String &topicpath);
  
  protected: // TODO - some of the other methods should probably be protected
    std::forward_list<Subscription> items; // An item could be a retained message or a subscription - they overlap
    std::forward_list<Message> queued; // An item could be a retained message or a subscription - they overlap
};

namespace xMqtt {
  void MessageReceived(String &topic, String &payload);
  void setup();
  void loop();
} // namespace xMqtt

extern MqttManager* Mqtt; // Will get initialized by setup in frugalIot.ino

#endif // SYSTEM_MQTT_H
