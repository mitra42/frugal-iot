/* MQTT client
* 
* Configuration
* Required: SYSTEM_MQTT_MS
* Optional: ESP8266 SYSTEM_MQTT_DEBUG 
* 
* Note definitions
* topicPath = full path /dev/project/node/sensor/leaf and usually String or String& or String*
* topicPrefix = /dev/project/node/ (note trailng slash)
* topicTwig (or Twig) is the components after the topicPrefix typically sensor/io and usually char*
* topicLeaf (or Leaf) is the last component e.g. "temperature" and usually char* (deprecated in favor of topicTwig) its "id" in any IO
* "topic" is ambiguous and therefore wrong ! 
*
* Incoming flow 
* messageReceived -> dispatch
*/

#include "_settings.h"
#ifdef ESP32
  #include "esp_task_wdt.h" // TODO-125
#endif

#include <MQTT.h>
#include "system/frugal.h" // for frugal_iot

// Enrolment talks HTTPS to the server, the same way OTA does - and reuses OTA's pinned root
// certificate, so there is one place that has to be right.
#ifdef ESP8266
  #include <ESP8266HTTPClient.h>
  #include <WiFiClientSecure.h>
#elif defined(ESP32)
  #include <HTTPClient.h>
  #include <WiFiClientSecure.h>
#endif

#include "system/rootca.h"

// The enrolment reply is {"username":"...","password":"..."} and nothing else, so it is read with
// indexOf rather than by adding a JSON parser to every image for this one request. Returns an empty
// String if the field is absent, which the caller treats as a failed enrolment.
static String jsonField(const String& json, const char* field) {
  String key = String("\"") + field + "\"";
  int k = json.indexOf(key);
  if (k < 0) return String();
  int colon = json.indexOf(':', k + key.length());
  if (colon < 0) return String();
  int open = json.indexOf('"', colon + 1);
  if (open < 0) return String();
  int close = json.indexOf('"', open + 1);
  if (close < 0) return String();
  return json.substring(open + 1, close);
}

/*
 * Where to enrol. A whole URL, not a path, and set at compile time the way
 * SYSTEM_OTA_SERVERPORTPATH is - for the same reason.
 *
 * It cannot be derived from the MQTT hostname, which is what the first version of this did. On
 * production the broker and the server happen to share a name and the server is HTTPS on 443, so
 * deriving it works by luck; anywhere else it does not. On a Pi the broker is
 * frugaliot.local:1883 and the server is frugaliot.local:8080 over plain HTTP, so a derived
 * "https://<broker host>/enrol" is wrong in the scheme AND the port.
 *
 * An http:// URL is honoured, which is what makes a local Pi testable. It sends the enrolment
 * secret in the clear - no worse than the rest of that link, where MQTT itself is plaintext on
 * 1883, but a deliberate step down from production and worth knowing you have taken it.
 */
#ifndef SYSTEM_MQTT_ENROL_URL
  #define SYSTEM_MQTT_ENROL_URL "https://frugaliot.naturalinnovation.org/enrol"
#endif

// How many consecutive "not authorised" answers before the stored credential is treated as dead.
// More than one, because a broker restarting can refuse a connection in passing; few enough that a
// node reset from the dashboard recovers in a minute or two rather than needing a visit.
#ifndef SYSTEM_MQTT_ENROL_RETRY_AFTER
  #define SYSTEM_MQTT_ENROL_RETRY_AFTER 5
#endif

#ifndef SYSTEM_MQTT_BACKOFF
  #define SYSTEM_MQTT_BACKOFF 5000 // Reasonable backoff on MQTT conncetion failure - 5 seconds (was 10ms ! )
#endif
#ifndef SYSTEM_MQTT_ENROL_RETRY_MS
  // How long between enrolment attempts. Half an hour: a node waiting to be approved on the
  // dashboard has to keep asking, because being approved is the only way it can come back, but it
  // may be waiting for days and there is nothing to be gained by asking often.
  #define SYSTEM_MQTT_ENROL_RETRY_MS 1800000
#endif
// mqtt client -> System_MQTT callback
// Note intentionally outside class, passed as callback to Mqtt client
void MqttMessageReceived(String &topicPath, String &payload) { // cant be constant as dispatch isnt
  frugal_iot.mqtt->messageReceived(topicPath, payload);
}

