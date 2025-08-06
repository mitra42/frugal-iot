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
 * 
 * Acknowledgement to the ESP-WiFiSettings library from which some snippets have been used
 */

#include <DNSServer.h>
#if defined(ESP32) || defined(LIBRETINY)
#include <AsyncTCP.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
// Handle some arbitrary differences between AsyncTCP.h and ESPAsyncTCP.h
#define wifi_auth_mode_t uint8_t    // wl_enc_type
#define WIFI_AUTH_OPEN ENC_TYPE_NONE
constexpr auto WIFI_AUTH_WPA2_ENTERPRISE = -1337; // not available on ESP8266
#elif defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350)
// Untested - these just come from the example
//#include <WiFi.h>
#endif

#include "ESPAsyncWebServer.h"
#include "system_captive.h"
#include "system_language.h" // for Texts
#include "misc.h" // for Sprintf
#include "system_frugal.h" // for frugal_iot

static DNSServer dnsServer;
static AsyncWebServer server(80);

String html_entities(const String& raw) {
    String r;
    for (unsigned int i = 0; i < raw.length(); i++) {
        char c = raw.charAt(i);
        if (c < '!' || c == '"' || c == '&' || c == '\'' || c == '<' || c == '>' || c == 0x7f) {
            // ascii control characters, html syntax characters, and space
            r += Sprintf("&#%d;", c);
        } else {
            r += c;
        }
    }
    return r;
}
// For debugging - as hard to view source on mobile
//#define RESPONSEPRINT Serial.print
//#define RESPONSEPRINT response->print
class CaptiveRequestHandler : public AsyncWebHandler {
 public:
  bool canHandle(__unused AsyncWebServerRequest *request) const override {
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    Serial.println("Captive: Handling request with captive portal");
    //Serial.print("XXX Host="); Serial.println(request->host());
    //Serial.print("XXX User-Agent:"); Serial.println(request->getHeader("User-Agent")->value());
    String ip = WiFi.softAPIP().toString();
    //Serial.print("XXX IP="); Serial.println(ip);
    if (request->host() != ip) {
      // iPhone doesn't deal well with redirects to http://hostname/ and
      // will wait 40 to 60 seconds before succesful retry. Works flawlessly
      // with http://ip/ though.
      //Serial.println("XXX Not Matching host and ip - i.e. its not accessing by IP address");  
      // Anecdotally (according to WiFiSettings library), some devices require a non-empty response body
      AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", ip);
      response->addHeader("Location", "http://" + ip + "/");
      request->send(response);
    } else {
      //Serial.println("XXX Matching host and ip - i.e. acessed by IP address, not name");      
      AsyncResponseStream *response = request->beginResponseStream("text/html");
      response->print(F("<!DOCTYPE html><html><head><title>"));
      response->print(T->CaptivePortal);
      response->print(F("</title></head><body>"));
      response->print(F(
            "<meta name=viewport content='width=device-width,initial-scale=1'>"
            "<style>"
            "*{box-sizing:border-box} "
            "html{background:#444;font:10pt sans-serif}"
            "body{background:#ccc;color:black;max-width:30em;padding:1em;margin:1em auto}"
            "a:link{color:#000} "
            "label{clear:both}"
            "select,input:not([type^=c]){display:block;width:100%;border:1px solid #444;padding:.3ex}"
            "input[type^=s]{display:inline;width:auto;background:#de1;padding:1ex;border:1px solid #000;border-radius:1ex}"
            "[type^=c]{float:left;margin-left:-1.5em}"
            ":not([type^=s]):focus{outline:2px solid #d1ed1e}"
            ".w::before{content:'\\26a0\\fe0f'}"
            "p::before{margin-left:-2em;float:left;padding-top:1ex}"
            ".i::before{content:'\\2139\\fe0f'}"
            ".c{display:block;padding-left:2em}"
            ".w,.i{display:block;padding:.5ex .5ex .5ex 3em}"
            ".w,.i{background:#aaa;min-height:3em}"
            "</style>"
      ));
      response->print(F("<form action=\"/restart\" method=post><input type=submit value=\""));
      response->print(T->RESTART);
      response->print(F("\"></form><hr>"
      ));
      //Dropdown of SSIDs (see WiFiSettings.cpp ~L310)
      response->print(F("<h1>"));
      response->print(T->ConnectToWiFi);
      response->print(F("</h1><form method=post action=\"/\"><label>"));
      response->print(T->WiFiNetwork);
      response->print(F("<select name=ssid onchange=\"document.getElementsByName('password')[0].value=''\">"
        "<option hidden>"));
      response->print(T->SelectOne);
      response->print(F("</option>"));
      while (WiFi.scanComplete() < 0) {  }; // TODO-153 careful in case this blocks everything mid-scan   
      for (int i = 0; i < frugal_iot.wifi->num_networks; i++) {
        String s = WiFi.SSID(i);
        wifi_auth_mode_t mode = WiFi.encryptionType(i); //uint8_t on ESP8266
        response->print(F("<option value='"));
        response->print(html_entities(WiFi.SSID(i)) + F("'")); // TODO-153 make this the topic
        if (s == WiFi.SSID()) { response->print(T->_selected); }
        response->print(F(">"));
        response->print(WiFi.SSID(i));
        response->print(mode == WIFI_AUTH_OPEN ? F("") : F("&#x1f512;")); // Lock icon
        if (mode == WIFI_AUTH_WPA2_ENTERPRISE) { response->print(F(" unsupported 802.1x")); }
        response->print(F("</option>"));
      }
      response->print(F("</select></label>"));
      response->print(F("<label>"));
      response->print(T->Password);
      response->print(F(":<input name=password value=''></label><hr>")); // TODO-153 what if have password already

      //TODO-153 add dropdown of languages (see WiFiSettings.cpp ~L360)
      //TODO-153 add multiple lines from modules - may actually loop here  (see WiFiSettings.cpp ~L376)

      frugal_iot.captiveLines(response); // Calls back to captive.string etc 

      response->print(F(
        "<p style='position:sticky;bottom:0;text-align:right'>"
        "<input type=submit value=\""));
      response->print(T->SAVE);
      response->print(F("\" style='font-size:150%'></form>" //TODO-TRANSLATE
      ));
      response->print("</body></html>");
      request->send(response);
    }
  }
};


