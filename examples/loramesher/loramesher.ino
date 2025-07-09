/* 
 *  Frugal IoT example - LoRaMesher demo - a work in progress
 * 
 * Optional: 
 *  SYSTEM_LORAMESHER_SENDER_TEST or SYSTEM_LORAMESHER_RECEIVER_TEST which role it should play
 * 
 */

#include "frugal_iot.h"
#include "system_mqtt.h"

// Change the parameters here to match your ... 
// organization, project, id, description
System_Frugal frugal_iot("dev", "developers", "loramesher", "LoraMesher demo");

void setup() {
  frugal_iot.startSerial(); // Encapsulate setting up and starting serial

  // Override MQTT host, username and password if you have an "organization" other than "dev" (developers)
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");

  // Configure power handling - type, cycle_ms, wake_ms 
  // power will be awake wake_ms then for the rest of cycle_ms be in a mode defined by type 
  // Loop= awake all the time; 
  // Light = Light Sleep; 
  // LightWiFi=Light + WiFi on (not working); 
  // Modem=Modem sleep - works but negligable power saving
  // Deep - works but slow recovery and slow response to UX so do not use except for multi minute cycles. 
  frugal_iot.configure_power(Power_Loop, 30000, 30000); // Take a reading every 30 seconds - awake all the time

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


#ifdef SYSTEM_LORAMESHER_SENDER_TEST

#endif // SYSTEM_LORAMESHER_SENDER_TEST


#ifdef SYSTEM_LORAMESHER_RECEIVER_TEST

    void printAppData(AppPacket<uint8_t>* appPacket) {
        Serial.printf("Packet arrived from %X with size %d\n", appPacket->src, appPacket->payloadSize);
        //Get the payload to iterate through it
        uint8_t* dPacket = appPacket->payload;
        // Note - dont use appPacket->getPayloadLength - it will report number of packets=1 not length of payload
        size_t payloadLength = appPacket->getPayloadLength()-1; // Length of string without terminator 
        Serial.write(dPacket, payloadLength); Serial.println(); // Being conservative in case no terminating \0 
              // Display information
        frugal_iot.oled->display.clearDisplay();
        frugal_iot.oled->display.setCursor(0,0);
        frugal_iot.oled->display.print("LORA MESH RECEIVER");
        frugal_iot.oled->display.setCursor(0,20);
        frugal_iot.oled->display.print("Received packet:");
        frugal_iot.oled->display.setCursor(0,30);
        frugal_iot.oled->display.print((char*)dPacket);
        //frugal_iot.oled->display.setCursor(0,40);
        //frugal_iot.oled->display.print("RSSI:");
        //frugal_iot.oled->display.setCursor(30,40);
        //frugal_iot.oled->display.print(rssi);
        frugal_iot.oled->display.display();   
    }
#endif // SYSTEM_LORAMESHER_RECEIVER_TEST



// You can put custom code in here, 
void loop() {
  if (frugal_iot.timeForPeriodic) {
    // Things which happen once for each sensor read period go here. 
    // This is also a good place to put things that check how long since last running
    #ifdef SYSTEM_LORAMESHER_SENDER_TEST
      // senderPeriodic();
    #endif
  }
  frugal_iot.loop(); // Do not delete this call to frugal_iot.loop
}

