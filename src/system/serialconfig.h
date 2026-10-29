/* Frugal IoT - configuration over USB serial
 *
 * The third way into the same intake as the captive portal and MQTT: a line becomes a set/ message,
 * is queued, and the main loop dispatches it to writeConfigToFS(). So whatever the portal can set,
 * this can, with the same validation and side effects. See docs/CONFIG_PLAN.md in the client repo.
 *
 * Lines starting "~" are protocol; the debug log shares the wire and is ignored. Fields are
 * separated by TAB and percent-encoded for %, TAB, CR and LF, since an SSID or password may hold
 * spaces.
 *
 *   ~hello                    ->  ~hello <TAB> 1 <TAB> nodeid <TAB> otakey
 *   ~status                   ->  ~status <TAB> {json}
 *   ~set <TAB> twig <TAB> val ->  ~ok <TAB> twig   (queued - ~status shows whether it took)
 *   ~apply                    ->  ~ok <TAB> apply  (restarts first if a change needs it)
 *   anything else             ->  ~err <TAB> what <TAB> code     code is one of format, too_long, unknown
 *
 * Deep Sleep issues: none - a node that has just reset stays awake for SYSTEM_CAPTIVE_STARTUP_MS, and
 * a line received counts as portal activity, so a session is not cut off.
 */
#ifndef SYSTEM_SERIALCONFIG_H
#define SYSTEM_SERIALCONFIG_H

#include "system/base.h"

#ifndef SYSTEM_SERIALCONFIG_MAXLINE
  #define SYSTEM_SERIALCONFIG_MAXLINE 512 // Percent-encoding triples a character; an SSID plus password is under 100
#endif

class System_SerialConfig : public System_Base {
  public:
    System_SerialConfig();
    void setup() override;
    void loop() override;
  protected:
    String line;
    bool overflowed = false; // discarding to the next newline
    void handleLine(const String& l);
    void hello();
    void status();
    void ok(const String& what);
    void err(const String& what, const char* code);
};

#endif // SYSTEM_SERIALCONFIG_H
