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
#elif defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350)
// Untested - these just come from the example
//#include <RPAsyncTCP.h>
//#include <WiFi.h>
#endif

#include "ESPAsyncWebServer.h"
#include "system_captive.h"
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
    Serial.print("XXX Host="); Serial.println(request->host());
    Serial.print("XXX User-Agent:"); Serial.println(request->getHeader("User-Agent")->value());
    String ip = WiFi.softAPIP().toString();
    Serial.print("XXX IP="); Serial.println(ip);
    if (request->host() != ip) {
      // iPhone doesn't deal well with redirects to http://hostname/ and
      // will wait 40 to 60 seconds before succesful retry. Works flawlessly
      // with http://ip/ though.
      Serial.println("XXX Not Matching host and ip - i.e. its not accessing by IP address");  
      // Anecdotally (according to WiFiSettings library), some devices require a non-empty response body
      AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", ip);
      response->addHeader("Location", "http://" + ip + "/");
      request->send(response);
    } else {
      Serial.println("XXX Matching host and ip - i.e. acessed by IP address, not name");      
      AsyncResponseStream *response = request->beginResponseStream("text/html");
      response->print(F("<!DOCTYPE html><html><head><title>Captive Portal</title></head><body>"));
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
      response->print(F( //TODO-153-TRANSLATE
        "<form action=\"/restart\" method=post><input type=submit value=\"RESTART\"></form><hr>"
      ));
      //Dropdown of SSIDs (see WiFiSettings.cpp ~L310)
      response->print(F(
        "<h1>Connect to WiFi</h1>"
        "<form method=post action=\"/\">"
        "<label>WiFi Network"
        "<select name=ssid onchange=\"document.getElementsByName('password')[0].value=''\">"
        "<option hidden>Select one...</option>"
      ));
      while (WiFi.scanComplete() < 0) {  }; // TODO-153 careful in case this blocks everything mid-scan   
      for (int i = 0; i < frugal_iot.wifi->num_networks; i++) {
        String s = WiFi.SSID(i);
        wifi_auth_mode_t mode = WiFi.encryptionType(i);
        response->print(F("<option value='"));
        response->print(html_entities(WiFi.SSID(i)) + F("'")); // TODO-153 make this the topic
        if (s == WiFi.SSID()) { response->print(" selected"); }
        response->print(F(">"));
        response->print(WiFi.SSID(i));
        response->print(mode == WIFI_AUTH_OPEN ? F("") : F("&#x1f512;"));
        if (mode == WIFI_AUTH_WPA2_ENTERPRISE) { response->print(F(" unsupported 802.1x")); }
        response->print(F("</option>"));
      }
      response->print(F("</select></label>"));
      response->print(F("<label>Password:<input name=password value=''></label><hr>")); // TODO-153 what if have password already

      //TODO-153 add dropdown of languages (see WiFiSettings.cpp ~L360)
      //TODO-153 add multiple lines from modules - may actually loop here  (see WiFiSettings.cpp ~L376)

      frugal_iot.captiveLines(response); // Calls back to captive.string etc 

      response->print(F(
        "<p style='position:sticky;bottom:0;text-align:right'>"
        "<input type=submit value=\"SAVE\" style='font-size:150%'></form>" //TODO-TRANSLATE
      ));
      
      #ifdef INSTANDARDEXAMPLENOTUSEDHERE
        response->printf("<p>You were trying to reach: http://%s%s</p>", request->host().c_str(), request->url().c_str());
        #if SOC_WIFI_SUPPORTED || CONFIG_ESP_WIFI_REMOTE_ENABLED || LT_ARD_HAS_WIFI
          response->printf("<p>Try opening <a href='http://%s'>this link</a> instead</p>", WiFi.softAPIP().toString().c_str());
        #endif
      #endif // INSTANDARDEXAMPLENOTUSEDHERE
      response->print("</body></html>");
      request->send(response);
    }
  }
};


System_Captive::System_Captive()
: System_Base("captive", "Captive") {}

void System_Captive::setup() {
  Serial.println("Configuring access point...");

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
            Serial.println("XXX Got ssid"); 
            const AsyncWebParameter* password = request->getParam("password", true);
            if (password && password->value().length()) {
              Serial.println("XXX Adding wifi"); Serial.print(p->value()); Serial.print("="); Serial.println(password->value());
              frugal_iot.dispatchTwig("wifi", p->value(), password->value(), true);
              // TODO-153 may wish to force it to try this new one
            } else {
              Serial.println("XXX But No password");
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
    request->send(200, "text/plain", "restarting .... plesae wait"); //TODO-153 translate
    // May need a callback here to do certain things before restarting 
    ESP.restart();
  });
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);  // only when requested from AP
  // more handlers...
  Serial.println("XXX " __FILE__ "starting webserver");
  server.begin();
}

