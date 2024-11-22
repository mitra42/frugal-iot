#ifndef CONTROL_DEMO_MQTT_H
#define CONTROL_DEMO_MQTT_H

#define CONTROL_DEMO_MQTT_ADVERTISEMENT "\n  -\n    topic: control_demo_mqtt_output\n    name: Control Demo MQTT output\n    type: topic\n    options: bool\n    display: dropdown\n    rw: rw"

namespace cDemoMqtt {
void setup();
//void loop();
} // namespace cDemoMqtt
#endif // CONTROL_DEMO_MQTT_H