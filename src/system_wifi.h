#ifndef SYSTEM_WIFI_H
#define SYSTEM_WIFI_H

#include "system_base.h"

class System_WiFi : public System_Base {
  public:
    // Customized variables from configuration
    String mqtt_host;
    String device_name;
    String discovery_project;
    System_WiFi();
    String &clientid();
    void checkConnected();
    bool prepareForLightSleep();
    bool recoverFromLightSleep();
    bool scanConnectOneAndAll();
    void setup();
  private:

    bool connect();
    bool connect1();
    bool reconnectWiFi(); // Try hard to reconnect WiFi
    void setupLanguages();
};

#endif // SYSTEM_WIFI_H