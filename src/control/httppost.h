// Deep Sleep issues: okToSleep() holds the node awake while a post is pending, for at most CONTROL_HTTPPOST_TIMEOUT_MS; a post that cannot be made by then is dropped, not queued.
/* Control_HttpPost - POST the current value of every input to a URL.
 *
 * The general "send my readings to a web server" mechanism. Control_Gsheets is one subclass; a
 * sketch posting to its own hub is another. Subclass it and override body(), and contentType() if
 * it is not a form, for whatever the server expects. The default body is form-urlencoded, one
 * field per input: col=value&col=value.
 *
 * When it posts:
 *   everyCycle false (default) - whenever an input changes, i.e. after act()
 *   everyCycle true            - once per cycle, whether or not anything changed, for a server
 *                                that wants a row per reading rather than a row per change
 * The post itself always happens from loop(), never inside act() or periodically(). act() runs
 * from a message dispatch, and periodically() runs BEFORE this cycle's readings have been delivered
 * (a sensor's output reaches a wired input through the local message queue), so both just mark the
 * post as wanted and the next loop() pass makes it, by when the inputs have caught up. With
 * everyCycle it waits up to CONTROL_HTTPPOST_SETTLE_MS for an input to change before accepting that
 * none did.
 *
 * WiFi: posts only while connected, and holds off sleep until it has - up to
 * CONTROL_HTTPPOST_TIMEOUT_MS, after which the post is dropped.
 *
 * HTTPS: a URL starting https:// gets a WiFiClientSecure with no certificate check - the same as
 * OTA without SYSTEM_OTA_USECERT. Enough for a Google Apps Script; not for a server that must be
 * verified.
 */
#ifndef CONTROL_HTTPPOST_H
#define CONTROL_HTTPPOST_H

#include "_settings.h"
#include "system/io.h"
#include "control/control.h"

#ifndef CONTROL_HTTPPOST_TIMEOUT_MS
  #define CONTROL_HTTPPOST_TIMEOUT_MS 30000 // Give up on a post, and allow sleep, if WiFi is not up by then
#endif
#ifndef CONTROL_HTTPPOST_SETTLE_MS
  #define CONTROL_HTTPPOST_SETTLE_MS 500 // everyCycle: wait this long after periodically() for an input to change
#endif
#ifndef CONTROL_HTTPPOST_HTTP_TIMEOUT_MS
  #define CONTROL_HTTPPOST_HTTP_TIMEOUT_MS 5000 // For the connection and the response - post() blocks for up to this
#endif

class Control_HttpPost : public Control {
  public:
    const String url;
    const bool everyCycle;
    Control_HttpPost(const char* id, const char* name, const String url, bool everyCycle = false);
    // Each adds an input named col and wires it; the returned pointer is for a subclass's body()
    INtext* track(const char* col, const String topicPath); // from a topic - this node's or another's
    INtext* track(const char* col, OUT* out);               // from that output
    INtext* track(const char* col, System_Base* module);    // from that module's default output
    void act() override;
    void periodically() override;
    void loop() override;
    bool okToSleep(unsigned long ms, uint16_t how) override;
    bool post(); // Send body() now - true on a 2xx or 3xx response. Blocking - loop() calls this.
  protected:
    virtual String body();             // col=value&col=value - values are sent as-is, not URL-encoded
    virtual const char* contentType(); // application/x-www-form-urlencoded
    bool changed = false;              // act() has run since the last post
    bool pending = false;              // A post is wanted - see loop()
    unsigned long pendingSince = 0;    // millis() when it became wanted
    INtext* addInput(const char* col);
};
#endif // CONTROL_HTTPPOST_H
