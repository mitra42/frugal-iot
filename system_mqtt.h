#ifndef SYSTEM_MQTT_H
#define SYSTEM_MQTT_H

// From MQTT.h copied here so rest of files dont need to include MQTT.h just to subscribe
typedef void (*MQTTClientCallbackSimple)(String &, String &);

#define CONTROL_DEMO_MQTT_ADVERTISEMENT "\n  -\n    topic: control_demo_mqtt_output\n    name: Control Demo MQTT output\n    type: topic\n    options: bool\n    display: dropdown\n    rw: rw"

namespace xMqtt {
void messageSend(String &topic, String &payload, const bool retain, const int qos);
void messageSend(String &topic, const float &value, const int width, const bool retain, const int qos);
void messageSend(String &topic, const int value, const bool retain, const int qos);
void subscribe(String &topic, MQTTClientCallbackSimple cb);

void setup();
void loop();
} // namespace xMqtt

#endif // SYSTEM_MQTT_H