// Run every 10ms TODO-25 and TODO-23 this should be MUCH longer ideally
System_MQTT::System_MQTT(const char* hostname, const char* username, const char* password) 
: System_Base("mqtt", "MQTT"), 
  client(1024,128),
  hostname(hostname),
  ms(10),
  nextLoopTime(0), 
  password(password),
  username(username)
{}

void System_MQTT::setup() {
  readConfigFromFS(); // Reads config (hostname, and username/password if enrolled) into our dispatch
}

// Which credential to connect with. A stored one means this node has enrolled and has its own
// account; falling back to the compiled-in pair is what keeps a sketch using the older
// configure_mqtt(host, user, password) working, and is also the only thing available on a server
// too old to enrol.
const char* System_MQTT::mqttUsername() {
  return storedUsername.length() ? storedUsername.c_str() : username;
}
const char* System_MQTT::mqttPassword() {
  return storedPassword.length() ? storedPassword.c_str() : password;
}

/*
 * Ask the server for this node's own broker credential.
 *
 * Only if it has none: a credential in LittleFS survives a reboot, and re-enrolling would be
 * refused anyway unless the node proves it holds the current one (which it would, but there is no
 * reason to ask).
 *
 * Retried on a slow timer rather than once per boot, and called from loop() as well as at startup.
 * Three reasons, all of them nodes that could not otherwise come back: the server may simply have
 * been down when this node booted; a withdrawn enrolment secret is refused until an administrator
 * approves this node id on the dashboard, which may be hours later; and nobody can reach a node in
 * a field to restart it. Half an hour between attempts, and the server rate limits per organization
 * and per node id on top of that.
 *
 * The reply is two short strings, so it is read with indexOf rather than by adding a JSON parser to
 * every image for this one request.
 */
void System_MQTT::enrolIfNeeded() {
  if (!enrolmentSecret) return;                            // sketch uses the three-argument form
  if (storedUsername.length() && storedPassword.length()) return;   // already has one
  if (!frugal_iot.wifi->connected()) return;               // needs the network
  // Not once per boot: see nextEnrolTime in the header for why that stranded nodes. Subtract and
  // compare as signed, so the comparison still holds when millis() wraps at 49 days - a node
  // waiting to be approved is exactly the one that would still be waiting by then.
  if (nextEnrolTime && ((long)(millis() - nextEnrolTime) < 0)) return;
  nextEnrolTime = millis() + SYSTEM_MQTT_ENROL_RETRY_MS;

  const String url = String(SYSTEM_MQTT_ENROL_URL);
  const bool plainHttp = url.startsWith("http://");

  // One client or the other, chosen by the URL. Declared together rather than in branches because
  // HTTPClient::begin keeps a reference to whichever it is given, so it has to outlive the request.
  WiFiClient plain;
  WiFiClientSecure secure;
  if (!plainHttp) {
    #ifdef ESP32
      secure.setCACert(rootCAForServer());
    #elif defined(ESP8266)
      // Unverified, as OTA is on this chip - see SEC-10, accepted: few or no ESP8266s going
      // forward, and setting a trust anchor here has been observed to crash the ESP8266 TLS stack.
      secure.setInsecure();
    #endif
    secure.setTimeout(20000);
  } else {
    plain.setTimeout(20000);
  }
  // Not "net" - that is the member this class holds for the MQTT connection itself, and shadowing
  // it here would compile while meaning something else entirely.
  WiFiClient& httpNet = plainHttp ? plain : (WiFiClient&)secure;

  String body = String(F("{\"org\":\"")) + frugal_iot.org
              + F("\",\"project\":\"") + frugal_iot.project
              + F("\",\"nodeid\":\"") + frugal_iot.nodeid
              + F("\",\"enrolment_secret\":\"") + enrolmentSecret
              #ifdef SYSTEM_LORAMESHER_WANT
                // Declared by the BUILD, not by whether it happens to be acting as a gateway: any
                // node that sees WiFi can promote itself at runtime, so the broader publish a
                // gateway needs is granted by capability instead. See SECURITY-REVIEW.md S7.
                + F("\",\"lora\":true}")
              #else
                + F("\",\"lora\":false}")
              #endif
              ;

  HTTPClient http;
  Serial.print(F("Enrolling at ")); Serial.println(url);
  if (!http.begin(httpNet, url)) {
    Serial.println(F("Enrol: could not start the request"));
    return;
  }
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  String reply = (code > 0) ? http.getString() : String();
  http.end();

  if (code != 200) {
    // 409 means the server already has this node and wants proof we cannot give - which happens
    // when the filesystem has been erased. Say what to do about it rather than only the number.
    Serial.print(F("Enrol failed, HTTP ")); Serial.println(code);
    if (code == 409) {
      Serial.print(F("  This node is already enrolled. On the server run: frugal-iot-resetnode "));
      Serial.print(frugal_iot.org); Serial.print(F(" ")); Serial.print(frugal_iot.project);
      Serial.print(F(" ")); Serial.println(frugal_iot.nodeid);
    } else if (code == 403) {
      Serial.println(F("  The enrolment secret was refused - check configure_mqtt_enrolled()"));
    } else if (code > 0) {
      Serial.print(F("  ")); Serial.println(reply);
    }
    return;
  }

  String u = jsonField(reply, "username");
  String pw = jsonField(reply, "password");
  if (!u.length() || !pw.length()) {
    Serial.println(F("Enrol: the reply had no credential in it"));
    return;
  }
  storedUsername = u;
  storedPassword = pw;
  // Written with no echo, ever. Everything else here echoes a change back to the broker so the UX
  // can see it; a credential echoed to a retained topic would be published to the broker and would
  // outlive the mistake, and clearing it needs frugal-iot-clearretained.
  writeConfigToFS("username", u);
  writeConfigToFS("password", pw);
  Serial.print(F("Enrolled as ")); Serial.println(u);
}