void System_Captive::string(AsyncResponseStream* response, const char* id, const char* topicTwig, String& init, const char* label, uint8_t min_length, uint8_t max_length) {
  response->print(String(F("<p><label>")) + label + ":<br><input name='" + id + "/" + topicTwig + "' value='" + init + "' minlenght=" + min_length + " maxlength=" + max_length + "></label>");  
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

void System_Captive::loop() {
  dnsServer.processNextRequest(); // Apparantly does nothing, and not needed
}


#ifdef NOTINCORPORATEDYET

void System_WiFi::setupLanguages() {
  // TODO-39 need to make sure external for language is set prior to this - get defined from platformio.h and LANGUAGE_ALL
  #ifdef LANGUAGE_DEFAULT
    WiFiSettings.language = LANGUAGE_DEFAULT; // This must happen BEFORE WiFiSettings.begin().
  #endif
  WiFiSettings.begin(); // WiFi has created variables - at this point any previous ssid and language are now set
  Serial.print(F("Language = ")); Serial.println(WiFiSettings.language);
  #if defined LANGUAGE_EN || defined LANGUAGE_ALL
    if (WiFiSettings.language == "en") {
      T.MqttServer = F("MQTT server");
      T.DeviceName = F("Device name");
      T.Project = F("Project");
    } 
  #endif
  #if defined LANGUAGE_DE || defined LANGUAGE_ALL
    // German settings all machine translated - confirmation from native German speaker, or better translations welcome
    if (WiFiSettings.language == "de") {
      T.MqttServer = F("MQTT server");
      T.DeviceName = F("Gerätename");
      T.Project = F("Projekt");
    }
  #endif
  #if defined LANGUAGE_NL || defined LANGUAGE_ALL
    // Dutch settings all machine translated - confirmation from native Dutch speaker, or better translations welcome
    if (WiFiSettings.language == "de") {
      T.MqttServer = F("MQTT server");
      T.DeviceName = F("Apparaatnaam");
      T.Project = F("Project");
    }
  #endif
  #if defined LANGUAGE_ID || defined LANGUAGE_ALL
    // Indonesian settings all machine translated - confirmation from native Bahasa speaker, or better translations welcome
    if (WiFiSettings.language == "id") {
      T.MqttServer = F("MQTT server");
      T.DeviceName = F("Nama Perangkat");
      T.Project = F("Proyek");
    }
  #endif
}

void System_WiFi::setup() {
  setupLanguages(); // Must come before any calls to WiFiSettings.<anything> 

  // This may be confusing ! 
  // Each line initializes a variable to the existing value, 
  // but override from LittleFS if available, 
  // then adds a line to the WiFi portal that can be used to set the file value, 
  // which will be used after the reboot.

  // Custom configuration variables, these will read configured values if previously set and return default values if not.
  /*
    int integer(String name, [long min, long max,] int init = 0, String label = name);
    String string(String name, [[unsigned int min_length,] unsigned int max_length,] String init = "", String label = name);
    bool checkbox(String name, bool init = false, String label = name);
  */

  frugal_iot.mqtt->hostname = WiFiSettings.string(F("mqtt/hostname"), 4,40, frugal_iot.mqtt->hostname, T.MqttServer); 
  // TODO-29 turn projet into a dropdown, use an ifdef for the ORGANIZATION in _locals.h not support by ESPWiFi-Settings yet.
  frugal_iot.project = WiFiSettings.string(F("frugal_iot/project"), 3,20, frugal_iot.project, T.Project); 
  frugal_iot.device_name = WiFiSettings.string(F("frugal_iot/device_name"), 3,20, frugal_iot.device_name, T.DeviceName); 
  #ifdef SYSTEM_WIFI_DEBUG
    Serial.print(F("MQTT host = ")); Serial.println(frugal_iot.mqtt->hostname);
    Serial.print(F("Project = ")); Serial.println(frugal_iot.project);
    Serial.print(F("Device Name = ")); Serial.println(frugal_iot.device_name);
  #endif


// These are the language texts to use 
struct Texts {
    const __FlashStringHelper
      *MqttServer,
      *DeviceName,
      *Project
    ;
    /*
    const char
        *init
    ;
    */
};
Texts T;

#endif //NOTINCORPORATEDYET