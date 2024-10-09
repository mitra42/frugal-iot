/* MQTT client
* Based on the example in https://github.com/256dpi/arduino-mqtt
* 
* Configuration
* Required: SYSTEM_MQTT_SSID SYSTEM_MQTT_PASSWORD SYSTEM_MQTT_SERVER SYSTEM_MQTT_MS
* Optional: CHIP SYSTEM_MQTT_DEBUG SYSTEM_MQTT_DEMO
* 
* TODO - split out the WiFi to system_wifi and configure that
*/

#include "_settings.h"

#ifdef SYSTEM_MQTT_WANT

#if CHIP == ESP8266
#include <ESP8266WiFi.h> 
#else
#include <WiFi.h> // This will be platform dependent, will most likely want configurration for other chips/boards
#endif

#include <MQTT.h>

namespace xMqtt {

const char ssid[] = SYSTEM_MQTT_SSID;
const char pass[] = SYSTEM_MQTT_PASSWORD;

WiFiClient net;
MQTTClient client;

unsigned long nextLoopTime = 0;

#ifdef SYSTEM_MQTT_DEMO
// Hint, For testing, you can subscribe to some other string instead of "/hello" e.g. from a sensor, and display on receipt
String* sTopic = new String("/hello"); 
const char demopayload[] = "world";
unsigned long nextDemoLoopTime = 0;
#endif // SYSTEM_MQTT_DEMO

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
            Serial.println("Dispatching:"+topic);
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

void connect() {
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.print("WiFi connecting:");
  #endif // SYSTEM_MQTT_DEBUG
  while (WiFi.status() != WL_CONNECTED) {
    #ifdef SYSTEM_MQTT_DEBUG
      Serial.print(".");
      Serial.print(WiFi.status());
    #endif // SYSTEM_MQTT_DEBUG
    delay(1000);
  }

  #ifdef SYSTEM_MQTT_DEBUG
    Serial.print("\nMQTT connecting:");
  #endif 
  while (!client.connect("arduino", "public", "public")) {
    #ifdef SYSTEM_MQTT_DEBUG
      Serial.print(".");
    #endif
    delay(1000);
  }
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.println("\nMQTT connected!");
  #endif
  Subscription::resubscribeAll();
}

#ifdef SYSTEM_MQTT_DEMO
  void demoHandler(String &topic, String &payload) {
    Serial.println("handling: " + topic + " - " + payload);
  }
#endif // SYSTEM_MQTT_DEMO

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
  WiFi.begin(ssid, pass);

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  client.begin(SYSTEM_MQTT_SERVER, net);
  client.onMessage(messageReceived);  // Called back from client.loop

  connect();
  #ifdef SYSTEM_MQTT_DEMO
    Subscription::subscribe(*sTopic, &demoHandler);
  #endif
}

void loop() {
  if (nextLoopTime <= millis()) {
    // Automatically reconnect
    if (!client.connected()) {
      connect();
    }
    client.loop(); // Do this at end of loop so some time before checks if connected
    nextLoopTime = millis() + SYSTEM_MQTT_MS;
  }
  #ifdef SENSOR_MQTT_DEMO
    if (nextDemoLoopTime <= millis()) {
      messageSend(sTopic, &demopayload, false, 0);
      nextDemoLoopTime = millis + 1000;
    }
  #endif // SENSOR_MQTT_DEMO
}
} // Namespace xMqtt
#endif //SYSTEM_MQTT_WANT
