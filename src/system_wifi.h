#ifndef SYSTEM_WIFI_H
#define SYSTEM_WIFI_H

// TODO this should be a class ! 
namespace xWifi {
// Customized variables from configuration
extern String mqtt_host;
extern String discovery_project;
extern String device_name;

void setup();
bool connect();
void checkConnected();
bool reconnectWiFi(); // Try hard to reconnect WiFi
bool prepareForLightSleep();
bool recoverFromLightSleep();
String &clientid();
} // namespace xWifi
#endif // SYSTEM_WIFI_H