#ifndef SYSTEM_WIFI_H
#define SYSTEM_WIFI_H

#include "system_base.h"

class System_WiFi : public System_Base {
  public:
    System_WiFi();
    String &clientid();
    void checkConnected();
    bool prepareForLightSleep();
    bool recoverFromLightSleep();
    bool scanConnectOneAndAll();
    void setup();
    void addWiFi(String ssid, String password);
  private:

    bool connect();
    bool connect1();
    bool reconnectWiFi(); // Try hard to reconnect WiFi
    void setupLanguages();
};

#endif // SYSTEM_WIFI_H