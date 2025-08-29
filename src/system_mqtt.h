#ifndef SYSTEM_MQTT_H
#define SYSTEM_MQTT_H

// From MQTT.h copied here so rest of files dont need to include MQTT.h just to subscribe
//typedef void (*MQTTClientCallbackSimple)(String &, String &);
//typedef void (*InputReceivedCallback)(String &);


#include "_settings.h"

#ifndef SYSTEM_MQTT_MS
  #define SYSTEM_MQTT_MS (10) // Every 10 ms
#endif

#if ESP8266 // Note ESP8266 and ESP32 are defined for respective chips - unclear if anything like that for other Arduinos
  #include <ESP8266WiFi.h>  // for WiFiClient
#else
  #include <WiFi.h> // This will be platform dependent, will work on ESP32 but most likely want configurration for other chips/boards
#endif
#include <MQTT.h> // https://github.com/256dpi/arduino-mqtt
// If configred not to use WiFi (or in future BLE) then will just operate locally, sending MQTT between components on this node, but 
// not elsewhere.
// TODO-49 add support for BLE if it makes sense for MQTT
#include <Arduino.h>
#include "system_wifi.h"   // xWiFi
#include "system_discovery.h"
#include <forward_list>
#include "system_base.h"

class System_MQTT : public System_Base {
  public:
    System_MQTT(const char* hostname, const char* username, const char* password);
    void setup_after_wifi();
    bool connected(); // Check if connected, dont change status
    bool prepareForLightSleep();
    bool recoverFromLightSleep();
    // Downstream
    void messageReceived(const String &topicPath, const String &payload); // Used in MqttMessageReceived callbacks 
        // Upstream
    bool subscribe(const String& topicPath);
    bool send(const String &topicPath, const String &payload, const bool retain, const int qos);

  protected: // TODO - some of the other methods should probably be protected
    MQTTClient client; //was using (512,128) as discover message was bouncing back, but no longer subscribing to "device" topic.
    WiFiClient net;
    String hostname; 
    bool inReceived = false;
    unsigned long ms;
    unsigned long nextLoopTime; // Not sleepSafeMillis as frequent.
    bool subscriptionsDone = false; // True when server has reported a session - so dont need to subscribe OR have resubscribed. Also true at start before did subscriptions.
    const char* password;
    const char* username;
    void setup() override;
    void captiveLines(AsyncResponseStream* response) override;
    void loop() override;
    bool connect(); // Connect to MQTT broker and - if necessary - resubscribe to all topics
    void dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet); // receiving message for the mqtt module
};

#endif // SYSTEM_MQTT_H
