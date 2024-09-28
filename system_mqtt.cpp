/* MQTT client
* Based on the example in https://github.com/256dpi/arduino-mqtt
* 
* Configuration
* Required: SYSTEM_MQTT_SSID SYSTEM_MQTT_PASSWORD SYSTEM_MQTT_SERVER SENSOR_MQTT_TOPIC SENSOR_MQTT_PAYLOAD
* Optional: CHIP SYSTEM_MQTT_DEBUG
* 
* TODO - split out the WiFi to system_wifi and configure that
*/

#include "_settings.h"

#ifdef WANT_SYSTEM_MQTT

#include "system_clock.h"

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

#ifdef SENSOR_ANALOG_MS
unsigned long lastLoopTime = 0;
#endif // SENSOR_ANALOG_MS


void connect() {
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.print("WiFi connecting:");
  #endif // SYSTEM_MQTT_DEBUG
  while (WiFi.status() != WL_CONNECTED) {
    #ifdef SYSTEM_MQTT_DEBUG
      Serial.print(".");
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

  client.subscribe(SENSOR_MQTT_TOPIC);
  // client.unsubscribe(SENSOR_MQTT_TOPIC);
}

void messageReceived(String &topic, String &payload) {
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.println("incoming: " + topic + " - " + payload);
  #endif
  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}

void setup() {
  WiFi.begin(ssid, pass);

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  client.begin(SYSTEM_MQTT_SERVER, net);
  client.onMessage(messageReceived);  // Called back from client.loop

  connect();
}

void loop() {
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  // Automatically reconnect
  if (!client.connected()) {
    connect();
  }
#ifdef SENSOR_ANALOG_MS
  if (xClock::hasIntervalPassed(lastLoopTime, SENSOR_MQTT_MS)) {
#endif // SENSOR_ANALOG_MS
    client.publish(SENSOR_MQTT_TOPIC, SENSOR_MQTT_PAYLOAD);
#ifdef SENSOR_ANALOG_MS
        lastLoopTime = xClock::getTime();
    }
#endif // SENSOR_ANALOG_MS
}

} // Namespace xMqtt
#endif //WANT_SYSTEM_MQTT
