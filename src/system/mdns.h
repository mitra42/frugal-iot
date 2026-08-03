/* Frugal-IoT - mDNS peer-to-peer messaging
 *
 * Discovers other Frugal-IoT devices on the same LAN via mDNS (_frugaliot._tcp)
 * and exchanges messages directly over HTTP, in parallel with MQTT.
 *
 * Enable with -D SYSTEM_MDNS_WANT in platformio.ini (ESP32 only).
 *
 * Configurable defines (set before including or in platformio.ini):
 *   SYSTEM_MDNS_ENDPOINT   HTTP path for incoming peer messages (default "/")
 *   SYSTEM_MDNS_PORT       Port to register and POST to (default 80)
 *   SYSTEM_MDNS_DEBUG      Enable verbose debug prints
 */
#include "_settings.h" // Before the #ifdef - that is where SYSTEM_MDNS_WANT comes from on Arduino

#ifdef SYSTEM_MDNS_WANT

#ifndef SYSTEM_MDNS_H
#define SYSTEM_MDNS_H

#ifndef ESP32
  #error "SYSTEM_MDNS_WANT is only supported on ESP32"
#endif

#include <Arduino.h>
#include <IPAddress.h>
#include <forward_list>
#include "system/base.h"

#ifndef SYSTEM_MDNS_ENDPOINT
  #define SYSTEM_MDNS_ENDPOINT "/"
#endif
#ifndef SYSTEM_MDNS_PORT
  #define SYSTEM_MDNS_PORT 80
#endif

class System_MDNS : public System_Base {
public:
  System_MDNS();
  void setup() override;
  void periodically() override;

  // True when station WiFi is connected (can reach LAN peers)
  bool connected();

  // Called by queuedMessage() — route a message to the peer whose nodeId
  // appears as the third segment of the topic path (org/project/nodeId/...).
  // Returns true if a matching peer was found and the HTTP POST succeeded.
  bool publishToPeer(const String& topicPath, const String& payload,
                     bool retain, uint8_t qos);

  // Called by attemptMdns() — push this message to every peer that has
  // registered a subscription matching this topic on this device.
  // Returns true if at least one subscriber's HTTP POST succeeded.
  bool publishToSubscribers(const String& topicPath, const String& payload);

  // Called by the STA-side POST handler when a peer registers a subscription.
  // subscriberNodeId comes from the peer's X-Frugal-Device request header (set on
  // every httpPost() this library sends) rather than a reverse IP lookup in _peers —
  // that would race against our own mDNS discovery of the subscribing peer.
  void addPeerSubscription(const String& topicPath, const String& subscriberNodeId);

  // Called by attemptMdns() — notifies any already-known mDNS peer whose nodeId
  // matches this subscription's topic prefix. Returns true if a matching peer was
  // found and the HTTP POST succeeded.
  bool notifyPeersOfSubscription(const String& topicPath);

  // HTTP POST helper — also used by System_Messages::subscribeViaMdns.
  bool httpPost(IPAddress ip, uint16_t port,
                const String& name, const String& value);

private:
  struct MdnsPeer {
    String  nodeId;
    IPAddress ip;
    uint16_t port;
  };
  struct MdnsSubscription {
    String  topicPath;
    String  subscriberNodeId; // Looked up from _peers; current IP/port fetched from there at publish time
  };

  std::forward_list<MdnsPeer>         _peers;
  std::forward_list<MdnsSubscription> _subscriptions; // what peers want FROM this device

  void onNewPeer(const String& nodeId, IPAddress ip, uint16_t port);
  void onPeerLost(const String& nodeId);
  static String urlEncode(const String& s);
};

#endif // SYSTEM_MDNS_H
#endif // SYSTEM_MDNS_WANT
