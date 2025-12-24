/* Frugal IoT - captive portal
 * 
 * This is a trivial captive portal, that allows for configuration etc. 
 * 
 * Its main use is for setting the WiFi settings.
 * 
 * BUT .... this is extensible, at some point we might start doing any or all of the following.
 * - displaying current data
 * - informing about version, configured devices and their parameters
 * - downloading saved data files
 * 
 * Its based on the example that comes with ESPAsyncWebServer 
 * https://github.com/ESP32Async/ESPAsyncWebServer/blob/main/examples/CaptivePortal/CaptivePortal.ino
 */
#ifndef SYSTEM_CAPTIVE_H
#define SYSTEM_CAPTIVE_H

#include "system_base.h"
#include "_settings.h" // For LANGUAGE_ALL



class System_Captive : public System_Base {
  public:
    String language_code;
    System_Captive();
    void setup() override;
    void setupLanguages();
    void loop() override;  /* Not String* */
    void addString(AsyncResponseStream* response, const char* id, const char* topicTwig, String init, String label, uint8_t min_length, uint8_t max_length);
    void addNumber(AsyncResponseStream* response, const char* id, const char* topicTwig, String init, String label, long min, long max);
    void addBool(AsyncResponseStream* response, const char* id, const char* topicTwig, bool init, String label);
    void addButton(AsyncResponseStream* response, const char* id, const char* topicTwig, String val, String label);
    bool setLanguage(const String& language_code);
    void dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet);
    void captiveLines(AsyncResponseStream* response);
  protected:
  };

#endif // SYSTEM_CAPTIVE_H