// Setup MQTT, connect and subscribe - note if WiFi is connected, this will block till MQTT times out 
void System_MQTT::setup_after_wifi() {
  // Before the first connection: without a credential there is nothing to connect with.
  enrolIfNeeded();
  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  client.begin(hostname.c_str(), net);
  client.setCleanSession(true); // on power up should refresh subscriptions
  // client.setClockSource(XXX); // TODO-23 See https://github.com/256dpi/arduino-mqtt will need for power management
  client.onMessage(MqttMessageReceived);  // Called back from client.loop - this is a naked function that just calls into the instance
  connect(); // Note if WiFi is connected, this will block till MQTT times out 
}
void System_MQTT::captiveLines(AsyncResponseStream* response) {
  frugal_iot.captive->addString(response, id, "hostname", hostname, T->MQTThostname, 5, 60);
}

void System_MQTT::loop() {
  if (nextLoopTime <= millis()) {
    // A node with no credential asks again from here, not only at boot: WiFi can be up while the
    // server is down, an administrator may approve it hours later, and nobody can reach it to
    // restart it. enrolIfNeeded returns at once unless it is both needed and due.
    enrolIfNeeded();
    // Automatically reconnect
    if (connect()) { ; // If Wifi is connected, this is blocking till timeout
      if (!client.loop()) {
        #ifdef SYSTEM_MQTT_DEBUG
          // https://github.com/256dpi/lwmqtt/blob/master/include/lwmqtt.h#L15
          // TODO-125 unclear which error this reports  it might client.returnCode()
          Serial.print(F("MQTT client loop failed ")); Serial.println(client.lastError()); // lwmqtt_err
        #endif // SYSTEM_MQTT_DEBUG
      }; // Do this at end of loop so some time before checks if connected
      nextLoopTime = millis() + SYSTEM_MQTT_MS; // Not sleepSafeSecs as this is frequent
    } else {
      nextLoopTime = (millis() + SYSTEM_MQTT_BACKOFF);
    }
  }
}
// ========== HELPERS ======================
bool System_MQTT::connected() {
  return client.connected(); 
}
/* Connect to MQTT broker and - if necessary - resubscribe to all topics */
// Note this is blocking if not connected, and Wifi is connected // TODO-153 flag in callers.
bool System_MQTT::connect() {
  if (!frugal_iot.wifi->connected()) {
    //Serial.print(F("MQTT waiting for WiFi"));
    return false; // State machine should be reconnecting
  }
  if (!client.connected()) {
    // TODO-153 make this non blocking
    /* Not connected */
    Serial.print(F("MQTT: connecting: to ")); Serial.println(hostname);
    // The call to client.connect is blocking 
    // Theoretically "skip=true" should be good, dont close if connected, but leads to error code=6
    if (!client.connect(frugal_iot.nodeid.c_str(), mqttUsername(), mqttPassword())) {
      /* Still not connected */
      // No F() here. F() yields a __FlashStringHelper* on BOTH cores, but only ESP32's Print has
      // a printf() overload taking one - ESP8266 has printf(const char*) and printf_P(PGM_P)
      // only, so F() there is a compile error. printf_P is not portable either (ESP32 has no
      // such method), so a plain literal is the only form that works on both.
      // Serial.print(F(...))/println(F(...)) ARE fine everywhere - both cores have those.
      Serial.printf("MQTT: Connect Fail %d %d\n", client.lastError(), client.returnCode());
      /* https://github.com/256dpi/lwmqtt/blob/master/include/lwmqtt.h */
      // -3 is LWMQTT_NETWORK_FAILED_CONNECT -10 is userid/password fail
      // 6 is LWMQTT_UNKNOWN_RETURN_CODE 
      // https://github.com/256dpi/lwmqtt/blob/master/include/lwmqtt.h#L116
      // Return code 5 is "not authorized" in MQTT 3.1.1, and lastError -10 is the library's own
      // way of saying the same thing. Only those count: a network failure is not the broker
      // telling us our credential is wrong, and discarding a good credential because the WiFi
      // dropped would be worse than the problem being solved.
      if ((client.returnCode() == 5) || (client.lastError() == -10)) {
        noteAuthFailure();
      }
      return false;
    } else { 
      /* Fresh connection */
      Serial.println(F("MQTT: Connected "));
      authFailures = 0;   // the credential works; earlier refusals were not about it
      if (!client.sessionPresent()) {
        subscriptionsDone = false; // No session so will need to redo subscriptions 
      } else {
        Serial.println(F("MQTT: Session present "));
      }
      frugal_iot.setup_after_mqtt(); // Main thing is to set LoRaMesher gateway
    }
  }
  /* Have a connection - new or old */
  if (!subscriptionsDone) { // Client has reported non-existence of a session
    /* State connected but broker doesnt know subscriptions */
    if (frugal_iot.messages->reSubscribeAll()) {  //TODO-189 check redoing LoRa subscriptions
      subscriptionsDone = true;
    } else {
      return false; // Something failed - probably connection dropped.
    }
  }
  /* Connected and Subscriptions done */
  client.setCleanSession(false);  // Next time use the session created
  return true;
}
/*
 * The broker has refused our credential. If it keeps doing so, throw it away and enrol again.
 *
 * This is what makes "reset this node" work from the dashboard for a node that is alive and well:
 * the server forgets it and deletes its broker account, the node's next few connections are
 * refused, and it then asks for a new credential. Without this a reset would strand exactly the
 * nodes that were working - they hold a credential, so they would never enrol - and someone would
 * have to go and erase the flash by hand.
 *
 * Only reached on an authorisation refusal, never on a network failure.
 */
