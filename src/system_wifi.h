#ifndef SYSTEM_WIFI_H
#define SYSTEM_WIFI_H

namespace xWifi {
// Customized variables from configuration
extern String mqtt_host;
extern String discovery_project;
extern String device_name;

void setup();
bool connect();
void checkConnected();
String &clientid();
} // namespace xWifi
#endif // SYSTEM_WIFI_H