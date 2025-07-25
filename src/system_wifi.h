#ifndef SYSTEM_WIFI_H
#define SYSTEM_WIFI_H

#include "system_base.h"

enum WiFiStatusType { 
  WIFI_STARTING, WIFI_DISCONNECTED, WIFI_RECONNECTING, WIFI_CONNECTED, WIFI_NEEDSCAN, WIFI_SCANNING, WIFI_SCANNED, WIFI_CONNECTING, WIFI_STABILIZING
};

//TODO-152 OTA is trying before WiFi is up

class System_WiFi : public System_Base {
  public:
    WiFiStatusType status = WIFI_STARTING ;
    unsigned long statusSince = 0;
    int16_t num_networks = -1;
    int32_t minRSSI; // Minimum RSSI we are trying to connect to in this cycle of state Machine
    int nextNetwork;
    System_WiFi();
    void switchSSID(const String ssid);
    bool connectOne(String ssid, int32_t rssi = 0);
    void connectInnerAsync(String ssid, String pw);
    //bool connect1(String ssid, String pw, int wait_seconds=30);
    bool connectOneAndAllNext();
    void connectOneAndAllReset();
    void setStatus(WiFiStatusType newstatus);
    bool connected();
    void stateMachine();
    bool rescan();
    //void periodically() override;
    void loop() override;
    bool prepareForLightSleep();
    bool recoverFromLightSleep();
    void setup() override;
    void addWiFi(String ssid, String password);
    void dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) override;
};

#endif // SYSTEM_WIFI_H