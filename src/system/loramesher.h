// Deep Sleep issues: the mesh has to be re-joined on every wake and routing tables are lost, so deep sleep and a LoRa mesh do not combine well.
/* Frugal IoT - LoRaMesher wrapper 
 *
 * Based on examples at https://github.com/LoRaMesher/LoRaMesher
 * 
 * Issues: https://github.com/mitra42/frugal-iot/issues/137
 * 
 * Notes:
 *    This uses the "radio" library for LoRa rather than Sandeep Mishra's 
 * 
*/
#ifndef SYSTEM_LORAMESHER_H
#define SYSTEM_LORAMESHER_H

#include "_settings.h"
// defined in _settings.h if board has LoRa, can also define in platformio.ini if e.g. have a LoRa shield
#ifdef SYSTEM_LORAMESHER_WANT  // defined in platformio.ini

#include <atomic>
#include <forward_list>
#include <list>
#include <mutex>
#include "loramesher.hpp" // defines namespace loramesher 
#include "system/base.h"
#include "system/io.h"

class MeshSubscription {
  public:
    const String topicPath;
    const uint16_t src; // The node id subscribing 
    MeshSubscription(const String topicPath, const uint16_t src);
};


enum LoraMesherMode {
    LORAMESHER_GATEWAY,
    LORAMESHER_NODE,
    LORAMESHER_UNCONNECTED,
};

extern const __FlashStringHelper* LMstatus[7]; // Size should match .pio/libdeps/heltec_wifi_lora_32_V3/LoRaMesher/src/protocols/lora_mesh/interfaces/i_network_service.hpp


class System_LoraMesher : public System_Base {
  public:
    System_LoraMesher();
    std::unique_ptr<loramesher::LoraMesher> mesher = nullptr;
    #ifdef SYSTEM_LORAMESHER_DEBUG
      void printRouteTable();
      void printNetworkStatus();
    #endif
    // == INCOMING (up or downstream)
    // public only because called from the callback - do not use externally
    #ifdef SYSTEM_LORAMESHER_DEBUG
      String lastTopicPath = String(); // Used by printAppData - written on App_LoRa_Recv, unlocked
      String lastPayload = String(); // Used by printAppData - written on App_LoRa_Recv, unlocked
    #endif
    void processReceivedPacket(loramesher::AddressType source, const std::vector<uint8_t>& data);
    // May this address speak for this topic? Checks the organization/project prefix, and binds the
    // node id to the address that first claimed it. Reports and returns false when it may not.
    bool relayPermitted(loramesher::AddressType source, const String& topicPath, const String& payload = String()); 
    // == OUTGOING (up or downstream)
    bool connected();
    // Match mqtt.client profile
    bool publish(const String &topicPath, const String &payload, const bool retain, const int qos);
    // == UPSTREAM 
    LoraMesherMode checkRole();
    const __FlashStringHelper* checkRoleString();
    // == DOWNSTREAM 

  protected:
    unsigned long lostMQTTat;
    bool initialize(); // Try and create the LoraMesher instance
    void setup() override;
    void loop() override;
    void periodically() override;
    bool isGateway(); 
    bool okToSleep(unsigned long ms, uint16_t how) override;
    void createReceiveMessages();
    void PromoteToNetworkManager();
    bool in_network();
    // == INCOMING (up or downstream)
    uint16_t rcvdPacketCounter = 0;
    // Guards meshSubscriptions and resubscribePending, which App_LoRa_Recv writes and the main task reads.
    // Never held while calling into frugal_iot.messages, which has its own lock taken in the other order by dispatch().
    std::mutex meshLock;
    std::forward_list<MeshSubscription> meshSubscriptions;
    /*
     * Which node id each LoRaMesher address has claimed, remembered from the first topic it sent.
     *
     * A gateway republishes what it hears onto MQTT under its OWN broker account, so the broker
     * cannot tell a relayed reading from an invented one - it has to trust the gateway. That makes
     * the gateway the only place a relayed topic can be checked at all, which is why these checks
     * are here and not in an ACL. See SECURITY-REVIEW.md S7.
     *
     * Trust on first sight: the first address to claim a node id keeps it, so a later transmitter
     * cannot take over an existing node's topics. It does not stop one that is first, which is
     * accepted residual risk - a rogue node inside the organization is not the threat this is for.
     */
    struct MeshIdentity { uint16_t address; String nodeid; };
    std::forward_list<MeshIdentity> meshIdentities; // Only touched on App_LoRa_Recv, so not locked
    bool hasMeshSubscription(uint16_t src);
    /*
     * Getting subscriptions back to a gateway that has lost them.
     *
     * A gateway holds its nodes' subscriptions only in RAM (meshSubscriptions), so they are gone
     * when it restarts. Two triggers, for the two ways that can go:
     * - The node notices: it goes from not being a node to being one, or its gateway changes (see
     *   periodically). It resends its subscriptions.
     * - The node does not notice, because the gateway was back before its route timed out. The
     *   gateway notices instead: a discovery packet arrives from a node it holds no subscriptions
     *   for (every node subscribes to at least its own set/#), and it sends that node "resubscribe".
     * Both packet handlers run on the App_LoRa_Recv task, so they only record what is wanted here,
     * and loop() acts on it on the main task.
     */
    LoraMesherMode lastRole = LORAMESHER_UNCONNECTED;
    uint16_t lastGatewayNodeAddress = loramesher::kBroadcastAddress;
    std::atomic<bool> resubscribeRequested{false}; // Node: our gateway has asked
    std::forward_list<uint16_t> resubscribePending; // Gateway: nodes to ask - guarded by meshLock
    // == OUTGOING (up or downstream)
    uint16_t sentPacketCounter = 0; 
    bool findGatewayNode();
    bool buildAndSend(uint16_t destn, const String &topic, const String &payload, bool retain, int qos);
    // == UPSTREAM 
    uint16_t gatewayNodeAddress = loramesher::kBroadcastAddress;
    // == DOWNSTREAM 
    bool relayDownstream(uint16_t destn, const String &topic, const String &payload);
    // Waiting to go down to nodes, so a send LoRaMesher refuses (TX queue full, no route yet) is
    // retried from loop() rather than lost. Main task only, so not locked.
    struct DownstreamMessage { uint16_t destn; String topicPath; String payload; };
    std::list<DownstreamMessage> downstream;
    void queueDownstream(uint16_t destn, const String &topicPath, const String &payload);
    void sendDownstreamQueued();
    void dispatch(System_Message &msg) override;
 
    #ifdef TODO_189_NOT_NEEDED
      LoraMesher::LoraMesherConfig config = LoraMesher::LoraMesherConfig();
    #endif
};

#endif // SYSTEM_LORAMESHER_WANT
#endif // SYSTEM_LORAMESHER_H

