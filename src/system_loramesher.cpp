/* Frugal IoT - LoRaMesher wrapper 
 *
 * Based on examples at https://github.com/LoRaMesher/LoRaMesher
 * 
 * Issues: https://github.com/mitra42/frugal-iot/issues/152 (and also 137 (LoRa) a bit)
 * 
 * Notes:
 *    This uses the "radio" library for LoRa rather than Sandeep Mishra's 
 * 
 * Overview:
 *   This library works supports multiple roles for LoRaMesher (LM)
 *     Nodes configure LoRa and create a routing table, 
 *     The Routing includes info about which nodes are connected to the internet (typically via WiFi)
 *     Messages are semantically the same as MQTT messages(topic and payload as for WiFi) preceeded by Retain and QOS
 *     Nodes not connected to WiFi send "Upstream", possibly relayed (inside the LM layer) by other nodes  to a Gateway
 *     Gateways keep track of any that are subscriptions and remember which node the subscription came from 
 *     Gateways queue upstream messages for the MQTT/WiFi to send to the server.
 *     Gateways check incoming messages from MQTT/WiFi using the same "dispatchLeaf" mechanism that the rest of FrugalIoT uses
 *     If incoming messages match a subscription they are routed to the destination node. 
 *     There is a side-effect that if an outgoing message matches a subscription it will also be routed to that node (probably both directly and via MQTT/WiFi)
 *
 * Required
 *     TODO-189 this has changed, check after decide which parameters are required/optional to be in platformio.ini
 * 
 * Open issues with LoRaMesher 1.0.0
 * BROADCAST_ADDR used to be defined, what is the equivalent now?
 * Do we need to delete the std::vector passed to the packet handling callback, it looks like its on the stack so OK?
 * LoraMesher issue#89 about running mesher in its own task instead of the app
 * LoraMesher issue#88 about gateway functionality
 */

#include "_settings.h"
// defined in _settings.h if board has LoRa, can also define in platformio.ini if e.g. have a LoRa shield
#ifdef SYSTEM_LORAMESHER_WANT 


// This is currently only defined for ESP32, 
// Certainly fials to complie on ESP8266 but might be fixable - I dont have a ESP8266+LoRa combo to try
#ifdef ESP32

#include "system_loramesher.h"
#include "loramesher.hpp"
#include "system_frugal.h"
#include "misc.h"  // for lprintf

// These are board specific - check if in board or variant files and if not define in platformio.ini for each board
// TODO-189 Jamie: This is a section, that IMHO should be inside LoraMesher, so it works with all the different boards with 
// inconsistent pins_arduino.h

#if defined(ARDUINO_TTGO_LoRa32_v21new) // V2 and V1 dont define LORA_D1 so unsure what they need
  // Defines LORA_RST LORA_IRQ LORA_SCK LORA_MISO LORA_MOSI LORA_CS in pins_arduino.h
  #define LORA_IO1 LORA_D1
  #define LORA_RADIO_TYPE loramesher::RadioType::kSx1276
#elif defined(ARDUINO_LILYGO_T3_S3_V1_X)
  // Defines LORA_RST LORA_IRQ LORA_SCK LORA_MISO LORA_MOSI LORA_CS in pins_arduino.h
  #define LORA_IO1 LORA_DIO1
  #define LORA_RADIO_TYPE loramesher::RadioType::kSx1278
#elif defined(ARDUINO_heltec_wifi_lora_32_V3)
  // Defines: SCK MISO MOSI SS, which it uses for the SPI to the LoRa which LoRamesher picks up correctly since LORA_SCK etc not defined
  #define LORA_RADIO_TYPE loramesher::RadioType::kSx1276
  #define LORA_RST RST_LoRa
  #define LORA_CS SS            // GPIO8
  #define LORA_IRQ DIO0         // Bizarre swap with Busy - not sure if documentation error or what but heltecs have notoriously faulty docs
  #define LORA_IO1 BUSY_LoRa 
#elif !defined(LORA_CS) || !defined(LORA_IRQ) !! !defined(LORA_RADIO_TYPE) || !defined(LORA_IO1) || !defined(LORA_RST) || !defined(LORA_MOSI) || !defined(LORA_MISO)
  #error LORA parameters not defined, but defined SYSTEM_LORAMESHER_WANT
#endif

// This set of variables are specific to the Frugal-IoT mesh network, all devices should have the same.

#define LORA_ADDRESS 0  // Set 0 for auto-address