void System_MQTT::noteAuthFailure() {
  if (!enrolmentSecret) return;            // nothing to enrol with; the credential is all we have
  if (++authFailures < SYSTEM_MQTT_ENROL_RETRY_AFTER) return;
  // No "already has nothing stored, so give up" here, deliberately. That guard stranded exactly
  // the node this is for: one that has just discarded a refused credential holds nothing, and
  // would then never ask to enrol again until it was rebooted.
  if (!storedUsername.length()) {
    authFailures = 0;
    enrolIfNeeded();                       // nothing to discard; just ask again when due
    return;
  }
  Serial.print(F("MQTT: credential refused ")); Serial.print(authFailures);
  Serial.println(F(" times - discarding it and enrolling again"));
  storedUsername = String();
  storedPassword = String();
  frugal_iot.fs_LittleFS->spurt("/mqtt/username", String());
  frugal_iot.fs_LittleFS->spurt("/mqtt/password", String());
  authFailures = 0;
  nextEnrolTime = 0;                       // ask now rather than waiting for the next window
  enrolIfNeeded();
}

// This is for MQTT messages addressed at the mqtt module e.g. dev/org/node/set/mqtt/hostname
void System_MQTT::dispatch(System_Message &msg) {
  // TODO-206 no need to resend, but *do* need to test changing via SPIFFS
  if (msg.isSet() && (msg.module() == id)) {
    if (msg.leaf() == "hostname") {
      hostname = msg.payload;
      writeConfigToFS(msg.leaf(), msg.payload);
      // Could echo here but dont need to
    } else if (msg.leaf() == "username" || msg.leaf() == "password") {
      // This node's own broker credential, normally arriving from LittleFS at startup via
      // readConfigFromFS. Deliberately NOT echoed and not written back: an echo would publish the
      // credential to the broker as a retained message, where it would outlive the mistake.
      //
      // It is also reachable over MQTT, mDNS or LoRa, like any other "set" - so somebody who can
      // already publish to this node's set/ topics could point it at a credential of their choosing.
      // That is no worse than what they could already do (they can drive its actuators directly),
      // and the same is true of set/mqtt/hostname above.
      if (msg.leaf() == "username") { storedUsername = msg.payload; } else { storedPassword = msg.payload; }
      msg.maybeWriteToFS();   // writes unless it came FROM the filesystem; never echoes
    } else {
      System_Base::dispatch(msg);
    }
  }
}
void System_MQTT::prepare() {
  //Note this is intentionally not called
  frugal_iot.mqtt->client.disconnect();
}
void System_MQTT::recover() {
  connect(); // TODO-23 Note this is blocking if WiFi is connected, which it typically won't be. 
}

