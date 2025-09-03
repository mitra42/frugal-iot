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
// defined in _settings.h if board has LoRa, can also define in platformio.ini if e.g. have a LoRa shield
#ifdef SYSTEM_LORAMESHER_WANT 

// This is currently only defined for ESP32, 
// Certainly fials to complie on ESP8266 but might be fixable - I dont have a ESP8266+LoRa combo to try
#ifdef ESP32

#include "LoraMesher.h"
#include "system_loramesher.h"
#include "misc.h"  // for lprintf
#include "system_frugal.h"
#include "system_mqtt.h"

#define QOS_DOWNSTREAM 12 // Special value of retain when downstream message (MQTT broker->gateway->LoRa->modules)
#ifndef SYSTEM_LORAMESHER_MAXLEGITPACKET
  #define SYSTEM_LORAMESHER_MAXLEGITPACKET 256 // Seeing some big (1Mb) bad packets from sender
#endif
// =========== MeshSubscription class - only used by LoRaMesher to track subs ====

// TODO-152A Consider timeout of LM subscriptions - since a node may get a different id each time it power cycles, and thus have multiple entries in the gateway's meshsubscription table.
// TODO-152A OR (better) look for a change in gateway node, and if so resend,
MeshSubscription::MeshSubscription(const String topicPath, const uint16_t src) 
: topicPath(topicPath), src(src) {}


// ====== CALLBACKS AND CODE OUTSIDE CLASS =============

// Used in task creation
TaskHandle_t receiveLoRaMessage_Handle = NULL;

#ifdef SYSTEM_LORAMESHER_DEBUG
  // If you want to debug, provide a function in our .ino (or main.cpp) that implements this, 
  // see examples/loramesher/loramesher.ino for an example that writes to OLED
  extern void printAppData();
#endif // SYSTEM_LORAMESHER_DEBUG

// Function that process the queue of received packets - it receives an AppPacket<xxx> and passes to handler
void processReceivedPackets(void*) {
    for (;;) {
        /* Wait for the notification of processReceivedPackets and enter blocking */
        ulTaskNotifyTake(pdPASS, portMAX_DELAY);
        // TODO-137 move much of this into a method on System_LoraMesher
        //Iterate through all the packets inside the Received User Packets Queue
        while (frugal_iot.loramesher->radio.getReceivedQueueSize() > 0) {
            Serial.printf("LoRaMesher Received packets: %d\n", frugal_iot.loramesher->radio.getReceivedQueueSize());

            //Get the first element inside the Received User Packets Queue
            //LORAMESHER-STRUCTURED: AppPacket<counterPacket>* packet = frugal_iot.loramesher->radio.getNextAppPzacket<counterPacket>(); printAppCounterPacket(packet); // This is the actual handling
            // Note its <uint8_t>* because its a string
            AppPacket<uint8_t>* appPacket = frugal_iot.loramesher->radio.getNextAppPacket<uint8_t>();
            frugal_iot.loramesher->processReceivedPacket(appPacket);
        }
    }
}

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


// ============ HELPERS =====================
System_LoraMesher::System_LoraMesher()
: System_Base("loramesher", "LoraMesher"),
    radio(LoraMesher::getInstance()),
    config(LoraMesher::LoraMesherConfig())
{
    config.loraCs = LORA_CS;
    config.loraRst = LORA_RST;
    config.loraIrq = LORA_IRQ;
    #ifdef LORA_D1  // e.g. on ttgo-lora32-v21new
      config.loraIo1 = LORA_D1;  // Requirement for D1 may mean it won't work on ARDUINO_TTGO_LoRa32_V1 or _V2 but will on _V21
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

// setup points radio at receiveLoRaMessage_Handle which is set to processReceivedPackets in createReceiveMessages()
// Could parameterize this, but working on assumption that this will only ever point to the MQTT handler once its all working
void System_LoraMesher::setup() {
  Serial.println(F("Loramesher setup"));
  // Nothing to read from disk so not calling readConfigFromFS 

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
      Serial.println(F("Setup did not find a gateway node - expect after receive routing"));
  }
  checkRole(); // Probably wont do anything as MQTT probably not up yet - but will repeat in periodically
  #ifdef SYSTEM_LORAMESHER_DEBUG
    printAppData();
  #endif
}

void System_LoraMesher::prepareForSleep() {
    // TODO-139 TODO-23 find where put radio and SPI to sleep - on some other libraries its LoRa.sleep() and SPI.end()
}

// Check and update our role based on whether we have upstream WiFi/MQTT
// Have to be a bit careful with this, MQTT goes on and off frequently, but router tables
// are updated slowly (minute?) so do not want to flap them. Also sleep can switch WiFi off. 
// Best strategy probably to turn on quickly, but be slow to turn off only if MQTT doesn't come back
#ifndef SYSTEM_LORAMESHER_MQTTWAIT
  #define SYSTEM_LORAMESHER_MQTTWAIT 60000 // Allow it to be down for up to a minute before dropping gateway