// TODO-189 Should be defined in platformio.ini or each wrapped in #ifndef, and replace LORA_ with SYSTEM_LORAMESHER_
// Suggest to Jamie that these (except frequency) are also defaulted in the library
#define LORA_SPREADING_FACTOR 7U
#define LORA_BANDWITH 125.0
#define LORA_CODING_RATE 7U
#define LORA_POWER 6
#define LORA_SYNC_WORD 20U
#define LORA_CRC true
#define LORA_PREAMBLE_LENGTH 8U
// Intentionally not defining the frequency - the developer MUST specify this in their platformio.ini as it is country specific 
#define LORA_FREQUENCY 915.0F // Note there are country specific laws as to what this needs to be !  //TODO-189 move to platform.ini

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

#ifdef SYSTEM_LORAMESHER_DEBUG
  // If you want to debug, provide a function in our .ino (or main.cpp) that implements this, 
  // see examples/loramesher/loramesher.ino for an example that writes to OLED
  extern void printAppData();
#endif // SYSTEM_LORAMESHER_DEBUG

// Simple data received callback (recommended approach)
void OnDataReceived(loramesher::AddressType source, const std::vector<uint8_t>& data) {
    // Recommendation: Forward to separate task for processing
    #ifdef SYSTEM_LORAMESHER_DEBUG
      Serial.print("Received data from: 0x"); Serial.print(source, HEX); Serial.print("("); Serial.print(data.size()); Serial.println(" bytes)");
    #endif
    // Should really be happening in separate task - but not sure how many tasks there are?
    frugal_iot.loramesher->processReceivedPacket(source, data);
}

#ifdef TODO_189_DONT_THINK_NEED_THIS
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
#endif

// ============ HELPERS =====================
bool System_LoraMesher::initialize() {
  try {
    loramesher::RadioConfig radioConfig(LORA_RADIO_TYPE, LORA_FREQUENCY,
                            LORA_SPREADING_FACTOR, LORA_BANDWITH,
                            LORA_CODING_RATE, LORA_POWER, LORA_SYNC_WORD,
                            LORA_CRC, LORA_PREAMBLE_LENGTH);
    loramesher::PinConfig pinConfig(LORA_CS,   // NSS pin
                        LORA_RST,  // Reset pin
                        LORA_IRQ,  // DIO0 pin
                        LORA_IO1   // DIO1 pin
    );
    loramesher::LoRaMeshProtocolConfig mesh_config(0,       // Auto-assign node address
                                       60000,   // Hello interval: 60 seconds
                                       180000,  // Route timeout: 180 seconds
                                       10);     // Max hops: 10
    loramesher::ProtocolConfig protocol_config;
    protocol_config.setLoRaMeshConfig(mesh_config);

    // Create LoraMesher with PingPong protocol
      mesher =
          loramesher::LoraMesher::Builder()
              .withRadioConfig(radioConfig)
              .withPinConfig(pinConfig)
              .withLoRaMeshProtocol(mesh_config)  // Use LoRaMesh protocol
              .withAutoAddressFromHardware(true)  // Enable hardware-based addressing (default)
              .Build();
      #ifdef SYSTEM_LORAMESHER_DEBUG
        Serial.print("LoRaMesher node address: 0x"); Serial.println(mesher->GetNodeAddress());
      #endif
      //Set up data callback
      mesher->SetDataCallback(OnDataReceived);
      // Start the network
      loramesher::Result result = mesher->Start();
      if (!result) {
        // Report error even if !SYSTEM_LORAMESHER_DEBUG
        Serial.println(F("Failed to start LoraMesher:")); Serial.println(result.GetErrorMessage().c_str());
        return 1; 
      }
      #ifdef SYSTEM_LORAMESHER_DEBUG
        Serial.println(F("LoraMesher started successfully"));
      #endif
      
      #ifdef SYSTEM_LORAMESHER_DEBUG
        auto mesh_protocol = mesher->GetLoRaMeshProtocol();
        if (mesh_protocol) {
          // Set up route update callback
          mesh_protocol->SetRouteUpdateCallback(
            [](bool route_updated, loramesher::AddressType destination,
              loramesher::AddressType next_hop, uint8_t hop_count) {
                if (route_updated) {
                  Serial.print(F("Route updated - Destination: ")); Serial.print(destination); 
                  Serial.print(F(", Next hop: ")); Serial.print(next_hop); 
                  Serial.print(F(", Hops: ")); Serial.println(hop_count);
                } else {
                    Serial.print(F("Route removed for destination: ")); Serial.println(destination);
                }
            });
      }
      #endif // SYSTEM_LORAMESHER_DEBUG
      // TODO-189 Would be better with the `mesher` running in the other task putting messages in a queue, not the app
      /*
      // Create the application task 
      os::TaskHandle_t app_task_handle = nullptr;
      os::RTOS::instance().CreateTask(appTask, "Receive_LoRa_Message",
                                      4096,  // Stack size
                                      mesher.get(),
                                      2,  // Priority
                                      &app_task_handle);
      */

    } catch (const std::exception& e) {
        LOG_ERROR("Exception: %s", e.what());
        return 1;
    }
    return 0;
}

