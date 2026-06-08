/*
 * Frugal IoT example - LCD display showing remote SHT sensor readings
 *
 * This device subscribes to temperature and humidity published by a separate
 * SHT sensor node over MQTT, formats the values, and writes them to a local
 * HD44780 LCD connected via I2C backpack.
 *
 * Set REMOTE_SHT_PATH to the full MQTT topic prefix of your SHT sensor node,
 * e.g. "dev/developers/esp32-ab12ef/"  (include trailing slash).
 * The node ID printed on the SHT device's serial output at startup.
 */

#include "Frugal-IoT.h"
#include "control_lcd_sht.h"

#ifndef REMOTE_SHT_PATH
  #define REMOTE_SHT_PATH "dev/lotus/esp8266-fb94bb/" // Replace with actual remote node prefix
#endif

System_Frugal frugal_iot("dev", "developers", "lcd", "LCD SHT Display");

void setup() {
  frugal_iot.configure_power(Power_Loop, 10000, 10000);
  frugal_iot.pre_setup();
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");

  frugal_iot.actuators->add(new Actuator_LCD());

  Control_LCD_SHT* cls = new Control_LCD_SHT();
  frugal_iot.controls->add(cls);
  // Subscribe to the remote SHT sensor's MQTT readings
  cls->temperature->wireTo(String(REMOTE_SHT_PATH) + "sht/temperature");
  cls->humidity->wireTo(String(REMOTE_SHT_PATH) + "sht/humidity");
  // Wire the formatted string output to the local LCD actuator's message input
  cls->message->wireTo(frugal_iot.messages->setPath("lcd/message"));

  frugal_iot.setup();
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop();
}
