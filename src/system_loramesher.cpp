/* Frugal IoT - LoRaMesher wrapper 
 *
 * Based on examples at https://github.com/LoRaMesher/LoRaMesher
 * 
 * Issues: https://github.com/mitra42/frugal-iot/issues/152 (and also 137 (LoRa) a bit)
 * 
 * Notes:
 *    This uses the "radio" library for LoRa rather than Sandeep Mishra's 
 * 
 * Explanations: (Based on limited understanding - i.e. could be wrong)
 *  LoRaMeshMessage = appPrtDst appPrtSrc messageId  // as used by LoRaChat
 *  FrugalIoTMessage = message // Will evolve to what needed for MQTT
 *  AppPacket<xxx> = src dst payload=xxx
*/

#include "_settings.h"
#ifdef SYSTEM_LORAMESHER_WANT

#error "SYSTEM_LORAMESHER code is known not to be finished - see issue # 152 - uncomment to develop"

#include "LoraMesher.h"
#include "system_loramesher.h"
#if defined(SYSTEM_LORAMESHER_SENDER_TEST) || defined(SYSTEM_LORAMESHER_RECEIVER_TEST)
  #include "system_oled.h"
#endif
#include "misc.h"  // for lprintf

// These settings duplicated in system_loramesher.cpp and system_lora.cpp (system_loramesher.cpp are newer)
#if defined(TTGO_LORA_SX1276)
  #define SYSTEM_LORAMESHER_MODULE LoraMesher::LoraModules::SX1276_MOD // For N.America and Australia 
#elif defined(TTGO_LORA_SX1278)
  #define SYSTEM_LORAMESHER_MODULE LoraMesher::LoraModules::SX1278_MOD // For N.America and Australia 
#endif

// For TTGO_LORA_SX127X 
// LORA_SCK; LORA_MISO; LORA_MOSI; LORA_CS; LORA_RST; and for V21 LORA_D1 are defined correctly in
// e.g. ~/Arduino15/packages/esp32/hardware/esp32/3.2.0/variants/ttgo-lora32-v21new/pins_arduino.h etc
#if defined(TTGO_LORA_SX127X_V1)
  #define LORA_D0 LORA_IRQ
#elif defined(TTGO_LORA_SX127X_V2) || defined(TTGO_LORA_SX127X_V21) // V3 is almost same as V2
  #define LORA_D0 LORA_IRQ
  // Note on V2 but not V21 LORA_D1 is not defined and may need a physical wire to GPIO 33
#else
  #error "Unsupported LORA configuration. Please define either TTGO_LORA_SX127X_V1 or TTGO_LORA_SX127X_V2. or define new BOARD"
#endif

System_LoraMesher* loramesher;

System_LoraMesher::System_LoraMesher()
: Frugal_Base("loramesher", "LoraMesher"),
    radio(LoraMesher::getInstance()),
    config(LoraMesher::LoraMesherConfig())
{
    config.loraCs = LORA_CS;
    config.loraRst = LORA_RST;
    config.loraIrq = LORA_IRQ;
    config.loraIo1 = LORA_D1;  // Requirement for D1 may mean it won't work on TTGO_LORA_SX127X_V1 or _V2 but will on _V21
    config.module = SYSTEM_LORAMESHER_MODULE;
    config.freq = SYSTEM_LORAMESHER_BAND;
}

#if defined(SYSTEM_LORAMESHER_TEST_COUNTER)
  uint32_t dataCounter = 0;
  struct counterPacket {
      uint32_t counter = 0;
  };
  counterPacket* helloPacket = new counterPacket;
#elif defined(SYSTEM_LORAMESHER_TEST_STRING)
  uint32_t dataCounter = 0;
#else 
  #error Must define SYSTEM_LORAMESHER_TEST_COUNTER or SYSTEM_LORAMESHER_TEST_STRING
#endif


