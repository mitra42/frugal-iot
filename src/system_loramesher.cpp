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
 * Need to integrate signals from LoRaMesher about when to sleep
 * Maybe ... should be adding gateway role earlier in process - seems to be after sending backlog of queue to MQTT
 * 
 * Open issues with this code
 * I think that a gateway node coming power cycling after a node is not going to see the subscription while the node thinks it sent it
 */

#include "_settings.h"

// defined in _settings.h if board has LoRa, can also define in platformio.ini if e.g. have a LoRa shield
#ifdef SYSTEM_LORAMESHER_WANT 

// This is currently only defined for ESP32, 
// Certainly fails to compile on ESP8266 but might be fixable - I dont have a ESP8266+LoRa combo to try
#ifdef ESP32

#include "system_loramesher.h"
#include "loramesher.hpp"
#include "system_frugal.h"
#include "misc.h"  // for lprintf
// These are board specific - check if in board or variant files and if not define in platformio.ini for each board
// TODO-189 Jamie: This is a section, that IMHO should be inside LoraMesher, so it works with all the different boards with 
// inconsistent pins_arduino.h

// Define maximum size of a message packet - this is topic + message + 2, (wired routes are usually the longest messages)
#ifndef SYSTEM_LORAMESHER_MAXMESSAGESIZE
  #define SYSTEM_LORAMESHER_MAXMESSAGESIZE 128
#endif
#ifndef LORAMESHER_LOG_LEVEL
  #define LORAMESHER_LOG_LEVEL 2 // Warning - set to 1 for Info and 0 for debug
#endif
// Define these two settings to speed things up when testing, undo later.
#define LORA_DUTY_CYCLE 1.0f
#define LORA_MIN_SLEEP_FRACTION 0

#if defined(ARDUINO_TTGO_LoRa32_v21new) // V2 and V1 dont define LORA_D1 so unsure what they need
  // Defines LORA_RST LORA_IRQ LORA_SCK LORA_MISO LORA_MOSI LORA_CS in pins_arduino.h
  #define LORA_IO1 LORA_D1
  #define LORA_RADIO_TYPE loramesher::RadioType::kSx1276
#elif defined(ARDUINO_LILYGO_T3_S3_V1_X)
  //Defines LORA_RST LORA_IRQ LORA_SCK LORA_MISO LORA_MOSI LORA_CS in pins_arduino.h
  #define LORA_IO1 LORA_DIO1
  #define LORA_RADIO_TYPE loramesher::RadioType::kSx1278
#elif defined(ARDUINO_heltec_wifi_lora_32_V3) // Note V1 and V2 used SX1276 or SX1278 chips depending on region
  // Defines: SCK MISO MOSI SS, which it uses for the SPI to the LoRa which LoRamesher picks up correctly since LORA_SCK etc not defined
 //RECOMMENDATION FROM PERPLEXITY: NSS 8, SCK 9, MOSI 10, MISO 11, RST 12, BUSY 13, and DIO1 14 was wrong
  // LM queued receive example:  Heltec WiFi LoRa:  CS=18, RST=14, IRQ=26, IO1=35 is for ARDUINO_heltec_wifi_lora_32_V2 but untested
  #define LORA_RADIO_TYPE loramesher::RadioType::kSx1262
  #define LORA_RST RST_LoRa
  #define LORA_CS SS            // GPIO8
  #define LORA_IRQ DIO0         // Bizarre swap with Busy - not sure if documentation error or what but heltecs have notoriously faulty docs
  #define LORA_IO1 BUSY_LoRa 
  #define LORA_TCXO_VOLTAGE 1.8F  
#elif !defined(LORA_CS) || !defined(LORA_IRQ) !! !defined(LORA_RADIO_TYPE) || !defined(LORA_IO1) || !defined(LORA_RST) || !defined(LORA_MOSI) || !defined(LORA_MISO)
  #error LORA parameters not defined, but defined SYSTEM_LORAMESHER_WANT
#endif

// This set of variables are specific to the Frugal-IoT mesh network, all devices should have the same.
// TODO-189 wrap each of these in a #ifndef.

#define LORA_ADDRESS 0  // Set 0 for auto-address

// TODO-189 Should be defined in platformio.ini or each wrapped in #ifndef, and replace LORA_ with SYSTEM_LORAMESHER_
// Suggest to Jamie that these (except frequency) are also defaulted in the library
#define LORA_SPREADING_FACTOR 7U
#define LORA_BANDWITH 125.0
#define LORA_CODING_RATE 7U
#define LORA_POWER 6
#define LORA_SYNC_WORD 20U //TODO-189 pick different one for FrugalIoT
#define LORA_CRC true
#define LORA_PREAMBLE_LENGTH 8U
#define LORA_FREQUENCY SYSTEM_LORAMESHER_FREQUENCY // Intentionally defined in platformio.ini as user MUST decide which to use