#endif

void System_LoraMesher::periodically() {
  checkRole(); // Adjust routing tables to reflect if we have a MQTT connection or not
  #ifdef SYSTEM_LORAMESHER_DEBUG
    printAppData();
  #endif
}
// ========= INCOMING - UP OR DOWNSTEAM =========


// Note that received Packet could be Downstream (from MQTT broker via gateway) 
// or Upstream (from another node)
void System_LoraMesher::processReceivedPacket(AppPacket<uint8_t>* appPacket) {
  rcvdPacketCounter++;
  if ((appPacket->payloadSize > SYSTEM_LORAMESHER_MAXLEGITPACKET) ||(appPacket->payloadSize <= 0)) {
    Serial.printf("LoRaMesher Received bad packet length=%d\n", appPacket->payloadSize);
  } else {
    // Serial.print(F("XXX payloadSize=")); Serial.println(appPacket->payloadSize);
    const uint8_t qos = appPacket->payload[0] - '0';
    const bool downstream = appPacket->payload[0] == ('0'+QOS_DOWNSTREAM);
    const bool retain = appPacket->payload[1] - '0'; // will be 0 or 1
    // Assume appPacket->payload is a uint8_t* or char* and is null-terminated from [2] onward
    const char* str = (const char*)&appPacket->payload[2];
    String topicPath;
    String payload;

    const char* eq = strchr(str, ':');
    if (eq) {
        topicPath = String(str).substring(0, eq - str);
        payload = eq+1;
    } else { // Shouldnt have this case
        topicPath = str;
        payload = String();
    }
    #ifdef SYSTEM_LORAMESHER_DEBUG
      lastTopicPath = topicPath;
      lastPayload = payload; 
      printAppData(); // See note above about this being an extern
    #endif
    Serial.print(F("LoRaMesher received ")); Serial.print(topicPath); Serial.print(F("=")); Serial.println(payload);

    // How do we tell if this is an message heading upstream (from node to MQTT) or downstream (from MQTT on gateway node or other node via the router)
    // Three cases
    // a: message downstream from Broker: src=gateway dst=thisLeaf 
    // b: message reflected at gateway: src=gateway dst=thisLeaf
    // c: message outgoing to us as gateway: src=anotherLeaf dst=thisLeaf/gateway
    // Cant use src as there may be multiple gateways AND could be message reflected at gateway
    // For now - may change this - use retain=12 (character is '<' on "a" and "b"
    if (topicPath == "subscribe") { // Will always be UPSTREAM
      Serial.print(F("LoRaMesher forwarding subscription to MQTT ")); Serial.println(payload);
      // Need to remember the subscription before calling subscribe, because there may be retained data returned immediately
      meshSubscriptions.emplace_front(payload, appPacket->src);
      frugal_iot.messages->subscribe(payload);  // Should queue for MQTT since we are the gateway
    }
    if (downstream) {
      //Serial.print(F("XXX " __FILE__)); Serial.print(F("downstream ")); Serial.println(topicPath);
      frugal_iot.messages->dispatch(topicPath, payload);
    } else { // upstream (not subscribe)
      Serial.print(F("LoRaMesher forwarding to MQTT:")); Serial.print(topicPath); Serial.print(F("=")); Serial.println(payload);
      frugal_iot.messages->send(topicPath, payload, retain, qos); // Should queue for MQTT since we are the gateway
    }
  }
  //Delete the packet when used. It is very important to call this function to release the memory of the packet.
  radio.deletePacket(appPacket);
}


// ========= OUTGOING - UP OR DOWNSTEAM =========

// addGatewayRole is called on receiver during setup, once route tables propogate this should start seeing hte gateway
bool System_LoraMesher::findGatewayNode() {
    RouteNode* rn = radio.getClosestGateway();
    if (rn) {
      if (gatewayNodeAddress == BROADCAST_ADDR) {
        gatewayNodeAddress = rn->networkNode.address;
        Serial.print(F("LoRaMesher - newly found gateway node ")); Serial.println(gatewayNodeAddress);
      }
      return true;
    } else {
      if (gatewayNodeAddress != BROADCAST_ADDR) {
        Serial.print(F("LoRaMesher - lost gateway node"));
        gatewayNodeAddress = BROADCAST_ADDR;
      }
      return false;
    }
}

// Do we have a LoRaMNesher connection to a gateway upstream ? 
// This is used by System_Messages to decide whether to use LoRaMesher (or MQTT)
bool System_LoraMesher::connected() {
  return findGatewayNode();
}

