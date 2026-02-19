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
#ifdef ESP8266
    #define INADDR_NONE IPAddress(0,0,0,0)
#endif
#include "ESPAsyncWebServer.h"
#include "system_captive.h"
#include "system_language.h" // for Texts
#include "misc.h" // for Sprintf
#include "system_frugal.h" // for frugal_iot

static DNSServer dnsServer;
static AsyncWebServer server(80);
String message; // Not in class, as accessed from Lambda functions

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
    Serial.println(F("Captive: Handling request with captive portal"));
    //Serial.print(F("XXX Host=")); Serial.println(request->host());
    //Serial.print(F("XXX User-Agent:")); Serial.println(request->getHeader("User-Agent")->value());
    String ip = WiFi.softAPIP().toString();
    //Serial.print(F("XXX IP=")); Serial.println(ip);
    if (request->host() != ip) {
      // iPhone doesn't deal well with redirects to http://hostname/ and
      // will wait 40 to 60 seconds before succesful retry. Works flawlessly
      // with http://ip/ though.
      //Serial.println(F("XXX Not Matching host and ip - i.e. its not accessing by IP address"));  
      // Anecdotally (according to WiFiSettings library), some devices require a non-empty response body
      AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", ip);
      response->addHeader("Location", "http://" + ip + "/");
      request->send(response);
    } else {
      // BULD THE CAPTIVE PORTAL HERE
      //Serial.println(F("XXX Matching host and ip - i.e. acessed by IP address, not name"));      
      AsyncResponseStream *response = request->beginResponseStream("text/html; charset=utf-8");
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
            "input[type^=s],input[type^=b]{display:inline;width:auto;background:#de1;padding:1ex;border:1px solid #000;border-radius:1ex}"
            "[type^=c]{float:left}"
            ":not([type^=s]):focus{outline:2px solid #d1ed1e}"
            ".w::before{content:'\\26a0\\fe0f'}"
            "p::before{margin-left:-2em;float:left;padding-top:1ex}"
            ".i::before{content:'\\2139\\fe0f'}"
            ".c{display:block;padding-left:2em}"
            ".w,.i{display:block;padding:.5ex .5ex .5ex 3em}"
            ".w,.i{background:#aaa;min-height:3em}"
            "</style>"
      ));
      response->print(F("<script>function s(name,value){let fd=new FormData();fd.set(name,value);fetch('/',{method:'POST',body:fd,credentials:'same-origin'}).catch(e=>console.error(e));};</script>"));
      if (message) {
        response->print(F("<h4>"));
        response->print(message);
        response->print(F("</h4>"));
        message = "";
      }
      response->print(F("<form action=\"/restart\" method=post><input type=submit value=\""));
      response->print(T->RESTART);
      response->print(F("\"></form><hr>"));

      //Dropdown of SSIDs (see WiFiSettings.cpp ~L310)
      response->print(F("<form method=post action=\"/\"><label>"));
      response->print(T->WiFiNetwork);
      response->print(F("<select name=ssid onchange=\"document.getElementsByName('password')[0].value=''\">"
        "<option hidden>"));
      response->print(T->SelectOne);
      response->print(F("</option>"));
      while (WiFi.scanComplete() < 0) {  }; // Careful in case this blocks everything mid-scan, seems to be ok.
      for (int i = 0; i < frugal_iot.wifi->num_networks; i++) {
        String s = WiFi.SSID(i);
        uint8_t bars =  frugal_iot.wifi->rssi_to_bars(WiFi.RSSI(i));
        wifi_auth_mode_t mode = WiFi.encryptionType(i); //uint8_t on ESP8266
        response->print(F("<option value='"));
        response->print(html_entities(WiFi.SSID(i)));
        response->print(F("'"));
        if (s == WiFi.SSID()) { response->print(F(" selected")); }
        response->print(F(">"));
        response->print(WiFi.SSID(i));
        if (!WIFI_AUTH_OPEN) { response->print(F(" &#x1f512;")); }  // Lock icon
        const char* bb[] = {" ", "&#x2804", "&#x2806", "&#x2807", "&#x2847", "&#x283f" };
        for (uint8_t b = 0; b <= bars; b++) {
          response->print(bb[b]); // e.g. .))) if three bars (ugly but saves images on device)
        }
        if (mode == WIFI_AUTH_WPA2_ENTERPRISE) { response->print(F(" unsupported 802.1x")); }
        response->print(F("</option>"));
      }
      response->print(F("</select></label>"));
      response->print(F("<label>"));
      response->print(T->Password);
      response->print(F(":<input name=password value=''></label>")); // TODO-153 what if have password already - maybe prefill with *** if known and check for this
      response->print(F(
        "<input type=submit value=\""));
      response->print(T->SETWIFI);
      response->print(F("\" style='font-size:100%'></form><hr>"));

      // Each captive line should be of form from addString etc below 
      // <p><label>...:<br><input...></label></p>  
      frugal_iot.captiveLines(response); // Calls back to captive.string etc 
      response->print(F("</body></html>"));
      request->send(response);
    }
  }
};


System_Captive::System_Captive()
: System_Base("captive", "Captive") {}

