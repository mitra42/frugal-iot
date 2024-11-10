#ifndef SYSTEM_MQTT_H
#define SYSTEM_MQTT_H

// From MQTT.h copied here so rest of files dont need to include MQTT.h just to subscribe
typedef void (*MQTTClientCallbackSimple)(String &, String &);

namespace xMqtt {
void messageSend(String &topic, String &payload, bool retain, int qos);
void messageSend(String &topic, float &value, int width, bool retain, int qos);
void messageSend(String &topic, int value, bool retain, int qos);
void subscribe(String &topic, MQTTClientCallbackSimple cb);

void setup();
void loop();
} // namespace xMqtt

#endif // SYSTEM_MQTT_H
