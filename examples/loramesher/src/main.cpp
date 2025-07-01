/* 
 *  Frugal IoT example - LoRaMesher demo - a work in progress
 * 
 * Optional: 
 *  SYSTEM_LORAMESHER_SENDER_TEST or SYSTEM_LORAMESHER_RECEIVER_TEST which role it should play
 *  SYSTEM_LORAMESHER_TEST_STRING specify to run the string test
 * 
 */

#include "frugal_iot.h"

// Change the parameters here to match your ... 
// organization, project, id, description
System_Frugal frugal_iot("dev", "developers", "loramesher", "LoraMesher demo");

void setup() {
  frugal_iot.startSerial(); // Encapsulate setting up and starting serial

  // Override MQTT host, username and password if you have an "organization" other than "dev" (developers)
  frugal_iot.system->add(new System_MQTT("frugaliot.naturalinnovation.org", "dev", "public"));
  // system_oled and actuator_ledbuiltin added automatically on boards that have them.

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));

  //esp_log_level_set(LM_TAG, ESP_LOG_INFO);     // enable INFO logs from LoraMesher - but doesnt seem to work
  frugal_iot.loramesher = new System_LoraMesher(); // Held in a variable as future LoRaMesher will access it directly e.g. from MQTT
  frugal_iot.system->add(frugal_iot.loramesher);

  // system_oled and actuator_ledbuiltin added automatically on boards that have them.
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop();
}