void System_Captive::setup() {
  Serial.println(F("Configuring access point..."));
  setupLanguages();
  readConfigFromFS(); // Reads config (language_code) and passes to our dispatchTwig (Must come AFTER setting language strings)

  // Unsure what these items choose - copied from example -SOC_WIFI_SUPPORTED seems to be defined for ESP32
  #if SOC_WIFI_SUPPORTED || CONFIG_ESP_WIFI_REMOTE_ENABLED || LT_ARD_HAS_WIFI || defined(ESP8266)
    if (!WiFi.softAP(frugal_iot.nodeid)) { //TODO-153 add password as option (see WiFiSettings)
      Serial.println(F("Soft AP creation failed."));
      return;  // Shouldn't happen
    }
    dnsServer.setTTL(0); //grabbed from old WiFiSettings  - unclear if needed or useful
    dnsServer.start(53, "*", WiFi.softAPIP());
  #endif
  String ip = WiFi.softAPIP().toString(); // Note how this is used by redirect 
  Serial.print(F("Access point on: ")); Serial.println(ip);

  // Order is important - this has to come BEFORE the catch-all default portal
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println(F("POST /"));
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
              frugal_iot.messages->queueFromCaptive("set/wifi/" + p->value(), password->value());
              // TODO-153 may wish to force it to try this new one
            } else {
              Serial.println(F("SSID But No password"));
            }
            // Even if no password - can try switching WiFi
            frugal_iot.wifi->switchSSID(p->value());
          } else if (p->name() == "password") {
            // Ignore - will be read by "ssid" above
          } else {
            // Special case language as need before resend captive portal in new language, but still queue as cannot write to disk inside request handler.
            if (p->name() == "captive/language_code") {
              frugal_iot.captive->setLanguage(p->value());
            }
            frugal_iot.messages->queueFromCaptive("set/" + p->name(), p->value());
          }
        }
        #ifdef ASYNCWEBSERVER_NEEDS_FILE_GETPARAMETERS
          else {
            Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
          }
        #endif
    } //for
    // Redisplay main screen
    //request->send(200, "text/plain", "Settings updated");
    String ip = WiFi.softAPIP().toString();
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", ip);
    response->addHeader("Location", "http://" + ip + "/");
    // TODO rebuilding new screen before process queued incoming 

    request->send(response);
    message = T->SettingsUpdated;
  });

  server.on("/restart", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println(F("POST /restart"));
    request->send(200, "text/plain",T->RestartingPleaseWait);
    // May need a callback here to do certain things before restarting 
    ESP.restart();
  });
  // more handlers...
  // This should be after the specific ones as it handles anything
  #ifdef ESP8266 // debugging - suggestion by Chat GPT as note that handler called
    server.addHandler(new CaptiveRequestHandler());  // only when requested from AP
  #else // This version works fine on ESP32 and is probably the default
    server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);  // only when requested from AP
  #endif
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.println(F("XXX NotFound handler called - shouldnt happen"));
  });
  server.begin();
}

// TODO may need to wrap labels etc in html_entities()

// String input
void System_Captive::addString(AsyncResponseStream* response, const char* id, const char* topicTwig, String init, String label, uint8_t min_length, uint8_t max_length) {
  response->print(String(F("<p><label>")) + label + ":<br><input name='" + id + "/" + topicTwig + "' value='" + init + "' minlength=" + min_length + " maxlength=" + max_length + " onchange=\"s(this.name,this.value)\"></label></p>");  
  // Chat GPT suggested:   + "onkeydown=\"if(event.key==='Enter'||event.keyCode==13){s(this.form);event.preventDefault();return false;}\">"  
}
// Number input
void System_Captive::addNumber(AsyncResponseStream* response, const char* id, const char* topicTwig, String init, String label, long min, long max) {
  response->print(String(F("<p><label>")) + label + ":<br><input type=number step=1 name='" + id + "/" + topicTwig + "' value='" + init + "' min=" + min + " max=" + max + " onchange=\"s(this.name,this.value)\"></label></p>");  
}
// Bool input - e.g. LED
void System_Captive::addBool(AsyncResponseStream* response, const char* id, const char* topicTwig, bool init, String label) {
  // weirdness here is because absence of check, means absence of input being sent, so send 0 with optional 1 following
  //response->print(String(F("<input type=hidden name='" ))+ id + "/" + topicTwig + "' value='0'>");  
  response->print(String(F("<p><label>")) + label + ": <input type=checkbox name='" + id + "/" + topicTwig + "' value='1'" + (init ? " checked" : "") + " onchange=\"s(this.name,this.checked?this.value:'0')\"></label></p>");  
}
void System_Captive::addButton(AsyncResponseStream* response, const char* id, const char* topicTwig, String val, String label) {
  response->print(String(F("<input type=button name='")) + id + "/" + topicTwig + "' value=\"" + label + "\" onclick=\"s(this.name,'" + val +"')\">");
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
  response->print(F("<form action=\"/\" method=post><p><label>"));
  response->print(T->Language); // Language in the current language
  response->print(F(":<br><select name=captive/language_code onchange=\"this.form.submit()\">")); 
  for (auto& t : TT) {
    response->print(String(F("<option value='")) + t->code + "'" 
    + ((t == T) ? "' selected" : "") + ">" 
    + t->LanguageName + "</option>");  // name in its own language
  }
  response->print(String(F("</select></label></p></form>")));
}

void System_Captive::loop() {
  dnsServer.processNextRequest(); // Apparantly does nothing, and not needed
}
