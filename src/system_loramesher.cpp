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
 *  AppPacket<xxx> = src dst payload=xxx
 * 
 * Required
 * SYSTEM_LORAMESHER_MODULE either LoraMesher::LoraModules::SX1278_MOD or ...::SX1276_MOD
 */

#include "_settings.h"
#ifdef SYSTEM_LORAMESHER_WANT // defined in platformio.ini - //TODO-152 remove this when code stable

// This is currently only defined for ESP32, 
// Certainly fials to complie on ESP8266 but might be fixable - I dont have a ESP8266+LoRa combo to try
#ifdef ESP32

//#error "SYSTEM_LORAMESHER code is known not to be finished - see issue # 152 - uncomment to develop"

#include "LoraMesher.h"
#include "system_loramesher.h"
#include "misc.h"  // for lprintf
#include "system_frugal.h"
#include "system_mqtt.h"

#define INCOMINGRETAIN 12 // Special value of retain when incoming message

// TODO-152 Consider timeout of LM subscriptions - since a node may get a different id each time it power cycles, and thus have multiple entries in the gateway's meshsubscription table.
MeshSubscription::MeshSubscription(const String topicPath, const uint16_t src) 
: topicPath(topicPath), src(src) {}


System_LoraMesher::System_LoraMesher()
: System_Base("loramesher", "LoraMesher"),
    radio(LoraMesher::getInstance()),
    config(LoraMesher::LoraMesherConfig())
{
    config.loraCs = LORA_CS;
    config.loraRst = LORA_RST;
    config.loraIrq = LORA_IRQ;
    #ifdef LORA_D1  // e.g. on ttgo-lora32-v21new
      config.loraIo1 = LORA_D1;  // Requirement for D1 may mean it won't work on ARDUINO_TTGO_LoRa32_v1 or _V2 but will on _V21
    #elif defined(LORA_DIO1) // e.g. on variant:lilygo_t3_s3_sx127x 
      config.loraIo1 = LORA_DIO1;  // Requirement for D1 may mean it won't work on ARDUINO_TTGO_LoRa32_v1 or _V2 but will on _V21
    #else
      #error unclear what to define loraIo1 at on this board
    #endif
    config.module = SYSTEM_LORAMESHER_MODULE;
    config.freq = SYSTEM_LORAMESHER_BAND;
    // This line is required because of some weirdness between different boards
    // LoRaMesher uses a default SPI.begin which picks up SCK MISO, MOSI, CS rather than LORA_MISO etc
    // ttgo-lora32-v21new defines LORA_SCK the same as MISO, MOSI, CS etc so it works but
    // lilygo_t3_s3_sx127x howwever defines SCK, MISO etc as the SD's SPI so LoraMesher fails 
    // https://github.com/LoRaMesher/LoRaMesher/pull/78 submitted so this is not needed.
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    config.spi = &SPI;
}



// LORAMESH-STRUCTURED: struct counterPacket {  uint32_t counter = 0; }; counterPacket* helloPacket = new counterPacket;

// TODO for now this is an extern as its not going to be the final interface so its not worth putting together a way to pass a callback
    // extern void printAppCounterPacket(AppPacket<counterPacket>*); // This was how we handled structured packets
    extern void printAppData(AppPacket<uint8_t>*); //TODO-152 probably remove

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
            //LORAMESHER-STRUCTURED: AppPacket<counterPacket>* packet = frugal_iot.loramesher->radio.getNextAppPzacket<counterPacket>(); printAppCounterPacket(packet); // This is the actual handling
            // Note its <uint8_t>* because its a string
            AppPacket<uint8_t>* appPacket = frugal_iot.loramesher->radio.getNextAppPacket<uint8_t>();
            frugal_iot.loramesher->processReceivedPacket(appPacket);
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

// Common part to both relayDownstream and publush
void System_LoraMesher::buildAndSend(uint16_t destn, const String &topic, const String &payload, bool retain, int qos) {
  // TODO it would be nice to use a structure, but LoraMesher doesnt support a structure with two unknown string lengths
  char qos_char = '0' + qos;
  char retain_char = '0' + retain;
  const char* stringymessage = lprintf(100, "%c%c%s:%s", 
    qos_char, retain_char,
    topic.c_str(), payload.c_str());
  size_t msglen = strlen(stringymessage)+1; // +1 to include terminating \0
  // Allocate enough memory for the struct + message
  uint8_t* msg = (uint8_t*) malloc(msglen);
  //DataPacket* dPacket = (DataPacket*) malloc(sizeof(DataPacket) + msglen); // sendPacket wants uint8_t*
  // Copy the string into the message array
  memcpy(msg, stringymessage, msglen);
  delete(stringymessage);
  // TODO-152 - maybe could just cast stringmessage as (uint8_t*) instead of copying
  frugal_iot.loramesher->sentPacketCounter++;
  frugal_iot.loramesher->radio.sendPacket(destn, msg, msglen); // Will be broadcast if no node
  free(msg); // msg is copied in createPacketAndSend
}
void System_LoraMesher::relayDownstream(uint16_t destn, const String &topic, const String &payload) {
  buildAndSend(destn, topic, payload, INCOMINGRETAIN, 0); // retain=12 gives a ascii character "<"
}
// This is called by System_MQTT::subscribe or System_MQTT::messageSendInner when have no WiFi 
// but do have LoraMesher
bool System_LoraMesher::publish(const String &topic, const String &payload, bool retain, int qos) {
  if (frugal_iot.loramesher->findGatewayNode()) { 
    buildAndSend(frugal_iot.loramesher->gatewayNodeAddress, topic, payload, retain, qos);
    return true;
  } else {
    return false; // Leave on queue if no gateway node.
  }
}