#define QOS_DOWNSTREAM 12 // Special value of retain when downstream message (MQTT broker->gateway->LoRa->modules)
#ifndef SYSTEM_LORAMESHER_MAXLEGITPACKET
  #define SYSTEM_LORAMESHER_MAXLEGITPACKET 256 // Seeing some big (1Mb) bad packets from sender
#endif


struct AppMessage {
    loramesher::AddressType source;
    std::vector<uint8_t> data;
};

#include "os/rtos.hpp"
loramesher::os::QueueHandle_t receive_queue = nullptr;
loramesher::os::TaskHandle_t receive_task_handle = nullptr;

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


/**
 * @brief Application task that processes incoming LoRa messages
 *
 * Receive Task — runs on its own stack, processes messages from queue
 * Blocks on the queue until a message arrives, then processes it on
 * this task's own stack — decoupled from the mesher's internal tasks.
 */
void ReceiveTask(void* /*pvParameters*/) {
  AppMessage* msg = nullptr;
  for (;;) {
      auto result = loramesher::os::RTOS::instance().ReceiveFromQueue(receive_queue, &msg, MAX_DELAY);
      if (result == loramesher::os::QueueResult::kOk && msg != nullptr) {
      Serial.print(F("LoRaMesher - app received 0x")); Serial.print(msg->data.size()); Serial.print(F(" bytes from 0x")); Serial.println(msg->source, HEX); 
      // application logic
      frugal_iot.loramesher->processReceivedPacket(msg->source, msg->data);
      delete msg; 
    }
  }
}

// ============ HELPERS =====================
bool System_LoraMesher::initialize() {
  try {
    // Create a queue - needs to be before SetDataCallback
    receive_queue = xQueueCreate(10, sizeof(AppMessage*)); //TODO-189 see if can move into LoraMesher instance
    // Create a task to receive messages
    createReceiveMessages();
    
    loramesher::RadioConfig radioConfig(LORA_RADIO_TYPE, LORA_FREQUENCY,
                            LORA_SPREADING_FACTOR, LORA_BANDWITH,
                            LORA_CODING_RATE, LORA_POWER, LORA_SYNC_WORD,
                            LORA_CRC, LORA_PREAMBLE_LENGTH);                        
    #ifdef LORA_TCXO_VOLTAGE
      radioConfig.setTcxoVoltage(LORA_TCXO_VOLTAGE);
    #endif
    loramesher::PinConfig pinConfig(
                        LORA_CS,   // NSS pin
                        LORA_RST,  // Reset pin
                        LORA_IRQ,  // DIO0 pin
                        LORA_IO1,   // DIO1 pin
                        SCK,MISO,MOSI // Because I dont trust the convolutions in the hal layer of LoRaMesher that I think are going to get the wrong values.
    );
    loramesher::LoRaMeshProtocolConfig mesh_config;
    #ifdef LORA_DUTY_CYCLE
      mesh_config.setTargetDutyCycle(LORA_DUTY_CYCLE);
    #endif
    #ifdef LORA_MIN_SLEEP_FRACTION
      mesh_config.setMinSleepFraction(LORA_MIN_SLEEP_FRACTION);
    #endif
    // Workaround form bug identified by jaimi5 23apr2026 will remove when fixed
    Serial.println("LM setting node only");
    mesh_config.setNodeRole(loramesher::NodeRole::NODE_ONLY); // Set all nodes as NODE_ONLY then gateway promotes
    // TODO figure out how to dynamically adjust whether node manager or not - see https://github.com/LoRaMesher/LoRaMesher/discussions/97#discussioncomment-16290643
    // Create LoraMesher with PingPong protocol
      mesher =
          loramesher::LoraMesher::Builder()
              .withRadioConfig(radioConfig)
              .withPinConfig(pinConfig)
              .withLoRaMeshProtocol(mesh_config)  // Use LoRaMesh protocol
              //.withAutoAddressFromHardware(true)  // Enable hardware-based addressing (default)
              .Build();
      //Set up data callback
      mesher->SetDataCallback( // TODO-189 move to own function instead of inline
        [](loramesher::AddressType source, const std::vector<uint8_t>& data) {
            auto* msg = new AppMessage{source, data};
            auto result = loramesher::os::RTOS::instance().SendToQueue(
              receive_queue, &msg, 0 /* non-blocking */);
            if (result != loramesher::os::QueueResult::kOk) {
              // Queue full — drop and free rather than block the mesher task
              delete msg;
              Serial.println(F("LoRaMesher: Receive queue full, dropping packet"));
            }
        });

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
        Serial.print("LoRaMesher node address: "); Serial.println(mesher->GetNodeAddress(), HEX);
      #endif

      #ifdef SYSTEM_LORAMESHER_DEBUG
        auto mesh_protocol = mesher->GetLoRaMeshProtocol();
        if (mesh_protocol) {
          // Set up route update callback
          mesh_protocol->SetRouteUpdateCallback(
            [](bool route_updated, loramesher::AddressType destination,
              loramesher::AddressType next_hop, uint8_t hop_count) {
                if (route_updated) {
                  Serial.print(F("Route updated - Destination: ")); Serial.print(destination, HEX); 
                  Serial.print(F(", Next hop: ")); Serial.print(next_hop, HEX); 
                  Serial.print(F(", Hops: ")); Serial.println(hop_count);
                } else {
                    Serial.print(F("Route removed for destination: ")); Serial.println(destination, HEX);
                }
            });
        }
      #endif // SYSTEM_LORAMESHER_DEBUG
      
    } catch (const std::exception& e) {
        LOG_ERROR("Exception: %s", e.what());
        return 1;
    }
    return 0;
}