// UPSTREAM module -> queue -> (loRaMesher -> queue ) -> MQTT -> Broker

bool System_MQTT::subscribe(const String& topicPath) {
  if (connected()) {
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.print(F("MQTT Subscribe ")); Serial.print(topicPath);
      #endif
    // Uses QOS=1 as usually incoming volume is low (except for controls), lets see if causes problem, symptom probably memory use at server or long list of incoming after sleep
    if (client.subscribe(topicPath, 1)) { 
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.println();
      #endif
      return true; 
    } else {
      // Note at this point we don't have a fallback if happen to subscribe when MQTT fails
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.print(F(" Failed"));
        // https://github.com/256dpi/lwmqtt/blob/master/include/lwmqtt.h#L15
        Serial.println(client.lastError());
      #endif // SYSTEM_MQTT_DEBUG
      // Drop thru to return false
    }
  }
  return false;
}
// If retain is set, then the broker will keep a copy 
// TODO implement qos on broker in this library
// qos: 0 = send at most once; 1 = send at least once; 2 = send exactly once
// These are intentionally required parameters rather than defaulting so the coder thinks about the desired behavior
bool System_MQTT::send(const String &topicPath, const String &payload, const bool retain, const int qos) {
  if (connected()) {
    #ifdef SYSTEM_MQTT_DEBUG
      Serial.print(topicPath); Serial.print(F("=")); Serial.print(payload); 
      // Serial.print(F(" qos=")); Serial.print(qos);
    #endif
    if (client.publish(topicPath, payload, retain, qos)) {
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.println();
      #endif
      return true;
    } else {
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.print(F("Failed to publish: ")); 
      #endif
      // https://github.com/256dpi/lwmqtt/blob/master/include/lwmqtt.h#L15
        
      switch (client.lastError()) {
        case -1:
          #ifdef SYSTEM_MQTT_DEBUG
            Serial.print(F(" MQTT Buffer too small, message length~")); Serial.println(topicPath.length() + payload.length());
          #endif
          return true; // It failed, but won't succeed later so delete
          //break;
        case -9:
          #ifdef SYSTEM_MQTT_DEBUG
            Serial.println(F(" Missing or Wrong packet"));
          #endif
          break;
        default: 
          #ifdef SYSTEM_MQTT_DEBUG
            Serial.print(F(" err=")); Serial.println(client.lastError());
          #endif
          ;
        }
      return false;
    };
  } else { 
    return false; // Not connected to MQTT so leave on queue
  }
}

// ========== DOWNSTREAM: Broker -> Mqtt -> (LoRaMesher) -> Modules


void System_MQTT::messageReceived(const String &topicPath, const String &payload) { // cant be constant as dispatch isnt
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.print(F("MQTT incoming: ")); Serial.print(topicPath); Serial.print(F("=")); Serial.println(payload);
  #endif
  inReceived = true;
  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
  frugal_iot.messages->queueIncoming(topicPath, payload, MsgFromMQTT);
  inReceived = false;
}




