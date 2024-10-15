/* MQTT client
* Based on the example in https://github.com/256dpi/arduino-mqtt
* 
* Configuration
* Required: SYSTEM_MQTT_SSID SYSTEM_MQTT_PASSWORD SYSTEM_MQTT_SERVER SYSTEM_MQTT_MS
* Optional: ESP8266 SYSTEM_MQTT_DEBUG SYSTEM_MQTT_DEMO
* 
* TODO - split out the WiFi to system_wifi and configure that
*/

#include "_settings.h"

#ifdef SYSTEM_MQTT_WANT

#if ESP8266 // Note ESP8266 and ESP32 are defined for respective chips - unclear if anything like that for other Arduinos
#include <ESP8266WiFi.h>  // for WiFiClient
#else
#include <WiFi.h> // This will be platform dependent, will work on ESP32 but most likely want configurration for other chips/boards
#endif

#include <MQTT.h>
#include "system_wifi.h"   // xWifi

namespace xMqtt {

WiFiClient net;
MQTTClient client;

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
        client.subscribe(topic);
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
      Subscription *sub;
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.print("Resubscribing: "); 
      #endif
      for (sub = subscriptions; sub; sub = sub->next) {
        client.subscribe(*(sub->topic));
        #ifdef SYSTEM_MQTT_DEBUG
          Serial.print(" " + *(sub->topic));
        #endif // SYSTEM_MQTT_DEBUG
      }
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.println();
      #endif;
    }
};
Subscription *Subscription::subscriptions = NULL;

// A data structure that retains the most recent payload for any topic that has been sent at least once.  
// It is intended to retain this value on both outgoing and incoming messages so that it can operate in a disconnected network. 
class Retention {
  public:
    static Retention *retained;
    String *topic;
    String *message;
    Retention *next;
    Retention(String &t, String &m, Retention* n) {
      topic = &t;
      message = &m;
      next = n;
    } 
    static Retention *find(String &t) {
      Retention *i; 
      for (i = retained; i && (*i->topic != t); i = i->next) {
      }
      return i; // Found or not found case both return here
    }

    static void retain(String &t, String &m) {
      Retention *r = find(t);
      if (r) {
        r->message = &m;
      } else {
        retained = new Retention(t, m, retained);
      }
    }
};
Retention *Retention::retained = NULL;

bool connect() {
  if (WiFi.status() != WL_CONNECTED) {
    #ifdef SYSTEM_MQTT_DEBUG
      Serial.println("MQTT forcing WiFi reconnect");
    #endif
    xWifi::connect(); // TODO-22 - blocking and potential puts portal up, may prefer some kind of reconnect
  }
  if (client.connected()) {
    return true;
  } else {
    #ifdef SYSTEM_MQTT_DEBUG
      Serial.print("\nMQTT connecting: to ");
      Serial.print(xWifi::mqtt_host.c_str());
    #endif 
    if (client.connect("arduino", "public", "public")) { // TODO-21 parameterize this
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

// Note this is called both as a callback from client.onMessage and from messageSend if SYSTEM_MQTT_LOOPBACK
void messageReceived(String &topic, String &payload) {
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.println("MQTT incoming: " + topic + " - " + payload);
  #endif
  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.

  Subscription::dispatch(topic, payload);
}

void subscribe(String &topic, MQTTClientCallbackSimple cb) {
  Subscription::subscribe(topic, cb);
  // If we have retained a previous message for this topic then send to client
  if (Retention *r = Retention::find(topic)) {
    messageReceived(*r->topic, *r->message);
  }
}




// If retain is set, then the broker will keep a copy 
// TODO implement qos on broker in this library
// qos: 0 = send at most once; 1 = send at least once; 2 = send exactly once
// These are intentionally required parameters rather than defaulting so the coder thinks about the desired behavior
void messageSend(String &topic, String &payload, bool retain, int qos) {
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.println("MQTT sending:" + topic + " " + payload);
  #endif
  client.publish(topic, payload, retain, qos);
  // This does a local loopback, if anything is listening for this message it will get it twice - once locally and once via server.
  #ifdef SYSTEM_MQTT_LOOPBACK
    if (retain) {
     Retention::retain(topic, payload); // Keep a copy of outgoing, so local subscribers will see 
    }
    messageReceived(topic, payload);
  #endif
}
void messageSend(String &topic, float &value, int width, bool retain, int qos) {
  String *foo = new String(value, width);
  messageSend(topic, *foo, retain, qos);

}
void messageSend(String &topic, int value, bool retain, int qos) {
  String *foo = new String(value);
  messageSend(topic, *foo, retain, qos);
}

void setup() {
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
}

void loop() {
  if (nextLoopTime <= millis()) {
    // Automatically reconnect
    if (!client.connected()) {
      if (!connect()) { // Non blocking but skip client.loop. Note if fails to connect will set nextLoopTime in 1000 ms.
        nextLoopTime = millis() + 1000; // If non-blocking then dont do any MQTT for a second then try connect again
      }
    } else {
      client.loop(); // Do this at end of loop so some time before checks if connected
      nextLoopTime = millis() + SYSTEM_MQTT_MS;
    }
  }
}
} // Namespace xMqtt
#endif //SYSTEM_MQTT_WANT
