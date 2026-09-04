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
#include "system/wifi.h"   // xWiFi
#include "system/discovery.h"
#include <forward_list>
#include "system/base.h"
#include "system/io.h"

class System_MQTT : public System_Base {
  public:
    System_MQTT(const char* hostname, const char* username, const char* password);
    void configure_enrolment(const char* secret) { enrolmentSecret = secret; }
    void setup_after_wifi();
    bool connected(); // Check if connected, dont change status
    void prepare() override;
    void recover() override;
    // Downstream
    void messageReceived(const String &topicPath, const String &payload); // Used in MqttMessageReceived callbacks 
        // Upstream
    bool subscribe(const String& topicPath);
    bool send(const String &topicPath, const String &payload, const bool retain, const int qos);

  protected: // TODO - some of the other methods should probably be protected
    MQTTClient client; //was using (512,128) as discover message was bouncing back, but no longer subscribing to "device" topic.
    WiFiClient net;
    String hostname; 
    // The credential this node was issued by the server, read from LittleFS at setup and written
    // there by enrol(). Empty until it has enrolled, in which case the compiled-in username and
    // password below are used instead - which is how a sketch using the older
    // configure_mqtt(host, user, password) form keeps working.
    String storedUsername;
    String storedPassword;
    // Set by configure_enrolment(). Grants exactly one thing: "create a node in this organization".
    // No read, no write, no broker access. See SECURITY-REVIEW.md section 6.
    const char* enrolmentSecret = nullptr;
    bool enrolTried = false;         // one attempt per boot, so a refusal is not a hot loop
    // Consecutive refusals of the stored credential. The broker can legitimately forget a node -
    // an administrator resetting it from the dashboard, or dynsec state restored from an older
    // backup - and a node holding a credential would otherwise never enrol again, because it has
    // one. So a credential the broker keeps rejecting is discarded and a new one asked for.
    uint8_t authFailures = 0;
    bool inReceived = false;
    unsigned long ms;
    unsigned long nextLoopTime; // Not sleepSafeSecs as frequent.
    bool subscriptionsDone = false; // True when server has reported a session - so dont need to subscribe OR have resubscribed. Also true at start before did subscriptions.
    const char* password;
    const char* username;
    void setup() override;
    void captiveLines(AsyncResponseStream* response) override;
    void loop() override;
    bool connect(); // Connect to MQTT broker and - if necessary - resubscribe to all topics
    // Fetch this node's own broker credential from the server, over HTTPS, if it has none yet.
    // Does nothing if it already has one, or if no enrolment secret was configured.
    void enrolIfNeeded();
    void noteAuthFailure();          // called when the broker rejects our credential
    const char* mqttUsername();     // stored if enrolled, else the compiled-in one
    const char* mqttPassword();
    void dispatch(System_Message &msg) override;
};

#endif // SYSTEM_MQTT_H
