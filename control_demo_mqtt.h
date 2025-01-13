#ifndef CONTROL_DEMO_MQTT_H
#define CONTROL_DEMO_MQTT_H

// TODO this advertisement needs to be kept in line automatically with the .cpp 
#define CONTROL_DEMO_MQTT_ADVERTISEMENT "\n  -\n    topic: control_demo_mqtt_outputControl\n    name: Control Demo MQTT output\n    type: topic\n    options: bool\n    display: dropdown\n    rw: rw" \
                                        "\n  -\n    topic: control_demo_mqtt_input2\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: rw"

namespace cDemoMqtt {
void setup();
//void loop();
void dispatchPath(const String &topicpath, const String &payload);
void dispatchLeaf(const String &topicleaf, const String &payload);
} // namespace cDemoMqtt
#endif // CONTROL_DEMO_MQTT_H