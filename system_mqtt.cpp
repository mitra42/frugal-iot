/* MQTT client
* Based on the example in https://github.com/256dpi/arduino-mqtt
* 
* Configuration
* Required: SYSTEM_MQTT_SSID SYSTEM_MQTT_PASSWORD SYSTEM_MQTT_SERVER SYSTEM_MQTT_MS
* Optional: CHIP SYSTEM_MQTT_DEBUG SYSTEM_MQTT_DEMO
* 
* TODO - split out the WiFi to system_wifi and configure that
* TODO - split out the message and dispatching so can be used internally for wiring between modules or via MQTT 
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

// Note order if this is important - the constructor needs the variable. 
class Subscription {
  public: 
    String *topic; 
    MQTTClientCallbackSimple cb;
    Subscription *next;
    //Subscription() { topic = NULL; cb = NULL; next = NULL}
    Subscription(String &t, MQTTClientCallbackSimple c, Subscription* n) {
      topic = &t;
      cb = c;
      next = n;
    };
};
Subscription *subscriptions = NULL;

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

  // Resubscribe to any subscriptions after reconnect
  Serial.print("Re-Subscribing to:");
  Subscription* sub = subscriptions; 
  while (sub) {
    client.subscribe(*(sub->topic));
    #ifdef SYSTEM_MQTT_DEBUG
      Serial.print(" " + *(sub->topic));
    #endif // SYSTEM_MQTT_DEBUG
    sub = sub->next;
  }
  #ifdef SYSTEM_MQTT_DEBUG
  Serial.println();
  #endif // SYSTEM_MQTT_DEBUG
  // client.unsubscribe(SENSOR_MQTT_TOPIC);
}

// Call this from any module that wants to listen to a channel
void subscribe(String &topic, MQTTClientCallbackSimple cb) {
  subscriptions = new Subscription(topic, cb, subscriptions);
  client.subscribe(topic);
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.println("Subscribing to: " + topic);
  #endif // SYSTEM_MQTT_DEBUG
}

#ifdef SYSTEM_MQTT_DEMO
  void demoHandler(String &topic, String &payload) {
    Serial.println("handling: " + topic + " - " + payload);
  }
#endif // SYSTEM_MQTT_DEMO

void messageReceived(String &topic, String &payload) {
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.println("MQTT incoming: " + topic + " - " + payload);
  #endif
  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.

  // TODO maybe this should be a separate dispatcher, independent of MQTT so can be used internally for event changes.
  Subscription* sub = subscriptions; 
  while (sub) {
    if (*sub->topic == topic) {
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.println("Dispatching:"+topic);
      #endif // SYSTEM_MQTT_DEBUG
      sub->cb(topic, payload);
    }
    // debugging if needed to figure out why the comparisom above was mismatching
    // else { Serial.println("No match "+*(sub->topic)+" "+topic); }
    sub = sub->next;
  }
}

void messageSend(String &topic, String &payload) {
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.println("MQTT sending:" + topic + " " + payload);
  #endif
  client.publish(topic, payload);
  // This does a local loopback, if anything is listening for this message it will get it twice - once locally and once via server.
  #ifdef SYSTEM_MQTT_LOOPBACK
    messageReceived(topic, payload);
  #endif
}
void messageSend(String &topic, float &value, int width) {
  String *foo = new String(value, width);
  messageSend(topic, *foo);

}
void messageSend(String &topic, int value) {
  String *foo = new String(value);
  messageSend(topic, *foo);
}

void setup() {
  WiFi.begin(ssid, pass);

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  client.begin(SYSTEM_MQTT_SERVER, net);
  client.onMessage(messageReceived);  // Called back from client.loop

  connect();
  #ifdef SYSTEM_MQTT_DEMO
    subscribe(*sTopic, &demoHandler);
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
      messageSend(sTopic, &demopayload);
      nextDemoLoopTime = millis + 1000;
    }
  #endif // SENSOR_MQTT_DEMO
}
} // Namespace xMqtt
#endif //SYSTEM_MQTT_WANT
