// Deep Sleep issues: none - the AP and web server are rebuilt in setup(), so anyone browsing it is disconnected for the sleep.
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

#include "system/base.h"
#include "system/io.h"
#include "_settings.h" // For LANGUAGE_ALL



String html_entities(const String& raw); // Escape text for the portal's HTML - captive.cpp

#ifndef SYSTEM_CAPTIVE_STATUS_REFRESH_MS
  // How often the portal refreshes its WiFi and MQTT status lines - the feedback for "Set WiFi"
  #define SYSTEM_CAPTIVE_STATUS_REFRESH_MS 3000
#endif

/* The body of a /values/<id> reply: one JSON object of "topicTwig":"value" members, written
 * straight to the response as each module adds to it - see System_Base::captiveValues().
 */
class Captive_Values {
  public:
    explicit Captive_Values(Print* out);
    void add(const String& topicTwig, const String& value);
    void add(IO* io); // Its topicTwig and StringValue()
    void end();
  private:
    Print* out;
    bool first = true;
};

class System_Captive : public System_Base {
  public:
    String language_code;
    System_Captive();
    void setup() override;
    void setupLanguages();
    void loop() override;  /* Not String* */
    void addString(AsyncResponseStream* response, const char* id, const char* topicTwig, String init, String label, uint8_t min_length, uint8_t max_length);
    // refresh: the IO whose value the portal should keep this field showing, if any
    void addNumber(AsyncResponseStream* response, const char* id, const char* topicTwig, String init, String label, long min, long max, IO* refresh = nullptr);
    // Open the <div> System_Group wraps around a refreshable module's lines; caller closes it
    void startRefresh(Print* out, const char* moduleId, unsigned long refresh_ms);
    // An IO's value as a <span> the page refreshes - for a module's read-only captiveLines()
    String valueSpan(IO* io);
    void addBool(AsyncResponseStream* response, const char* id, const char* topicTwig, bool init, String label);
    void addButton(AsyncResponseStream* response, const char* id, const char* topicTwig, String val, String label);
    bool setLanguage(const String& language_code);
    void statusLines(Print* out, bool full) override;
    void dispatch(System_Message &msg) override;
    void captiveLines(AsyncResponseStream* response);
    void captiveValues(Captive_Values* values, const String& moduleId) override;
    // Register a POST handler on the station WiFi interface (used by System_MDNS)
    void addSTARoute(const char* uri, ArRequestHandlerFunction handler);
    bool okToSleep(unsigned long ms, uint16_t how) override;
    // millis() of the last request from a client on our AP, 0 if none - set from the async_tcp task
    volatile unsigned long lastActivityMs = 0;
  protected:
  };

#endif // SYSTEM_CAPTIVE_H
