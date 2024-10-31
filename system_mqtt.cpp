/* MQTT client
* Based on the example in https://github.com/256dpi/arduino-mqtt
* 
* Configuration
* Required: SYSTEM_MQTT_MS SYSTEM_DISCOVERY_ORGANIZATION SYSTEM_MQTT_PASSWORD
* Optional: ESP8266 SYSTEM_MQTT_DEBUG SYSTEM_WIFI_WANT SYSTEM_MQTT_LOOPBACK
* 
*/

#include "_settings.h"

#ifdef SYSTEM_MQTT_WANT
#if (!defined(SYSTEM_MQTT_USER) || !defined(SYSTEM_MQTT_PASSWORD) || !defined(SYSTEM_DISCOVERY_ORGANIZATION) || !defined(SYSTEM_MQTT_MS))
  error system_discover does not have all requirements in _configuration.h: SYSTEM_DISCOVERY_MS SYSTEM_DISCOVERY_ORGANIZATION
#endif

#if ESP8266 // Note ESP8266 and ESP32 are defined for respective chips - unclear if anything like that for other Arduinos
#include <ESP8266WiFi.h>  // for WiFiClient
#else
#include <WiFi.h> // This will be platform dependent, will work on ESP32 but most likely want configurration for other chips/boards
#endif

#include <MQTT.h>

// If configred not to use Wifi (or in future BLE) then will just operate locally, sending MQTT between components on this node, but 
// not elsewhere.
// TODO add support for BLE if it makes sense for MQTT
#ifdef SYSTEM_WIFI_WANT
#include "system_wifi.h"   // xWifi
#endif  //SYSTEM_WIFI_WANT

