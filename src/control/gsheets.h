// Deep Sleep issues: see Control_HttpPost - a post is held until WiFi is up, then the node may sleep.
/* Control_Gsheets - log inputs to a Google Sheet, through a Google Apps Script web app.
 *
 * A Control_HttpPost whose body is JSON with a timestamp: {"timestamp":"...","col":"value",...}.
 * The script URL is https, so it goes through the no-certificate-check path described in
 * httppost.h. Posts on every input change; pass everyCycle true for a row per reading instead.
 *
 * See https://github.com/mitra42/frugal-iot/issues/136 - the server-side frugal-iot-logger does
 * the same job without a script, which is why this never went further.
 *
 * Thanks to Abrar at Kopernik for the idea
 */
#ifndef CONTROL_GSHEETS_H
#define CONTROL_GSHEETS_H

#include "_settings.h"
#include "control/httppost.h"

class Control_Gsheets : public Control_HttpPost {
  public:
    Control_Gsheets(const char* name, const String url, bool everyCycle = false);
    Control_Gsheets(const char* name, const char* const url, bool everyCycle = false);
    void setup() override; // Warns if time is not set up - the timestamp needs it
  protected:
    String body() override;
    const char* contentType() override;
};
#endif // CONTROL_GSHEETS_H