// This is a workaround, for a bug in LM (April 2026) may not be used once thay bug is fixed
void System_LoraMesher::PromoteToNetworkManager() {
  auto status = mesher->GetNetworkStatus();                                                                           
  using ProtocolState = loramesher::protocols::lora_mesh::INetworkService::ProtocolState;                                 
                                                                                                                          
  bool in_network = (status.current_state == ProtocolState::NORMAL_OPERATION ||                                           
                     status.current_state == ProtocolState::NETWORK_MANAGER ||
                     status.current_state == ProtocolState::JOINING);
  if (!in_network) {
    Serial.println("LM setting NETWORK_MANAGER");
    if (!mesher->SetNodeRole(loramesher::NodeRole::NETWORK_MANAGER)) {
      Serial.println("Loramesher error setting Network Manager");
    }
  }
}
System_LoraMesher::System_LoraMesher()
: System_Base("loramesher", "LoraMesher")
{
  // TODO-189
    // Loramesher will initialize SPI on LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS (if config.spi is unset)
    // if need to override then best to set those values, else can override here on a per-board basis 
    //SPI.begin(SCK, MISO, MOSI, LORA_CS); //TODO-176 check and recheck if/why needed -
    //config.spi = &SPI;
}


// setup points radio at receiveLoRaMessage_Handle which is set to processReceivedPackets in createReceiveMessages()
// Could parameterize this, but working on assumption that this will only ever point to the MQTT handler once its all working
void System_LoraMesher::setup() {
  Serial.println(F("Loramesher setup"));
  // Nothing to read from disk so not calling readConfigFromFS 

  // A debug level is set in LoRaMesher/src/config/system_config.hpp so maybe not needed
  //esp_log_level_set("*", ESP_LOG_VERBOSE);
  //esp_log_level_set(LM_TAG, ESP_LOG_VERBOSE); // To get lots of logging from LoraMesher

  if (initialize()) { // Creates 1, maybe 2 tasks
    Serial.println("LoRaMesher initialize failed - now what !");
  }

  if (!findGatewayNode()) {
      Serial.println(F("Setup did not find a gateway node - expect after receive routing"));
  }
  checkRole(); // Probably wont do anything as MQTT probably not up yet - but will repeat in periodically
  #ifdef SYSTEM_LORAMESHER_DEBUG
    printAppData();
  #endif
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
        /* Serial.print(F("  Destination: 0x")); */ Serial.print(route.destination, HEX);
        Serial.print(F(" via: ")); Serial.print(route.next_hop, HEX);
        Serial.print(F(", Hops: "));  Serial.print(route.hop_count);
        Serial.print(F(" Q: ")); Serial.print(route.link_quality);
        Serial.print(F(" Last seen: ")); Serial.print(route.last_seen_ms);
        Serial.print(F("ms ")); Serial.print(route.is_valid ? "valid" : "invalid");
        Serial.print(F(" Capabilities: ")); Serial.print(route.capabilities, HEX);
        if (route.is_network_manager); Serial.print(F(" MANAGER"));
        Serial.println();
      }
  }
  void System_LoraMesher::printNetworkStatus() {
    auto status = mesher->GetNetworkStatus();
    Serial.print(F("Network status: State: ")); Serial.print(static_cast<int>(status.current_state));
    Serial.print(F(", Manager: ")); Serial.print(status.network_manager, HEX);
    Serial.print(F(", Slot: ")); Serial.print(status.current_slot);
    Serial.print(F(", Nodes: ")); Serial.print(status.connected_nodes);
    Serial.print(F(" Capabilities: ")); Serial.print(mesher->GetNodeCapabilities(), HEX), Serial.print(F(" ")); Serial.print(checkRoleString());
    Serial.print(F(" Gateway: ")); Serial.println(gatewayNodeAddress, HEX);
  }

