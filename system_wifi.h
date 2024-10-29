#ifndef SYSTEM_WIFI_H
#define SYSTEM_WIFI_H

namespace xWifi {
// Customized variables from configuration
extern String mqtt_host;

void setup();
void connect();
void checkConnected();
String &clientid();
//void loop();
} // namespace xWifi
#endif // SYSTEM_WIFI_H