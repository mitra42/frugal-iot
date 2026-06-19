/* Frugal-IoT - mDNS peer-to-peer messaging */

#ifdef SYSTEM_MDNS_WANT

#include "_settings.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include "ESPAsyncWebServer.h"
#include "system_mdns.h"
#include "system_frugal.h" // for frugal_iot

System_MDNS::System_MDNS()
: System_Base("mdns", "mDNS") {}

void System_MDNS::setup() {
  if (!MDNS.begin(frugal_iot.nodeid.c_str())) {
    Serial.println(F("mDNS: failed to start responder"));
  } else {
    MDNS.addService("frugaliot", "tcp", SYSTEM_MDNS_PORT);
    #ifdef SYSTEM_MDNS_DEBUG
      Serial.print(F("mDNS: started as ")); Serial.print(frugal_iot.nodeid);
      Serial.print(F(".local port ")); Serial.println(SYSTEM_MDNS_PORT);
    #endif

    // Register a POST handler on the station WiFi interface (shared captive server).
    // Incoming posts carry name=<topicPath>&value=<payload> (form-encoded).
    // A special name "subscribe" registers the caller as a subscriber for value.
    frugal_iot.captive->addSTARoute(SYSTEM_MDNS_ENDPOINT, [](AsyncWebServerRequest *request) {
      const AsyncWebParameter* nameParam  = request->getParam("name",  true);
      const AsyncWebParameter* valueParam = request->getParam("value", true);
      if (nameParam && valueParam) {
        const String& name  = nameParam->value();
        const String& value = valueParam->value();
        if (name == "subscribe") {
          #ifdef SYSTEM_MDNS_DEBUG
            Serial.print(F("mDNS: incoming subscribe for ")); Serial.println(value);
          #endif
          frugal_iot.mdns->addPeerSubscription(
            value,
            request->client()->remoteIP(),
            SYSTEM_MDNS_PORT);
        } else {
          #ifdef SYSTEM_MDNS_DEBUG
            Serial.print(F("mDNS: incoming message "));
            Serial.print(name); Serial.print(F("=")); Serial.println(value);
          #endif
          frugal_iot.messages->queueIncoming(name, value);
        }
      }
      request->send(200, "text/plain", "OK");
    });
  }
}

void System_MDNS::periodically() {
  if (connected()) {
    // queryService blocks briefly while collecting mDNS responses.
    // On a quiet LAN this typically completes well under 1 s.
    int n = MDNS.queryService("frugaliot", "tcp");

    #ifdef SYSTEM_MDNS_DEBUG
      Serial.print(F("mDNS: query found ")); Serial.print(n); Serial.println(F(" peer(s)"));
    #endif

    // Build a fresh peer list from this query's results
    std::forward_list<MdnsPeer> foundPeers;
    for (int i = 0; i < n; i++) {
      String nodeId = MDNS.hostname(i);
      if (nodeId.endsWith(".local")) {
        nodeId = nodeId.substring(0, nodeId.length() - 6);
      }
      if (nodeId == frugal_iot.nodeid) {
        continue; // Skip ourselves
      }
      // MDNS.IP() is not available in all ESP32 core versions; resolve via WiFi stack instead.
      IPAddress ip;
      if (!WiFi.hostByName((nodeId + ".local").c_str(), ip)) {
        #ifdef SYSTEM_MDNS_DEBUG
          Serial.print(F("mDNS: can't resolve ")); Serial.println(nodeId);
        #endif
        continue;
      }
      uint16_t port = MDNS.port(i);
      foundPeers.push_front({nodeId, ip, port});

      bool alreadyKnown = false;
      for (const auto& p : _peers) {
        if (p.nodeId == nodeId) {
          alreadyKnown = true;
        }
      }
      if (!alreadyKnown) {
        onNewPeer(nodeId, ip, port);
      }
    }

    // Detect peers that have disappeared
    for (const auto& existing : _peers) {
      bool stillPresent = false;
      for (const auto& found : foundPeers) {
        if (found.nodeId == existing.nodeId) {
          stillPresent = true;
        }
      }
      if (!stillPresent) {
        onPeerLost(existing.nodeId);
      }
    }

    _peers = foundPeers;
  }
}

void System_MDNS::onNewPeer(const String& nodeId, IPAddress ip, uint16_t port) {
  #ifdef SYSTEM_MDNS_DEBUG
    Serial.print(F("mDNS: new peer ")); Serial.print(nodeId);
    Serial.print(F(" at ")); Serial.println(ip.toString());
  #endif
  // Send any subscriptions we hold that match this peer's topic prefix
  frugal_iot.messages->subscribeViaMdns(nodeId, ip, port);
}

void System_MDNS::onPeerLost(const String& nodeId) {
  #ifdef SYSTEM_MDNS_DEBUG
    Serial.print(F("mDNS: lost peer ")); Serial.println(nodeId);
  #endif
  // Remove subscriber-table entries that came from this peer's IP
  for (const auto& peer : _peers) {
    if (peer.nodeId == nodeId) {
      IPAddress lostIP = peer.ip;
      // Lambda function, takes lostIp from environment, then calls function with sub set to reference to each member of list
      _subscriptions.remove_if([lostIP](const MdnsSubscription& sub) {
        return sub.subscriberIP == lostIP;
      });
    }
  }
}