System_LoraMesher::System_LoraMesher()
: System_Base("loramesher", "LoraMesher")
#ifdef TODO_189_DONT_THINK_NEED_THIS
    radio(LoraMesher::getInstance()),
    config(LoraMesher::LoraMesherConfig())
#endif //TODO_189_DONT_THINK_NEED_THIS
{
#ifdef TODO_189_DONT_THINK_NEED_THIS
    config.loraCs = LORA_CS;
    config.loraRst = LORA_RST;
    config.loraIrq = LORA_IRQ;
    config.loraIo1 = LORA_IO1; 
    config.module = SYSTEM_LORAMESHER_MODULE;
    config.freq = SYSTEM_LORAMESHER_BAND;
    // Loramesher will initialize SPI on LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS (if config.spi is unset)
    // if need to override then best to set those values, else can override here on a per-board basis 
    //SPI.begin(SCK, MISO, MOSI, LORA_CS); //TODO-176 check and recheck if/why needed -
    //config.spi = &SPI;
#endif
}


// setup points radio at receiveLoRaMessage_Handle which is set to processReceivedPackets in createReceiveMessages()
// Could parameterize this, but working on assumption that this will only ever point to the MQTT handler once its all working
void System_LoraMesher::setup() {
  Serial.println(F("Loramesher setup"));
  // Nothing to read from disk so not calling readConfigFromFS 

  // Set in LoRaMesher/src/config/system_config.hpp so not needed here TODO-189 Jamie allow override in platformio.ini 
  //esp_log_level_set("*", ESP_LOG_VERBOSE);
  //esp_log_level_set(LM_TAG, ESP_LOG_VERBOSE); // To get lots of logging from LoraMesher

  if (initialize()) { // Creates 1, maybe 2 tasks
    Serial.println("LoRaMesher initialize failed - now what !");
  }

#ifdef TODO_189_DONT_THINK_NEED_THIS
  // Error codes are buried deep  .pio/libdeps/*/RadioLib/src/TypeDef.h
  // -2 is chip not found 
  // -12 is invalid frequency usually means band and module are not matched.
  // -16 is RADIOLIB_ERR_SPI_WRITE_FAILED suggesting wrong pins for SPI
  radio.begin(config);        //Init the loramesher with a configuration
  createReceiveMessages();    //Create the receive task and add it to the LoRaMesher
  radio.setReceiveAppDataTaskHandle(receiveLoRaMessage_Handle); //Set the task handle to the LoRaMesher
  //gpio_install_isr_service(ESP_INTR_FLAG_IRAM); // known error message - Jaimi says doesn't matter
  radio.start();     //Start LoRaMesher
#endif
  if (!findGatewayNode()) {
      Serial.println(F("Setup did not find a gateway node - expect after receive routing"));
  }
  checkRole(); // Probably wont do anything as MQTT probably not up yet - but will repeat in periodically
  #ifdef SYSTEM_LORAMESHER_DEBUG
    printAppData();
  #endif
}

