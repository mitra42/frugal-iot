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
  void senderPeriodic() {
    static uint32_t dataCounter = 0;
    Serial.print("Send packet ");  Serial.println(dataCounter);
    if (frugal_iot.loramesher->gatewayNodeAddress == BROADCAST_ADDR) {
      if (frugal_iot.loramesher->findGatewayNode()) {
        Serial.println("============ XXX Now seeing gateway node ======");
      }
    }
    #if defined(SYSTEM_LORAMESHER_TEST_COUNTER)
      counterPacket* data = new counterPacket;
      data->counter = dataCounter++;
    #elif defined(SYSTEM_LORAMESHER_TEST_STRING)
      const char* stringymessage = lprintf(20, "Test # %d", dataCounter++);
      size_t msglen = strlen(stringymessage)+1; // +1 to include terminating \0
      // Allocate enough memory for the struct + message
      uint8_t* msg = (uint8_t*) malloc(msglen);
      //DataPacket* dPacket = (DataPacket*) malloc(sizeof(DataPacket) + msglen); // sendPacket wants uint8_t*
      // Copy the string into the message array
      memcpy(msg, stringymessage, msglen);
      delete(stringymessage);
      // TODO - maybe could just cast stringmessage as (uint8_t*) instead of copying
    #else // Start - not working - for FrugalIoT message
      const char* stringymessage = "This is a test";
      size_t msglen = strlen(stringymessage)+1;
      // Allocate enough memory for the struct + message
      FrugalIoTMessage* msg = (FrugalIoTMessage*) malloc(sizeof(FrugalIoTMessage) + msglen);
      // Set messageId as needed
      msg->messageId = dataCounter++; // or whatever value you want
      // Copy the string into the message array
      memcpy(msg->message, stringymessage, msglen);
      //Serial.println(stringypacket);
    #endif
    frugal_iot.oled->display.clearDisplay();
    frugal_iot.oled->display.setCursor(0,0);
    frugal_iot.oled->display.setTextSize(1);
    frugal_iot.oled->display.println("LORAMESH SENDER");
    frugal_iot.oled->display.setCursor(0,20);
    frugal_iot.oled->display.print("LoRa packet sent.");
    frugal_iot.oled->display.setCursor(0,30);
    #if defined(SYSTEM_LORAMESHER_TEST_COUNTER)
      frugal_iot.oled->display.print("Counter:");
      frugal_iot.oled->display.setCursor(50,30);
      frugal_iot.oled->display.print(dataCounter);      
    #elif defined(SYSTEM_LORAMESHER_TEST_STRING)
      frugal_iot.oled->display.print("Counter:");
      frugal_iot.oled->display.setCursor(50,30);
      frugal_iot.oled->display.print(dataCounter-1);
    #else // Start - wont work - for FrugalIoT
      frugal_iot.oled->display.print(dataCounter);      
    #endif
    frugal_iot.oled->display.display();
    #if defined(SYSTEM_LORAMESHER_TEST_COUNTER)
      radio.createPacketAndSend(BROADCAST_ADDR, data, 1); // Size is number of counterPackets.
    #elif defined(SYSTEM_LORAMESHER_TEST_STRING)
      frugal_iot.loramesher->radio.sendPacket(frugal_iot.loramesher->gatewayNodeAddress, msg, msglen); // Will be broadcast if no node
    #else // start - wont work - to create FrugalIoT  
      frugal_iot.loramesher->radio.createPacketAndSend(BROADCAST_ADDR, msg, 1); // Size is number of counterPackets.
      free(msg);
    #endif
}
#endif // SYSTEM_LORAMESHER_SENDER_TEST


#ifdef SYSTEM_LORAMESHER_RECEIVER_TEST
  #ifdef SYSTEM_LORAMESHER_TEST_COUNTER
    void printCounterPacket(counterPacket data) {
      Serial.printf("Hello Counter received nº %d\n", data.counter);
      // Display information
      frugal_iot.oled->display.clearDisplay();
      frugal_iot.oled->display.setCursor(0,0);
      frugal_iot.oled->display.print("LORA MESH RECEIVER");
      frugal_iot.oled->display.setCursor(0,20);
      frugal_iot.oled->display.print("Received packet:");
      frugal_iot.oled->display.setCursor(0,30);
      frugal_iot.oled->display.print(data.counter);
      //frugal_iot.oled->display.setCursor(0,40);
      //frugal_iot.oled->display.print("RSSI:");
      //frugal_iot.oled->display.setCursor(30,40);
      //frugal_iot.oled->display.print(rssi);
      frugal_iot.oled->display.display();   
    }
    void printAppCounterPacket(AppPacket<counterPacket>* packet) {
        Serial.printf("Packet arrived from %X with size %d\n", packet->src, packet->payloadSize);
        //Get the payload to iterate through it
        counterPacket* dPacket = packet->payload;
        size_t payloadLength = packet->getPayloadLength();
        for (size_t i = 0; i < payloadLength; i++) {
            //Print the packet
            printCounterPacket(dPacket[i]);
        }
    }
  #elif defined(SYSTEM_LORAMESHER_TEST_STRING)
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

  #else // Start - wont work - of FrugalIoT
    void printAppFrugal(AppPacket<FrugalIoTMessage>* packet) {
      Serial.printf("Packet arrived from %X with size %d\n", packet->src, packet->payloadSize);
      //Get the payload to iterate through it
      FrugalIoTMessage* dPacket = packet->payload;
      size_t payloadLength = packet->getPayloadLength();
      Serial.println(*dPacket->message);
      /*
      for (size_t i = 0; i < payloadLength; i++) {
          //Print the packet
          printCounterPacket(dPacket[i]);
      }
      */
    }
  #endif
#endif // SYSTEM_LORAMESHER_RECEIVER_TEST



// You can put custom code in here, 
void loop() {
  if (frugal_iot.timeForPeriodic) {
    // Things which happen once for each sensor read period go here. 
    // This is also a good place to put things that check how long since last running
    #ifdef SYSTEM_LORAMESHER_SENDER_TEST
      senderPeriodic();
    #endif
  }
  frugal_iot.loop(); // Do not delete this call to frugal_iot.loop
}