#ifdef SYSTEM_LORAMESHER_RECEIVER_TEST
  #ifdef SYSTEM_LORAMESHER_TEST_COUNTER
    void printCounterPacket(counterPacket data) {
      Serial.printf("Hello Counter received nº %d\n", data.counter);
      // Display information
      oled->display.clearDisplay();
      oled->display.setCursor(0,0);
      oled->display.print("LORA MESH RECEIVER");
      oled->display.setCursor(0,20);
      oled->display.print("Received packet:");
      oled->display.setCursor(0,30);
      oled->display.print(data.counter);
      //oled->display.setCursor(0,40);
      //oled->display.print("RSSI:");
      //oled->display.setCursor(30,40);
      //oled->display.print(rssi);
      oled->display.display();   
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
        oled->display.clearDisplay();
        oled->display.setCursor(0,0);
        oled->display.print("LORA MESH RECEIVER");
        oled->display.setCursor(0,20);
        oled->display.print("Received packet:");
        oled->display.setCursor(0,30);
        oled->display.print((char*)dPacket);
        //oled->display.setCursor(0,40);
        //oled->display.print("RSSI:");
        //oled->display.setCursor(30,40);
        //oled->display.print(rssi);
        oled->display.display();   
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


#ifdef SYSTEM_LORAMESHER_RECEIVER_TEST
// Function that process the received packets - it receives an AppPacket<xxx> and passes to handler
void processReceivedPackets(void*) {
    for (;;) {
        /* Wait for the notification of processReceivedPackets and enter blocking */
        ulTaskNotifyTake(pdPASS, portMAX_DELAY);

        //Iterate through all the packets inside the Received User Packets Queue
        while (loramesher->radio.getReceivedQueueSize() > 0) {
            Serial.println("ReceivedUserData_TaskHandle notify received");
            Serial.printf("Queue receiveUserData size: %d\n", loramesher->radio.getReceivedQueueSize());

            //Get the first element inside the Received User Packets Queue
            #if defined(SYSTEM_LORAMESHER_TEST_COUNTER)
              AppPacket<counterPacket>* packet = loramesher->radio.getNextAppPzacket<counterPacket>();
              //Print the App Packet
              printAppCounterPacket(packet); // This is the actual handling
            #elif defined(SYSTEM_LORAMESHER_TEST_STRING)
              AppPacket<uint8_t>* appPacket = loramesher->radio.getNextAppPacket<uint8_t>();
              printAppData(appPacket);

            #else // Start (but wont work) of FrugalIoT packet
              AppPacket<FrugalIoTMessage>* appPacket = loramesher->radio.getNextAppPacket<FrugalIoTMessage>();
              printAppFrugal(packet);
              // Pull apart FrugalIoTMessage to something want to process
              //DataMessage* dataMessage = createDataMessage(AppPacket<FrugalIoTMessage>* appPacket))
            #endif      
            //Delete the packet when used. It is very important to call this function to release the memory of the packet.
            loramesher->radio.deletePacket(appPacket);
        }
    }
}

// Used in task creation
TaskHandle_t receiveLoRaMessage_Handle = NULL;

// Create a Receive Messages Task and add it to the LoRaMesher
// Equivalent of system_mqtt's: client.onMessage(xMqtt::MessageReceived)
void createReceiveMessages() {
    int res = xTaskCreate(
        processReceivedPackets,
        "Receive App Task",
        4096,
        (void*) 1,
        2,
        &receiveLoRaMessage_Handle);
    if (res != pdPASS) {
        Serial.printf("Error: Receive App Task creation gave error: %d\n", res);
    }
}
#endif // SYSTEM_LORAMESHER_RECEIVER_TEST

bool System_LoraMesher::findGatewayNode() {
    RouteNode* rn = radio.getClosestGateway();
    if (rn) {
      gatewayNodeAddress = rn->networkNode.address;
      return true;
    } else {
      gatewayNodeAddress = BROADCAST_ADDR;
      return false;
    }
}

void System_LoraMesher::setup() {
  Serial.println("Loramesher setup");
  // Error codes are buried deep  .pio/libdeps/temploramesher/RadioLib/src/TypeDef.h
  // -12 is invalid frequency usually means band and module are not matched.
  radio.begin(config);        //Init the loramesher with a configuration
  #ifdef SYSTEM_LORAMESHER_RECEIVER_TEST
    createReceiveMessages();    //Create the receive task and add it to the LoRaMesher
    radio.setReceiveAppDataTaskHandle(receiveLoRaMessage_Handle); //Set the task handle to the LoRaMesher
  #endif
  //gpio_install_isr_service(ESP_INTR_FLAG_IRAM); // known error message - Jaimi says doesn't matter
  radio.start();     //Start LoRaMesher
  #ifdef SYSTEM_LORAMESHER_SENDER_TEST
  if (!findGatewayNode()) {
      Serial.println("Setup did not find a gateway node");
  }
  #endif //SYSTEM_LORAMESHER_SENDER_TEST
  #ifdef SYSTEM_LORAMESHER_RECEIVER_TEST
    radio.addGatewayRole(); 
  #endif
}


void System_LoraMesher::periodically() {
  #ifdef SYSTEM_LORAMESHER_SENDER_TEST
    Serial.print("Send packet ");  Serial.println(dataCounter);
    if (gatewayNodeAddress == BROADCAST_ADDR) {
      if (findGatewayNode()) {
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
    oled->display.clearDisplay();
    oled->display.setCursor(0,0);
    oled->display.println("LORAMESH SENDER");
    oled->display.setCursor(0,20);
    oled->display.setTextSize(1);
    oled->display.print("LoRa packet sent.");
    oled->display.setCursor(0,30);
    #if defined(SYSTEM_LORAMESHER_TEST_COUNTER)
      oled->display.print("Counter:");
      oled->display.setCursor(50,30);
      oled->display.print(dataCounter);      
    #elif defined(SYSTEM_LORAMESHER_TEST_STRING)
      oled->display.print("Counter:");
      oled->display.setCursor(50,30);
      oled->display.print(dataCounter-1);
    #else // Start - wont work - for FrugalIoT
      oled->display.print(dataCounter);      
    #endif
    oled->display.display();
    #if defined(SYSTEM_LORAMESHER_TEST_COUNTER)
      radio.createPacketAndSend(BROADCAST_ADDR, data, 1); // Size is number of counterPackets.
    #elif defined(SYSTEM_LORAMESHER_TEST_STRING)
      radio.sendPacket(gatewayNodeAddress, msg, msglen); // Will be broadcast if no node
    #else // start - wont work - to create FrugalIoT  
      radio.createPacketAndSend(BROADCAST_ADDR, msg, 1); // Size is number of counterPackets.
      free(msg);
    #endif
  #endif
}

#if !defined(SYSTEM_LORAMESHER_SENDER_TEST) && !defined(SYSTEM_LORAMESHER_RECEIVER_TEST)
  System_LoraMesher::publish(const String &topicPath, const String &payload, const bool retain, const int qos) {
    // Build a struc - unclear how to handle strings
    counterPacket* data = new counterPacket;
      TODO figure out how to build packet of Strings
    radio.createPacketAndSend(BROADCAST_ADDR, data, 1); // Size is number of counterPackets. 
  }
#endif

 void System_LoraMesher::prepareForSleep() {
    // TODO-139 TODO-23 find where put radio and SPI to sleep - on some other libraries its LoRa.sleep() and SPI.end()
  }

#endif // SYSTEM_LORAMESHER_WANT
