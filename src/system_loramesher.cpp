/* Frugal IoT - LoRaMesher wrapper 
 *
 * Based on examples at https://github.com/LoRaMesher/LoRaMesher
 * 
 * Issues: https://github.com/mitra42/frugal-iot/issues/152 (and also 137 (LoRa) a bit)
 * 
 * Notes:
 *    This uses the "radio" library for LoRa rather than Sandeep Mishra's 
 * 
*/

#include "_settings.h"
#ifdef SYSTEM_LORAMESHER_WANT

#include "LoraMesher.h"
#include "system_loramesher.h"
#if defined(SYSTEM_LORAMESHER_SENDER_TEST) || defined(SYSTEM_LORAMESHER_RECEIVER_TEST)
  #include "system_oled.h"
#endif

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
: Frugal_Base(),
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

TaskHandle_t receiveLoRaMessage_Handle = NULL;

#if defined(SYSTEM_LORAMESHER_SENDER_TEST) || defined(SYSTEM_LORAMESHER_RECEIVER_TEST)
  uint32_t dataCounter = 0;
  struct dataPacket {
      uint32_t counter = 0;
  };
  dataPacket* helloPacket = new dataPacket;
#else
  struct dataPacket {
      XXX need { String String bool int }  TODO-HOW TO DO STRINGS
  };

  dataPacket* helloPacket = new dataPacket;
#endif


#ifdef SYSTEM_LORAMESHER_RECEIVER_TEST
  void printPacket(dataPacket data) {
      Serial.printf("Hello Counter received nº %d\n", data.counter);
      // Dsiplay information
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

  /**
   * @brief Iterate through the payload of the packet and print the counter of the packet
   *
   * @param packet
   */
  void printDataPacket(AppPacket<dataPacket>* packet) {
      Serial.printf("Packet arrived from %X with size %d\n", packet->src, packet->payloadSize);

      //Get the payload to iterate through it
      dataPacket* dPacket = packet->payload;
      size_t payloadLength = packet->getPayloadLength();

      for (size_t i = 0; i < payloadLength; i++) {
          //Print the packet
          printPacket(dPacket[i]);
      }
  }
#endif // SYSTEM_LORAMESHER_RECEIVER_TEST


// Function that process the received packets
void processReceivedPackets(void*) {
    for (;;) {
        /* Wait for the notification of processReceivedPackets and enter blocking */
        ulTaskNotifyTake(pdPASS, portMAX_DELAY);

        //Iterate through all the packets inside the Received User Packets Queue
        while (loramesher->radio.getReceivedQueueSize() > 0) {
            Serial.println("ReceivedUserData_TaskHandle notify received");
            Serial.printf("Queue receiveUserData size: %d\n", loramesher->radio.getReceivedQueueSize());

            //Get the first element inside the Received User Packets Queue
            AppPacket<dataPacket>* packet = loramesher->radio.getNextAppPacket<dataPacket>();

            #ifdef SYSTEM_LORAMESHER_RECEIVER_TEST
              //Print the data packet
              printDataPacket(packet); // This is the actual handling TODO expand
            #else 
              // TODO parse and dispatch
              Serial.println("LoRaMesh packet received - no handler defined yet");
            #endif
            //Delete the packet when used. It is very important to call this function to release the memory of the packet.
            loramesher->radio.deletePacket(packet);
        }
    }
}

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


void System_LoraMesher::setup() {
  Serial.println("Loramesher setup");
  // Error codes are buried deep  .pio/libdeps/temploramesher/RadioLib/src/TypeDef.h
  // -12 is invalid frequency usually means band and module are not matched.
  radio.begin(config);        //Init the loramesher with a configuration
  createReceiveMessages();    //Create the receive task and add it to the LoRaMesher
  radio.setReceiveAppDataTaskHandle(receiveLoRaMessage_Handle); //Set the task handle to the LoRaMesher
  gpio_install_isr_service(ESP_INTR_FLAG_IRAM); // ToDO needs a parameter - guessing this based on a call I saw in LoRaMesher code
  radio.start();     //Start LoRaMesher
}


void System_LoraMesher::periodically() {
  #ifdef SYSTEM_LORAMESHER_SENDER_TEST
    Serial.printf("Send packet %d\n", dataCounter);
    dataPacket* data = new dataPacket;
    data->counter = dataCounter++;
    oled->display.clearDisplay();
    oled->display.setCursor(0,0);
    oled->display.println("LORAMESH SENDER");
    oled->display.setCursor(0,20);
    oled->display.setTextSize(1);
    oled->display.print("LoRa packet sent.");
    oled->display.setCursor(0,30);
    oled->display.print("Counter:");
    oled->display.setCursor(50,30);
    oled->display.print(dataCounter);      
    oled->display.display();
    radio.createPacketAndSend(BROADCAST_ADDR, data, 1); // Size is number of datapackets.
  #endif
}

#if !defined(SYSTEM_LORAMESHER_SENDER_TEST) && !defined(SYSTEM_LORAMESHER_RECEIVER_TEST)
  System_LoraMesher::publish(const String &topicPath, const String &payload, const bool retain, const int qos) {
    // Build a struc - unclear how to handle strings
    dataPacket* data = new dataPacket;
      TODO figure out how to build packet of Strings
    radio.createPacketAndSend(BROADCAST_ADDR, data, 1); // Size is number of datapackets. 
  }
#endif

#endif // SYSTEM_LORAMESHER_WANT