System_Captive::System_Captive()
: System_Base("captive", "Captive") {}

void System_Captive::setup() {
  Serial.println("Configuring access point...");
  setupLanguages();
  readConfigFromFS(); // Reads config (hostname) and passes to our dispatchTwig (Must come AFTER setting language strings)

  // Unsure what these items choose - copied from example -SOC_WIFI_SUPPORTED seems to be defined for ESP32
  #if SOC_WIFI_SUPPORTED || CONFIG_ESP_WIFI_REMOTE_ENABLED || LT_ARD_HAS_WIFI
    if (!WiFi.softAP(frugal_iot.nodeid)) { //TODO-153 add password as option (see WiFiSettings)
      Serial.println("Soft AP creation failed.");
      return;  // Shouldn't happen
    }
    dnsServer.start(53, "*", WiFi.softAPIP());
  #endif
  String ip = WiFi.softAPIP().toString(); // TODO-153 note how this is used by redirect 
  Serial.print(F("Access point on:")); Serial.println(ip);

  // Order is important - this has to come BEFORE the catch-all default portal
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("POST /");
    int params = request->params();
    for(int i=0;i<params;i++){
      const AsyncWebParameter* p = request->getParam(i);
      #ifdef ASYNCWEBSERVER_NEEDS_FILE_UPLOAD
        if(p->isFile()){ //p->isPost() is also true
          Serial.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
        } else 
      #endif
        if(p->isPost()){
          Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
          if (p->name() == "ssid") {
            const AsyncWebParameter* password = request->getParam("password", true);
            if (password && password->value().length()) {
              frugal_iot.dispatchTwig("wifi", p->value(), password->value(), true);
              // TODO-153 may wish to force it to try this new one
            } else {
              Serial.println("SSID But No password");
            }
            // Even if no password - can try switching WiFi
            frugal_iot.wifi->switchSSID(p->value());
          } else if (p->name() == "password") {
            // Ignore - will be read by "ssid" above
          } else {
            frugal_iot.dispatchTwig(p->name(), p->value(), true);
          }
        }
        #ifdef ASYNCWEBSERVER_NEEDS_FILE_GETPARAMETERS
          else {
            Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
          }
        #endif
    } //for
    request->send(200, "text/plain", "Settings updated");
  });

  server.on("/restart", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("POST /restart");
    request->send(200, "text/plain",T->RestartingPleaseWait); //TODO-153 translate
    // May need a callback here to do certain things before restarting 
    ESP.restart();
  });
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);  // only when requested from AP
  // more handlers...
  server.begin();
}

// TODO may need to wrap labels etc in html_entities()
void System_Captive::addString(AsyncResponseStream* response, const char* id, const char* topicTwig, String init, String label, uint8_t min_length, uint8_t max_length) {
  response->print(String(F("<p><label>")) + label + ":<br><input name='" + id + "/" + topicTwig + "' value='" + init + "' minlength=" + min_length + " maxlength=" + max_length + "></label>");  
}
/* Untested number and bool */
void System_Captive::addNumber(AsyncResponseStream* response, const char* id, const char* topicTwig, String init, String label, long min, long max) {
  response->print(String(F("<p><label>")) + label + ":<br><input type=number step=1 name='" + id + "/" + topicTwig + "' value='" + init + "' min=" + min + " max=" + max + "></label>");  
}
void System_Captive::addBool(AsyncResponseStream* response, const char* id, const char* topicTwig, String init, String label, long min, long max) {
  response->print(String(F("<p><label><input type=checkbox name='")) + id + "/" + topicTwig + "' value='" + init + (init ? " checked" : "") + ">" + label + "</label>");  
}
/* Example code to add handler

// save callback for particular URL path
auto handler = server.on("/some/path", [](AsyncWebServerRequest *request){
  //do something useful
});
// upload a file to /upload
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200);
  }, onUpload);

*/

bool System_Captive::setLanguage(const String& payload) {
  for (auto t : TT) {
    if (payload == t->code) {
      language_code = payload;
      T = t;
      return true;
    }
  }
  return false;
}

void System_Captive::dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) {
  if (isSet && (topicSensorId == id)) {
    if (topicTwig == "language_code") {
      if (setLanguage(payload)) { // Code e.g. "EN"
        writeConfigToFS(topicTwig, payload);
      }
    } else {
      System_Base::dispatchTwig(topicSensorId, topicTwig, payload, isSet);
    }
  }
}

void System_Captive::captiveLines(AsyncResponseStream* response) {
  response->print(String(F("<p><label>"))
    + T->Language // Language in the current language
    + ":<br><select name=language_code>");
  for (auto& t : TT) {
    response->print(String(F("<option value='")) + t->code + "'" 
    + ((t == T) ? "' selected" : "") + ">" 
    + t->LanguageName + "</option>");  // name in its own language
  }
  response->print(String(F("</select></label>")));
}

void System_Captive::loop() {
  dnsServer.processNextRequest(); // Apparantly does nothing, and not needed
}
