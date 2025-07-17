/*
  Local Dev

  You can add new functions of classes here, without them being pushed to Git.

 */

#include "_settings.h"  // Settings for what to include etc

#include "control.h"
#include "system_oled.h"
#include "system_lora.h"
#include "system_frugal.h"

namespace localDev {

// Put your setup here, 
// 
// This is also good place to instantiate instances of classes as its called from frugal_iot.cpp
// Note if you use a subclass of Actuator, Sensor or Control, then the instance's setup() will be called as part of the System_Base::setupAll() as called by frugal_iot.cpp 
void setup() {
  // And here is an example of instantiating a custom control using that function
  //controls.push_back(new ControlHysterisis("humidity", "Humidity control", 50, 0, 100));

  #if defined(SYSTEM_LORA_SENDER_TEST) || defined(SYSTEM_LORA_RECEIVER_TEST) // TODO-137 (LoRa) and TODO-149 (oled)
    // This is a test for the LoRa sender
    Serial.println(F("LoRa Sender Test Starting"));
  // TODO-137 (LoRa) amd TODO-149 (oled)
    #ifdef SYSTEM_LORA_DEBUG
      Serial.println(F("LoRa Debugging Enabled"));
    #endif // SYSTEM_LORA_DEBUG
    #ifdef SYSTEM_OLED_DEBUG
      Serial.println(F("OLED Debugging Enabled"));
    #endif // SYSTEM_OLED_DEBUG
    #ifdef SYSTEM_LORA_SENDER_TEST
      frugal_iot.oled->display.print("LORA SENDER ");
      Serial.println(F("LoRa Sender Starting"));
    #elif defined(SYSTEM_LORA_RECEIVER_TEST)
      frugal_iot.oled->display.print("LORA RECEIVER ");
      Serial.println(F("OLED Receiver Starting"));
    #endif
    // Should only do this if LoRa setup succeeded
    frugal_iot.oled->display.setCursor(1,0);
    frugal_iot.oled->display.print("LoRa and OLED should be good to go");
    frugal_iot.oled->display.display();
    delay(2000);
  #endif // SYSTEM_LORA_SENDER_TEST || SYSTEM_LORA_RECEIVER_TEST

}


// Put your loop here, but note if you use a subclass of Actuator, Sensor or Control 
// then its loop member function will be called automatically and is not needed here
// In that case, the function still needs to exist - just empty 
void periodically () {
  #if defined(SYSTEM_LORA_SENDER_TEST)
    static int counter = 0; // Counter for LoRa sender test
    Serial.print("Sending packet: ");
    Serial.println(counter);
    
    //Send LoRa packet to receiver
    LoRa.beginPacket();
    LoRa.print("hello ");
    LoRa.print(counter);
    LoRa.endPacket();
    
    frugal_iot.oled->display.clearDisplay();
    frugal_iot.oled->display.setCursor(0,0);
    frugal_iot.oled->display.println("LORA SENDER");
    frugal_iot.oled->display.setCursor(0,20);
    frugal_iot.oled->display.setTextSize(1);
    frugal_iot.oled->display.print("LoRa packet sent.");
    frugal_iot.oled->display.setCursor(0,30);
    frugal_iot.oled->display.print("Counter:");
    frugal_iot.oled->display.setCursor(50,30);
    frugal_iot.oled->display.print(counter);      
    frugal_iot.oled->display.display();
    counter++;      
  #endif // SYSTEM_LORA_SENDER_TEST || SYSTEM_LORA_RECEIVER_TEST
}

void loop() {
  // This is called frequently, so you can put code here that needs to run often
  #if defined(SYSTEM_LORA_RECEIVER_TEST)

    static String LoRaData;

    //try to parse packet
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      //received a packet
      Serial.print("Received packet ");

      //read packet
      while (LoRa.available()) {
        LoRaData = LoRa.readString();
        Serial.print(LoRaData);
      }

      //print RSSI of packet
      int rssi = LoRa.packetRssi();
      Serial.print(" with RSSI ");    
      Serial.println(rssi);

      // Dsiplay information
      frugal_iot.oled->display.clearDisplay();
      frugal_iot.oled->display.setCursor(0,0);
      frugal_iot.oled->display.print("LORA RECEIVER");
      frugal_iot.oled->display.setCursor(0,20);
      frugal_iot.oled->display.print("Received packet:");
      frugal_iot.oled->display.setCursor(0,30);
      frugal_iot.oled->display.print(LoRaData);
      frugal_iot.oled->display.setCursor(0,40);
      frugal_iot.oled->display.print("RSSI:");
      frugal_iot.oled->display.setCursor(30,40);
      frugal_iot.oled->display.print(rssi);
      frugal_iot.oled->display.display();   
    }
  #endif // SYSTEM_LORA_RECEIVER_TEST
}
void infrequently() { }

} // namespace localDev