bool System_MDNS::connected() {
  return WiFi.status() == WL_CONNECTED;
}

bool System_MDNS::publishToPeer(const String& topicPath, const String& payload,
                                bool /*retain*/, uint8_t /*qos*/) {
  bool sent = false;
  if (connected()) {
    // Only route set/ commands (org/project/nodeId/set/...) — not sensor readings.
    String prefix = frugal_iot.org + "/" + frugal_iot.project + "/";
    if (topicPath.startsWith(prefix)) {
      String rest     = topicPath.substring(prefix.length());
      int    slashPos = rest.indexOf('/');
      if (slashPos >= 0) {
        String nodeId      = rest.substring(0, slashPos);
        String afterNodeId = rest.substring(slashPos + 1);
        if (nodeId != frugal_iot.nodeid && afterNodeId.startsWith("set/")) {
          for (const auto& peer : _peers) {
            if (peer.nodeId == nodeId) {
              #ifdef SYSTEM_MDNS_DEBUG
                Serial.print(F("mDNS: routing ")); Serial.print(topicPath);
                Serial.print(F(" to peer ")); Serial.println(nodeId);
              #endif
              sent = httpPost(peer.ip, peer.port, topicPath, payload);
            }
          }
        }
      }
    }
  }
  return sent;
}

void System_MDNS::publishToSubscribers(const String& topicPath,
                                       const String& payload) {
  if (connected()) {
    for (const auto& sub : _subscriptions) {
      if (sub.topicPath == topicPath) {
        #ifdef SYSTEM_MDNS_DEBUG
          Serial.print(F("mDNS: pushing ")); Serial.print(topicPath);
          Serial.print(F(" to subscriber ")); Serial.println(sub.subscriberIP.toString());
        #endif
        httpPost(sub.subscriberIP, sub.subscriberPort, topicPath, payload);
      }
    }
  }
}

void System_MDNS::addPeerSubscription(const String& topicPath,
                                      IPAddress subscriberIP,
                                      uint16_t subscriberPort) {
  bool duplicate = false;
  for (const auto& sub : _subscriptions) {
    if (sub.topicPath == topicPath && sub.subscriberIP == subscriberIP) {
      duplicate = true;
    }
  }
  if (!duplicate) {
    _subscriptions.push_front({topicPath, subscriberIP, subscriberPort});
    #ifdef SYSTEM_MDNS_DEBUG
      Serial.print(F("mDNS: added subscription ")); Serial.print(topicPath);
      Serial.print(F(" for ")); Serial.println(subscriberIP.toString());
    #endif
  }
}

bool System_MDNS::httpPost(IPAddress ip, uint16_t port,
                           const String& name, const String& value) {
  bool success = false;
  HTTPClient http;
  String url = String("http://") + ip.toString() + ":" + String(port)
               + SYSTEM_MDNS_ENDPOINT;
  if (http.begin(url)) {
    http.setTimeout(500); // Short timeout — LAN peers should respond quickly
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("X-Frugal-Device", frugal_iot.nodeid);
    String body = "name=" + urlEncode(name) + "&value=" + urlEncode(value);
    int code = http.POST(body);
    http.end();
    success = (code == 200);
    #ifdef SYSTEM_MDNS_DEBUG
      Serial.print(F("mDNS: POST ")); Serial.print(ip.toString());
      Serial.print(F(" ")); Serial.print(name);
      Serial.print(F("=")); Serial.print(value);
      Serial.print(F(" -> ")); Serial.println(code);
    #endif
  } else {
    #ifdef SYSTEM_MDNS_DEBUG
      Serial.print(F("mDNS: httpPost begin failed ")); Serial.println(url);
    #endif
  }
  return success;
}

// Called when a subscription is newly committed locally. Checks existing peers and
// sends an HTTP subscribe POST to any peer whose nodeId matches the topic's nodeId segment.
void System_MDNS::notifyPeersOfSubscription(const String& topicPath) {
  if (connected()) {
    String prefix = frugal_iot.org + "/" + frugal_iot.project + "/";
    if (topicPath.startsWith(prefix)) {
      String rest     = topicPath.substring(prefix.length());
      int    slashPos = rest.indexOf('/');
      if (slashPos >= 0) {
        String nodeId = rest.substring(0, slashPos);
        for (const auto& peer : _peers) {
          if (peer.nodeId == nodeId) {
            #ifdef SYSTEM_MDNS_DEBUG
              Serial.print(F("mDNS: new local subscription ")); Serial.print(topicPath);
              Serial.print(F(" -> notifying peer ")); Serial.println(nodeId);
            #endif
            httpPost(peer.ip, peer.port, "subscribe", topicPath);
          }
        }
      }
    }
  }
}

// Percent-encode a string for use in an application/x-www-form-urlencoded body.
// Topic paths contain '/' which must be encoded as %2F.
String System_MDNS::urlEncode(const String& s) {
  String encoded;
  encoded.reserve(s.length() + 16);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", (uint8_t)c);
      encoded += buf;
    }
  }
  return encoded;
}

#endif // SYSTEM_MDNS_WANT