// Common part to both relayDownstream and publish (which is upstream)
void System_LoraMesher::buildAndSend(uint16_t destn, const String &topic, const String &payload, bool retain, int qos) {
  // TODO it would be nice to use a structure, but LoraMesher doesnt support a structure with two unknown string lengths
  char qos_char = '0' + qos; // 0 1 2 as for MQTT incoming=12
  char retain_char = '0' + retain;
  const uint8_t* stringymessage = lprintf(100, "%c%c%s:%s", // Creates new buffer, exolicitly deleted below
    qos_char, retain_char,
    topic.c_str(), payload.c_str());
  size_t msglen = strlen(reinterpret_cast<const char*>(stringymessage))+1; // +1 to include terminating \0
  // Allocate enough memory for the struct + message
  //uint8_t* msg = (uint8_t*) malloc(msglen); // explicitly free-d below
  //DataPacket* dPacket = (DataPacket*) malloc(sizeof(DataPacket) + msglen); // sendPacket wants uint8_t*
  // Copy the string into the message array
  //memcpy(msg, stringymessage, msglen);
  //delete(stringymessage);
  // TODO-152 - maybe could just cast stringmessage as (uint8_t*) instead of copying
  frugal_iot.loramesher->sentPacketCounter++;
  // TODO-152 remove the const_cast once https://github.com/LoRaMesher/LoRaMesher/pull/83 is merged
  frugal_iot.loramesher->radio.sendPacket(destn, const_cast<uint8_t*>(stringymessage), msglen); // Will be broadcast if no node
  delete(stringymessage); // msg is copied in createPacketAndSend
}

// ========= UPSTREAM  ==================

// This is called by System_MQTT::subscribe or System_MQTT::messageSendInner when have no WiFi 
// but do have LoraMesher
bool System_LoraMesher::publish(const String &topic, const String &payload, bool retain, int qos) {
  if (connected()) {  // Have upstream path
    buildAndSend(frugal_iot.loramesher->gatewayNodeAddress, topic, payload, retain, qos);
    return true;
  } else {
    return false; // Leave on queue if no gateway node. (shouldnt happen as should only be called when connected)
  }
}

// Check if we can act as an upstream gateway and if changes update routing tables
// TODO-23 consider interaction of this with sleep modes, when come back from sleep won't have 
// TODO-23 MQTT yet, but also unclear if want a gateway role retained during sleep
LoraMesherMode System_LoraMesher::checkRole() {
  if (frugal_iot.mqtt->connected()) {
    if (!radio.isGatewayRole()) {
      #ifdef SYSTEM_LORAMESHER_DEBUG
        Serial.println(F("Adding gateway role")); 
      #endif
      radio.addGatewayRole();
    }
    return LORAMESHER_GATEWAY;
  } else { // Not connected to MQTT
    // Wait a little while before removing gateway role
    if (radio.isGatewayRole() && (millis() > (lostMQTTat + SYSTEM_LORAMESHER_MQTTWAIT))) {
      #ifdef SYSTEM_LORAMESHER_DEBUG
        Serial.println(F("LoRaMesher removing gateway role as lost MQTT"));
      #endif
      radio.removeGatewayRole();
    } 
    return connected() ? LORAMESHER_NODE : LORAMESHER_UNCONNECTED;
  }
}
const __FlashStringHelper* System_LoraMesher::checkRoleString() {
  switch (checkRole()) {
    case LORAMESHER_GATEWAY: return T->LoraMesher_Gateway;
    case LORAMESHER_NODE: return T->LoraMesher_Node;
    case LORAMESHER_UNCONNECTED: return T->LoraMesher_Unconnected;
  }
  return F("UNKNOWN");
}

// ========= DOWNSTREAM =================

void System_LoraMesher::relayDownstream(uint16_t destn, const String &topic, const String &payload) {
  buildAndSend(destn, topic, payload, false, QOS_DOWNSTREAM); // retain=12 gives a ascii character "<"
}

// Catch downstream messages received over WiFi by MQTT and then matching a subscription we know
// Note - this (I think) will catch outgoing messages that also match a subscription. 
void System_LoraMesher::dispatchPath(const String &topicPath, const String &payload) {
  for (auto &sub : meshSubscriptions) { 
    // Find matching subscriptions
    if (match_topic(topicPath, sub.topicPath)) { // Allows for + and # wildcards
      //Serial.print(F("XXX ")); Serial.print(topicPath); Serial.print(F(" matches ")); Serial.println(topicPath);
      // send over LoRaWan to subscriber
      relayDownstream(sub.src, topicPath, payload);
    }
  }
}

#endif // ESP32
#endif // SYSTEM_LORAMESHER_WANT
