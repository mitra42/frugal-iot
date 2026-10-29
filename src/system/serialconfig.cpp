#include "system/serialconfig.h"
#include "system/frugal.h" // for frugal_iot

System_SerialConfig::System_SerialConfig() : System_Base("serialconfig", "SerialConfig") {}

static String encode(const String& raw) {
  String r;
  for (unsigned int i = 0; i < raw.length(); i++) {
    const char c = raw.charAt(i);
    if ((c == '%') || ((uint8_t)c < 0x20)) {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", (uint8_t)c);
      r += buf;
    } else {
      r += c;
    }
  }
  return r;
}

static int hexval(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
// false on a malformed %-escape, so a half-received value is never applied
static bool decode(const String& enc, String& out) {
  out = "";
  for (unsigned int i = 0; i < enc.length(); i++) {
    const char c = enc.charAt(i);
    if (c == '%') {
      if (i + 2 >= enc.length()) return false;
      const int h = hexval(enc.charAt(i + 1)), l = hexval(enc.charAt(i + 2));
      if (h < 0 || l < 0) return false;
      out += (char)(h * 16 + l);
      i += 2;
    } else {
      out += c;
    }
  }
  return true;
}

// A JSON string literal, for ~status
static void json(Print& out, const String& s) {
  out.print('"');
  for (unsigned int i = 0; i < s.length(); i++) {
    const char c = s.charAt(i);
    if ((c == '"') || (c == '\\')) { out.print('\\'); out.print(c); }
    else if ((uint8_t)c < 0x20) { out.printf("\\u%04x", (uint8_t)c); }
    else { out.print(c); }
  }
  out.print('"');
}

void System_SerialConfig::setup() {
  line.reserve(64);
  hello();
}

void System_SerialConfig::hello() {
  Serial.print(F("~hello\t1\t")); Serial.print(frugal_iot.nodeid); Serial.print('\t');
  #if defined(SYSTEM_OTA_PREFIX) && defined(SYSTEM_OTA_SUFFIX)
    Serial.println(SYSTEM_OTA_PREFIX "_" SYSTEM_OTA_SUFFIX);
  #else
    Serial.println('-');
  #endif
}

void System_SerialConfig::status() {
  Serial.print(F("~status\t{\"org\":")); json(Serial, frugal_iot.org);
  Serial.print(F(",\"project\":")); json(Serial, frugal_iot.project);
  Serial.print(F(",\"name\":")); json(Serial, frugal_iot.name);
  Serial.print(F(",\"wifi\":")); json(Serial, WiFi.SSID());
  Serial.print(F(",\"connected\":")); Serial.print(frugal_iot.wifi->connected() ? F("true") : F("false"));
  Serial.print(F(",\"mqtt\":")); Serial.print((frugal_iot.mqtt && frugal_iot.mqtt->connected()) ? F("true") : F("false"));
  Serial.print(F(",\"enrolled\":")); Serial.print((frugal_iot.mqtt && frugal_iot.mqtt->enrolled()) ? F("true") : F("false"));
  Serial.print(F(",\"restart\":")); Serial.print(frugal_iot.restartPending ? F("true") : F("false"));
  Serial.println('}');
}

void System_SerialConfig::ok(const String& what) { Serial.print(F("~ok\t")); Serial.println(encode(what)); }
void System_SerialConfig::err(const String& what, const char* code) {
  Serial.print(F("~err\t")); Serial.print(encode(what)); Serial.print('\t'); Serial.println(code);
}

void System_SerialConfig::handleLine(const String& l) {
  if (!l.startsWith("~")) return; // debug output, or noise
  if (frugal_iot.captive) { frugal_iot.captive->lastActivityMs = millis(); }
  const int t1 = l.indexOf('\t');
  const String cmd = (t1 < 0) ? l.substring(1) : l.substring(1, t1);
  if (cmd == "hello") {
    hello();
  } else if (cmd == "status") {
    status();
  } else if (cmd == "apply") {
    ok("apply");
    if (frugal_iot.restartPending) {
      Serial.flush();
      delay(100);
      ESP.restart();
    }
  } else if (cmd == "set") {
    const int t2 = (t1 < 0) ? -1 : l.indexOf('\t', t1 + 1);
    String twig, value;
    if (t2 < 0 || !decode(l.substring(t1 + 1, t2), twig) || !decode(l.substring(t2 + 1), value) || !twig.length()) {
      err("set", "format");
      return;
    }
    frugal_iot.messages->queueFromSerial("set/" + twig, value);
    ok(twig);
  } else {
    err(cmd, "unknown");
  }
}

void System_SerialConfig::loop() {
  while (Serial.available()) {
    const char c = (char)Serial.read();
    if (c == '\n') {
      if (overflowed) {
        err("line", "too_long");
        overflowed = false;
      } else {
        if (line.endsWith("\r")) { line.remove(line.length() - 1); }
        handleLine(line);
      }
      line = "";
    } else if (!overflowed) {
      if (line.length() >= SYSTEM_SERIALCONFIG_MAXLINE) {
        overflowed = true;
        line = "";
      } else {
        line += c;
      }
    }
  }
}