void System_LoraMesher::processReceivedPacket(AppPacket<uint8_t>* appPacket) {
  rcvdPacketCounter++;
  printAppData(appPacket); // TODO-151 see note above about this being an extern
  const uint8_t qos = appPacket->payload[0] - '0';
  const bool incoming = appPacket->payload[0] == ('0'+INCOMINGRETAIN);
  const bool retain = appPacket->payload[0] - '0'; // will be 0 or 1
  // Assume appPacket->payload is a uint8_t* or char* and is null-terminated from [2] onward
  const char* str = (const char*)&appPacket->payload[2];
  String topicPath, payload;

  const char* eq = strchr(str, ':');
  if (eq) {
      topicPath = String(str).substring(0, eq - str);
      payload = String(eq + 1);
  } else { // Shouldnt have this case
      topicPath = String(str);
      payload = "";
  }
  //Delete the packet when used. It is very important to call this function to release the memory of the packet.
  radio.deletePacket(appPacket);
  Serial.print("LoRaMesher received"); Serial.print(topicPath); Serial.println(payload);

  // How do we tell if this is an message heading upstream (from node to MQTT) or downstream (from MQTT on gateway node or other node via the router)
  // Three cases
  // a: message incoming from Broker: src=gateway dst=thisLeaf 
  // b: message reflected at gateway: src=gateway dst=thisLeaf
  // c: message outgoing to us as gateway: src=anotherLeaf dst=thisLeaf/gateway
  // Cant use src as there may be multiple gateways AND could be message reflected at gateway
  // For now - may change this - use retain=12 (character is '<' on "a" and "b"
   if (topicPath == "subscribe") {
    Serial.print("XXX " __FILE__); Serial.println("got subscribe");
    // Need to remember the subscription before calling subscribe, because there may be retained data returned immediately
    meshSubscriptions.emplace_front(payload, appPacket->src);
    frugal_iot.mqtt->subscribe(payload);
  }
  if (incoming) {
    Serial.print("XXX " __FILE__); Serial.print("incoming "); Serial.println(topicPath);
    frugal_iot.mqtt->dispatchPath(topicPath, payload);
  } else {
   frugal_iot.mqtt->messageSend(topicPath, payload, retain, qos);
  }
}


// addGatewayRole is called on receiver during setup, once route tables propogate this should start seeing hte gateway
bool System_LoraMesher::findGatewayNode() {
    RouteNode* rn = radio.getClosestGateway();
    if (rn) {
      if (gatewayNodeAddress == BROADCAST_ADDR) {
        Serial.print(F("LoRaMesher - newly found gateway node"));
      }
      gatewayNodeAddress = rn->networkNode.address;
      return true;
    } else {
      gatewayNodeAddress = BROADCAST_ADDR;
      return false;
    }
}
bool System_LoraMesher::connected() {
  return findGatewayNode();
}

// Catch incoming messages received over WiFi by MQTT and then matching a subscription we know
// Note - this (I think) will catch outgoing messages that also match a subscription. 
void System_LoraMesher::dispatchPath(const String &topicPath, const String &payload) {
  for (auto &sub : meshSubscriptions) { 
    // Find matching subscriptions
    if (match_topic(topicPath, sub.topicPath)) { // Allows for + and # wildcards
      Serial.print("XXX "); Serial.print(topicPath); Serial.print(" matches "); Serial.println(topicPath);
      // send over LoRaWan to subscriber
      relayDownstream(sub.src, topicPath, payload);
    }
  }
}

// setup points radio at receiveLoRaMessage_Handle which is set to processReceivedPackets in createReceiveMessages()
// Could parameterize this, but working on assumption that this will only ever point to the MQTT handler once its all working
void System_LoraMesher::setup() {
  Serial.println("Loramesher setup");
    //esp_log_level_set("*", ESP_LOG_VERBOSE);
    esp_log_level_set(LM_TAG, ESP_LOG_VERBOSE); // To get lots of logging from LoraMesher

  // Error codes are buried deep  .pio/libdeps/*/RadioLib/src/TypeDef.h
  // -12 is invalid frequency usually means band and module are not matched.
  // -16 is RADIOLIB_ERR_SPI_WRITE_FAILED suggesting wrong pins for SPI
  radio.begin(config);        //Init the loramesher with a configuration
  createReceiveMessages();    //Create the receive task and add it to the LoRaMesher
  radio.setReceiveAppDataTaskHandle(receiveLoRaMessage_Handle); //Set the task handle to the LoRaMesher
  //gpio_install_isr_service(ESP_INTR_FLAG_IRAM); // known error message - Jaimi says doesn't matter
  radio.start();     //Start LoRaMesher
  if (!findGatewayNode()) {
      Serial.println("Setup did not find a gateway node - expect after receive routing");
  }
  #ifdef SYSTEM_LORAMESHER_RECEIVER_TEST
  // TODO-152 should do this automatically if have MQTT connected but note LoRamesher routing is slow
  // so don't flap this
    radio.addGatewayRole(); 
  #endif
}

 void System_LoraMesher::prepareForSleep() {
    // TODO-139 TODO-23 find where put radio and SPI to sleep - on some other libraries its LoRa.sleep() and SPI.end()
  }

#endif // ESP32
#endif // SYSTEM_LORAMESHER_WANT
