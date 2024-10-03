#ifndef SYSTEM_MQTT_H
#define SYSTEM_MQTT_H

namespace xMqtt {
void messageSend(String &topic, String &payload);
void messageSend(String &topic, float &value, int width);
void messageSend(String &topic, int value);
void subscribe(String &topic, MQTTClientCallbackSimple cb);

void setup();
void loop();
} // namespace xMqtt

#endif // SYSTEM_MQTT_H
