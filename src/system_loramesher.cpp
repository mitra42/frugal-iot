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
 *  LoRaMeshMessage = appPrtDst appPrtSrc messageId // as used by LoRaChat
 *  FrugalIoTMessage = message // Will evolve to what needed for MQTT
 *  AppPacket<xxx> = src dst payload=xxx
 * 
 * Required
 * SYSTEM_LORAMESHER_MODULE either LoraMesher::LoraModules::SX1278_MOD or ...::SX1276_MOD
 */

#include "_settings.h"
#ifdef SYSTEM_LORAMESHER_WANT // defined in platformio.ini

// This is currently only defined for ESP32, 
// Certainly fials to complie on ESP8266 but might be fixable - I dont have a ESP8266+LoRa combo to try
#ifdef ESP32

//#error "SYSTEM_LORAMESHER code is known not to be finished - see issue # 152 - uncomment to develop"

#include "LoraMesher.h"
#include "system_loramesher.h"
#include "misc.h"  // for lprintf
#include "system_frugal.h"

// These settings duplicated in system_loramesher.cpp and system_lora.cpp (system_loramesher.cpp are newer)
// For ARDUINO_TTGO_LoRa32 
// LORA_SCK; LORA_MISO; LORA_MOSI; LORA_CS; LORA_RST; and for V21 LORA_D1 are defined correctly in
// e.g. ~/Arduino15/packages/esp32/hardware/esp32/3.2.0/variants/ttgo-lora32-v21new/pins_arduino.h etc
#if defined(ARDUINO_TTGO_LoRa32_v1)
  #define LORA_D0 LORA_IRQ
#elif defined(ARDUINO_TTGO_LoRa32_v2) || defined(ARDUINO_TTGO_LoRa32_v21new) // V3 is almost same as V2
  #define LORA_D0 LORA_IRQ
  // Note on V2 but not V21 LORA_D1 is not defined and may need a physical wire to GPIO 33
#else
  #error "Unsupported LORA configuration. Please define either ARDUINO_TTGO_LoRa32_v1 or ARDUINO_TTGO_LoRa32_v2. or define new BOARD"
#endif

System_LoraMesher::System_LoraMesher()
: System_Base("loramesher", "LoraMesher"),
    radio(LoraMesher::getInstance()),
    config(LoraMesher::LoraMesherConfig())
{
    config.loraCs = LORA_CS;
    config.loraRst = LORA_RST;
    config.loraIrq = LORA_IRQ;
    config.loraIo1 = LORA_D1;  // Requirement for D1 may mean it won't work on ARDUINO_TTGO_LoRa32_v1 or _V2 but will on _V21
    config.module = SYSTEM_LORAMESHER_MODULE;
    config.freq = SYSTEM_LORAMESHER_BAND;
}

#if defined(SYSTEM_LORAMESHER_TEST_COUNTER)
  uint32_t dataCounter = 0;
  struct counterPacket {
      uint32_t counter = 0;
  };
  counterPacket* helloPacket = new counterPacket;
#elif defined(SYSTEM_LORAMESHER_TEST_STRING) || defined(SYSTEM_LORAMESHER_TEST_MQTT)
  uint32_t dataCounter = 0;
#else 
  #error Must define SYSTEM_LORAMESHER_TEST_COUNTER or SYSTEM_LORAMESHER_TEST_STRING or (SYSTEM_LORAMESHER_TEST_MQTT
#endif

// TODO for now this is an extern as its not going to be the final interface so its not worth putting together a way to pass a callback
#ifdef SYSTEM_LORAMESHER_RECEIVER_TEST
  #if defined(SYSTEM_LORAMESHER_TEST_COUNTER)
    extern void printAppCounterPacket(AppPacket<counterPacket>*);
  #elif defined(SYSTEM_LORAMESHER_TEST_STRING) || defined(SYSTEM_LORAMESHER_TEST_MQTT)
    extern void printAppData(AppPacket<uint8_t>*);
  #endif

  // Function that process the received packets - it receives an AppPacket<xxx> and passes to handler
void processReceivedPackets(void*) {
    for (;;) {
        /* Wait for the notification of processReceivedPackets and enter blocking */
        ulTaskNotifyTake(pdPASS, portMAX_DELAY);
        // TODO-137 move much of this into a method on System_LoraMesher
        //Iterate through all the packets inside the Received User Packets Queue
        while (frugal_iot.loramesher->radio.getReceivedQueueSize() > 0) {
            Serial.println("ReceivedUserData_TaskHandle notify received");
            Serial.printf("Queue receiveUserData size: %d\n", frugal_iot.loramesher->radio.getReceivedQueueSize());

            //Get the first element inside the Received User Packets Queue
            #if defined(SYSTEM_LORAMESHER_TEST_COUNTER)
              AppPacket<counterPacket>* packet = frugal_iot.loramesher->radio.getNextAppPzacket<counterPacket>();
              //Print the App Packet
              printAppCounterPacket(packet); // This is the actual handling
            #elif defined(SYSTEM_LORAMESHER_TEST_STRING) || defined(SYSTEM_LORAMESHER_TEST_MQTT)
              // Note its <uint8_t>* because its a string
              AppPacket<uint8_t>* appPacket = frugal_iot.loramesher->radio.getNextAppPacket<uint8_t>();
              printAppData(appPacket); // TODO-151 see note above about this being an extern
            #endif      
            //Delete the packet when used. It is very important to call this function to release the memory of the packet.
            frugal_iot.loramesher->radio.deletePacket(appPacket);
        }
    }
}

// Used in task creation
TaskHandle_t receiveLoRaMessage_Handle = NULL;

// Create a Receive Messages Task and add it to the LoRaMesher
// Equivalent of system_mqtt's: client.onMessage(MessageReceived)
void createReceiveMessages() {
    int res = xTaskCreate(
        processReceivedPackets, // TaskFunction_t
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

// addGatewayRole is called on receiver during setup, once route tables propogate this should start seeing hte gateway
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

// setup points radio at receiveLoRaMessage_Handle which is set to processReceivedPackets in createReceiveMessages()
// Could parameterize this, but working on assumption that this will only ever point to the MQTT handler once its all working
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
  if (!findGatewayNode()) {
      Serial.println("Setup did not find a gateway node - expect after recieve routing");
  }
  #ifdef SYSTEM_LORAMESHER_RECEIVER_TEST
    radio.addGatewayRole(); 
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

#endif // ESP32
#endif // SYSTEM_LORAMESHER_WANT