#endif // SYSTEM_LORAMESHER_DEBUG
// ========= INCOMING - UP OR DOWNSTEAM =========

void System_LoraMesher::createReceiveMessages() {
  loramesher::os::RTOS::instance().CreateTask(
    ReceiveTask, "App_LoRa_Recv",
    4096,  // Stack size in bytesx
    nullptr,
    2,  // Priority — adjust relative to your other tasks
    &receive_task_handle);
}


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
  // Note packet is deleted by the callback that calls this
}

// ========= OUTGOING - UP OR DOWNSTEAM =========

// addGatewayRole is called on receiver during setup, once route tables propogate this should start seeing the gateway
bool System_LoraMesher::findGatewayNode() {
    frugal_iot.oled->debug(true, 30, "findGatewayNode"); 
  std::optional<loramesher::RouteEntry> gateway;
  frugal_iot.oled->debug(false, 50, "-1"); 
  gateway = mesher->GetClosestGateway();
  frugal_iot.oled->debug(false, 50, "--2"); 
  if (gateway.has_value()) {
  frugal_iot.oled->debug(false, 50, "---3"); 
    auto newGatewayNodeAddress = gateway.value().destination;
      frugal_iot.oled->debug(false, 50, "----4"); 

      #ifdef SYSTEM_LORAMESHER_DEBUG
        if (gatewayNodeAddress != newGatewayNodeAddress) {
          Serial.print(F("LoRaMesher - setting gateway node to ")); Serial.println(newGatewayNodeAddress, HEX);
        }
      #endif
      gatewayNodeAddress = newGatewayNodeAddress;
      return true;
  } else { // Not found so remove if still think we have one
          frugal_iot.oled->debug(false, 50, "--A"); 

    if (gatewayNodeAddress != loramesher::kBroadcastAddress) {
                frugal_iot.oled->debug(false, 50, "---B"); 
      #ifdef SYSTEM_LORAMESHER_DEBUG
        Serial.print(F("LoRaMesher - lost gateway node was ")); Serial.println(gatewayNodeAddress, HEX);
      #endif
      gatewayNodeAddress = loramesher::kBroadcastAddress;
    }
              frugal_iot.oled->debug(false, 50, "----X"); 

    return false; 
  }
}

// Do we have a LoRaMNesher connection to a gateway upstream ? 
// This is used by System_Messages to decide whether to use LoRaMesher (or MQTT)
// Side effect of setting/clearing the gateway
bool System_LoraMesher::connected() {
  return findGatewayNode();
}

// Common part to both relayDownstream and publish (which is upstream)
void System_LoraMesher::buildAndSend(uint16_t destn, const String &topic, const String &payload, bool retain, int qos) {
  // TODO it would be nice to use a structure, but LoraMesher doesnt support a structure with two unknown string lengths
  char qos_char = '0' + qos; // 0 1 2 as for MQTT incoming=12
  char retain_char = '0' + retain;
  const uint8_t* stringymessage = lprintf(SYSTEM_LORAMESHER_MAXMESSAGESIZE, "%c%c%s:%s", // Creates new buffer, exolicitly deleted below
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

bool System_LoraMesher::isGateway() {
  return (mesher->GetNodeCapabilities() & loramesher::NodeCapabilities::GATEWAY);
}
// Check if we can act as an upstream gateway and if changes update routing tables
// TODO-23 consider interaction of this with sleep modes, when come back from sleep won't have 
// TODO-23 MQTT yet, but also unclear if want a gateway role retained during sleep
LoraMesherMode System_LoraMesher::checkRole() {
  if (frugal_iot.mqtt->connected()) {
    if (!isGateway()) {
      #ifdef SYSTEM_LORAMESHER_DEBUG 
        Serial.println(F("Adding gateway role")); 
      #endif
      mesher->SetNodeCapabilities(loramesher::NodeCapabilities::GATEWAY);
      // This is a workaround, for a bug in LM (April 2026) may not be used once thay bug is fixed
      PromoteToNetworkManager(); // only does this if not already in a network
    }
    return LORAMESHER_GATEWAY;
  } else { // Not connected to MQTT
    // Wait a little while before removing gateway role
    if (isGateway() && (millis() > (lostMQTTat + SYSTEM_LORAMESHER_MQTTWAIT))) {
      #ifdef SYSTEM_LORAMESHER_DEBUG
        Serial.println(F("LoRaMesher removing gateway role as lost MQTT"));
      #endif
      mesher->SetNodeCapabilities(loramesher::NodeCapabilities::NONE);
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