void System_LoraMesher::prepareForLightSleep() {
    //TODO-23 this doesnt appear to be called, its not sure if it should or not
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
#ifdef SYSTEM_LORAMESHER_DEBUG
  // Not currently used
  void System_LoraMesher::printRouteTable() {
      auto routes = mesher->GetRoutingTable();
      Serial.print(F("Routing table size: ")); Serial.println(routes.size());
      for (const auto& route : routes) {
        Serial.print(F("  Destination: 0x")); Serial.print(route.destination, HEX);
        Serial.print(F(", Next hop: 0x")); Serial.print(route.next_hop, HEX);
        Serial.print(F(", Hops: "));  Serial.print(route.hop_count);
        Serial.print(F(", Valid: ")); Serial.println(route.is_valid ? "yes" : "no");
      }
  }
  void System_LoraMesher::printNetworkStatus() {
    auto status = mesher->GetNetworkStatus();
    Serial.print(F("Network status: State=")); Serial.print(static_cast<int>(status.current_state));
    Serial.print(F(", Manager=0x")); Serial.print(status.network_manager, HEX);
    Serial.print(F(", Slot=")); Serial.print(status.current_slot);
    Serial.println(F(", Nodes=")); Serial.println(status.connected_nodes);
  }

#endif // SYSTEM_LORAMESHER_DEBUG
// ========= INCOMING - UP OR DOWNSTEAM =========


// Note that received Packet could be Downstream (from MQTT broker via gateway) 
// or Upstream (from another node)
void System_LoraMesher::processReceivedPacket(loramesher::AddressType source, const std::vector<uint8_t>& data) {
  rcvdPacketCounter++;
  if ((data.size() > SYSTEM_LORAMESHER_MAXLEGITPACKET) ||(data.size() <= 0)) {
    Serial.printf("LoRaMesher Received bad packet length=%d\n", data.size());
  } else {
    // Serial.print(F("XXX payloadSize=")); Serial.println(appPacket->payloadSize);
    const uint8_t qos = data[0] - '0';
    const bool downstream = data[0] == ('0'+QOS_DOWNSTREAM);
    const bool retain = data[1] - '0'; // will be 0 or 1
    // Assume appPacket->payload is a uint8_t* or char* and is null-terminated from [2] onward
    const char* str = (const char*)&data[2];
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
      meshSubscriptions.emplace_front(payload, source);
      frugal_iot.messages->subscribe(payload);  // Should queue for MQTT since we are the gateway
    }
    if (downstream) {
      //Serial.print(F("XXX " __FILE__)); Serial.print(F("downstream ")); Serial.println(topicPath);
      frugal_iot.messages->queueIncoming(topicPath, payload);
    } else { // upstream (not subscribe)
      Serial.print(F("LoRaMesher forwarding to MQTT:")); Serial.print(topicPath); Serial.print(F("=")); Serial.println(payload);
      frugal_iot.messages->send(topicPath, payload, retain, qos); // Should queue for MQTT since we are the gateway
    }
  }
  //Delete the packet when used. It is very important to call this function to release the memory of the packet.
  //radio.deletePacket(appPacket); //TODO-189 Jaimie do we still do this?
}


// ========= OUTGOING - UP OR DOWNSTEAM =========



// addGatewayRole is called on receiver during setup, once route tables propogate this should start seeing hte gateway
bool System_LoraMesher::findGatewayNode() {
  return false; 
  #ifdef TODO_189 // need this functionality - not currently present
    RouteNode* rn = radio.getClosestGateway(); //TODO-189 equivalent?
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
  #endif
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
  sentPacketCounter++;
  // TODO-189 look at other example for how to send a packet
  // Convert const uint8_t* to std::vector<uint8_t> for mesher->Send()
  // TODO-189 Suggest Jamie overloads Send to take a char* + size as the old one did
  std::vector<uint8_t> messageVector(stringymessage, stringymessage + msglen);
  loramesher::Result send_result = mesher->Send(destn, messageVector); // Will be broadcast if no node
  if (!send_result) {
    Serial.print("LoRaMesher Failed to send:"); Serial.println(send_result.GetErrorMessage().c_str());
  }
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

//TODO-189 work this one out

// Check if we can act as an upstream gateway and if changes update routing tables
// TODO-23 consider interaction of this with sleep modes, when come back from sleep won't have 
// TODO-23 MQTT yet, but also unclear if want a gateway role retained during sleep
LoraMesherMode System_LoraMesher::checkRole() {
  if (frugal_iot.mqtt->connected()) {
    #ifdef TODO_189 // Need this functionality
      if (!radio.isGatewayRole()) { //TODO-189 find equivalent
        #ifdef SYSTEM_LORAMESHER_DEBUG 
          Serial.println(F("Adding gateway role")); 
        #endif
        radio.addGatewayRole();
      }
    #endif
    return LORAMESHER_GATEWAY;
  } else { // Not connected to MQTT
    #ifdef TODO_189 // Need this functionality
      // Wait a little while before removing gateway role
      if (radio.isGatewayRole() && (millis() > (lostMQTTat + SYSTEM_LORAMESHER_MQTTWAIT))) {
        #ifdef SYSTEM_LORAMESHER_DEBUG
          Serial.println(F("LoRaMesher removing gateway role as lost MQTT"));
        #endif
        radio.removeGatewayRole();
      } 
    #endif
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
