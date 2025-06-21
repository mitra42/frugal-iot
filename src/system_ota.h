#ifndef SYSTEM_OTA_H
#define SYSTEM_OTA_H

#include <Arduino.h>

#ifndef SYSTEM_OTA_MS
  // By default, check for updates once an hour - override in platformio.ini
  #define SYSTEM_OTA_MS 3600000
#endif // SYSTEM_OTA_MS

class System_OTA : public System_Base {
  public:

    System_OTA();
    ~System_OTA();
    void init(const String otaServerAddress, const String softwareVersion, const char* caCert);
    void checkForUpdate(void);
	  bool isOK() { return _isOK; }
	  bool canRetry() { return _retryCount > 0; }
	  bool checked() { return _checked; }
	
    friend void otaStartCB(void);
    friend void otaProgressCB(int done, int size);
    friend void otaEndCB(void);
    friend void otaErrorCB(int errorCode);

    void setup_after_wifi();
    void infrequently();

  private:
    unsigned long nextLoopTime = SYSTEM_OTA_MS; // Dont activate on first loop - as happens in Setup  sleepSafeMillis()
    bool _isOK;
    bool _checked;
    int _retryCount;
    const char* _caCert;
    String _otaServerAddress;
    String _softwareVersion;
    char* getOTApath();
	
};

#endif // SYSTEM_OTA_H
