#ifndef SYSTEM_MQTT_H
#define SYSTEM_MQTT_H

// From MQTT.h copied here so rest of files dont need to include MQTT.h just to subscribe
typedef void (*MQTTClientCallbackSimple)(String &, String &);
typedef void (*InputReceivedCallback)(String &);

namespace xMqtt {
// Note, there are many of these to make sensor code simple and not duplicate conversions.
void messageSend(String &topic, String &payload, const bool retain, const int qos);
void messageSend(const char *topic, String &payload, const bool retain, const int qos);
void messageSend(String &topic, const float &value, const int width, const bool retain, const int qos);
void messageSend(const char *topic, const float &value, const int width, const bool retain, const int qos);
void messageSend(String &topic, const int value, const bool retain, const int qos);
void messageSend(const char *topic, const int value, const bool retain, const int qos);
void subscribe(String &topic, InputReceivedCallback cb);
void subscribe(const char* topic, InputReceivedCallback cb);
void setup();
void loop();
} // namespace xMqtt

#endif // SYSTEM_MQTT_H