namespace xMqtt {

#ifdef SYSTEM_WIFI_WANT
  WiFiClient net;
  MQTTClient client; //was using (512,128) as discovery message was bouncing back, but no longer subscribing to "device" topic.
#endif // SYSTEM_WIFI_WANT


unsigned long nextLoopTime = 0;


// The subscription class manages a list of subscription topics & callbacks. 
// It is not totally self contained as it knows how to call the client to subscribe and how to dispatch. 
class Subscription {
  public: 
    static Subscription *subscriptions;
    String *topic; 
    MQTTClientCallbackSimple cb;
    Subscription *next;
    //Subscription() { topic = NULL; cb = NULL; next = NULL}
    Subscription(String &t, MQTTClientCallbackSimple c, Subscription* n) {
      topic = &t;
      cb = c;
      next = n;
    };
    static Subscription *find(String &t) {
      Subscription *i; 
      for (i = subscriptions; i && (*i->topic != t); i = i->next) {
      }
      return i; // Found or not found case both return here
    }
    static void subscribe(String &topic, MQTTClientCallbackSimple cb) {
      Subscription *existingSub = find(topic);
      subscriptions = new Subscription(topic, cb, subscriptions);
      if (!existingSub) { 
        #ifdef SYSTEM_WIFI_WANT
          if (!client.subscribe(topic)) {
            #ifdef SYSTEM_MQTT_DEBUG
              Serial.println("MQTT Subscription failed to ");
              Serial.print(topic);
            #endif // SYSTEM_MQTT_DEBUG
          };
        #endif // SYSTEM_WIFI_WANT
      }
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.println("Subscribing to: " + topic);
      #endif // SYSTEM_MQTT_DEBUG
    }
    static void dispatch(String &topic, String &payload) {
      Subscription *sub;
      for (sub = subscriptions; sub; sub = sub->next) {
        if (*sub->topic == topic) {
          #ifdef SYSTEM_MQTT_DEBUG
            Serial.println("Dispatching: "+topic);
          #endif // SYSTEM_MQTT_DEBUG
          sub->cb(topic, payload);
        }
        // debugging if needed to figure out why the comparisom above was mismatching
        // else { Serial.println("No match "+*(sub->topic)+" "+topic); }
      }
    }
    static void resubscribeAll() {
      #ifdef SYSTEM_WIFI_WANT
        Subscription *sub;
        #ifdef SYSTEM_MQTT_DEBUG
          Serial.print("Resubscribing: "); 
        #endif // SYSTEM_MQTT_DEBUG
        for (sub = subscriptions; sub; sub = sub->next) {
          if (!client.subscribe(*(sub->topic))) {
            #ifdef SYSTEM_MQTT_DEBUG
              Serial.print("MQTT resubscription failed to ");
            #endif // SYSTEM_MQTT_DEBUG
          }
          #ifdef SYSTEM_MQTT_DEBUG
            Serial.print(" " + *(sub->topic));
          #endif // SYSTEM_MQTT_DEBUG
        }
        #ifdef SYSTEM_MQTT_DEBUG
          Serial.println();
        #endif; // SYSTEM_MQTT_DEBUG
      #endif //SYSTEM_WIFI_WANT
    }
};
Subscription *Subscription::subscriptions = NULL;

// A data structure that represents a single MQTT message
class Message {
  public:
    String *topic;
    String *message;
    bool retain;
    int qos;
    Message *next; // Allows a chain of them in a queue
    Message(String &t, String &m, bool r, int q) {
      topic = &t;
      message = &m;
      retain = r;
      qos = q;
      next = NULL;
    } 
};
class MessageList {
  public:
    MessageList() {
      top = NULL;
    }
    Message *find(String &t) {
      Message *i; 
      for (i = top; i && (*i->topic != t); i = i->next) {
      }
      return i; // Found or not found case both return here
    }
    void push(Message *m) {
      m->next = top;
      top = m;
    }
    Message *shift() {
      Message *i = top;
      Message *j;
      if (!i) return NULL;
      for (;i->next;(j=i, i = i->next)) {}
      j->next = NULL;
      return i;
    }
    void retain(Message *m) {
      Message *f = find(*m->topic);
      if (f) {
        f->message = m->message; 
      } else {
        push(m);
      }
    }
  private:
    Message *top;
};

// A data structure that retains the most recent payload for any topic that has been sent at least once.  
// It is intended to retain this value on both outgoing and incoming messages so that it can operate in a disconnected network. 
MessageList retained;
// A list of messages waiting to be sent.
MessageList queued;

#ifdef SYSTEM_WIFI_WANT // Until we have BLE, compiling without WIFI means just work locally. 
bool connect() {
  xWifi::checkConnected();  // TODO-22 - blocking and potential puts portal up, may prefer some kind of reconnect
  if (client.connected()) {
    return true;
  } else {
    #ifdef SYSTEM_MQTT_DEBUG
      Serial.print("\nMQTT connecting: to ");
      Serial.print(xWifi::mqtt_host.c_str());
    #endif 
   
    // Each organization needs a password in mosquitto_passwords which can be added by Mitra using mosquitto_passwd
    if (client.connect(xWifi::clientid().c_str(), SYSTEM_MQTT_USER, SYSTEM_MQTT_PASSWORD)) { // TODO-29 password should be in local non-git config file
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.println("Connected");
      #endif
      Subscription::resubscribeAll();
      return true;
    } else {
      return false;
    }
  }
}
#endif // SYSTEM_WIFI_WANT

// Inside the receiver its not allowed to send messages, at least with qos != 0; 
bool inReceived = false; 

// Note this is called both as a callback from client.onMessage and from messageSend if SYSTEM_MQTT_LOOPBACK
void messageReceived(String &topic, String &payload) {
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.println("MQTT incoming: " + topic + " - " + payload);
  #endif
  inReceived = true;
  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.

  Subscription::dispatch(topic, payload);
  inReceived = false;
}
void messageReceived(Message *m) {
  messageReceived(*m->topic, *m->message);
}
void subscribe(String &topic, MQTTClientCallbackSimple cb) {
  Subscription::subscribe(topic, cb);
  // If we have retained a previous message for this topic then send to client
  if (Message *r = retained.find(topic)) {
    messageReceived(r);
  }
}

// If retain is set, then the broker will keep a copy 
// TODO implement qos on broker in this library
// qos: 0 = send at most once; 1 = send at least once; 2 = send exactly once
// These are intentionally required parameters rather than defaulting so the coder thinks about the desired behavior
void messageSendInner(Message *m) {
  #ifdef SYSTEM_WIFI_WANT
    if (!client.publish(*m->topic, *m->message, m->retain, m->qos)) {
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.print("Failed to publish");
        Serial.print(*m->topic);
        Serial.print('=');
        Serial.println(*m->message);
      #endif // SYSTEM_MQTT_DEBUG
    };
  #endif // SYSTEM_WIFI_WANT
}
void messageSend(String &topic, String &payload, bool retain, int qos) {
  // TODO-21-sema also queue if WiFi is down and qos>0 - not worth doing till xWifi::connect is non-blocking
  Message *m = new Message(topic, payload, retain, qos);
  if (inReceived && qos) {
    queued.push(m);
  } else {
    messageSendInner(m);
  }
  // Whether send to net or queue, send loopback and do the retention stuff. 
  #ifdef SYSTEM_MQTT_LOOPBACK
    // This does a local loopback, if anything is listening for this message it will get it twice - once locally and once via server.
    if (m->retain) {
      retained.retain(m); // Keep a copy of outgoing, so local subscribers will see 
    }
    messageReceived(m);
  #endif // SYSTEM_MQTT_LOOPBACK
}
void messageSend(String &topic, float &value, int width, bool retain, int qos) {
  String *foo = new String(value, width);
  messageSend(topic, *foo, retain, qos);

}
void messageSend(String &topic, int value, bool retain, int qos) {
  String *foo = new String(value);
  messageSend(topic, *foo, retain, qos);
}

void messageSendQueued() {
  Message *m;
  for (;!inReceived && (m = queued.shift()); messageSendInner(m)) {}
}

void setup() {
  #ifdef SYSTEM_WIFI_WANT // Until have BLE, no WIFI means local only
    // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
    // by Arduino. You need to set the IP address directly.
    client.begin(xWifi::mqtt_host.c_str(), net);
    client.onMessage(messageReceived);  // Called back from client.loop

    // Note WiFi should be connected by this point but will check here anyway
    while (!connect()) {
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.print(".");
      #endif
      delay(1000); // Block waiting for WiFi and MQTT to connect 
    }
  #endif // SYSTEM_WIFI_WANT
}

void loop() {
  #ifdef SYSTEM_WIFI_WANT // Until have BLE, no WIFI means local only
    if (nextLoopTime <= millis()) {
      // Automatically reconnect
      if (!client.connected()) {
        if (!connect()) { // Non blocking but skip client.loop. Note if fails to connect will set nextLoopTime in 1000 ms.
          nextLoopTime = millis() + 1000; // If non-blocking then dont do any MQTT for a second then try connect again
        }
      } else {
        messageSendQueued();
        if (!client.loop()) {
          #ifdef SYSTEM_MQTT_DEBUG
            Serial.print("MQTT client loop failed ");
            lwmqtt_err_t err = client.lastError();
            Serial.println(err);
          #endif // SYSTEM_MQTT_DEBUG
        }; // Do this at end of loop so some time before checks if connected
        nextLoopTime = millis() + SYSTEM_MQTT_MS;
      }
    }
  #endif // SYSTEM_WIFI_WANT
}
} // Namespace xMqtt
#endif //SYSTEM_MQTT_WANT
