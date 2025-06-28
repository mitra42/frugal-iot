/* Frugal IoT - Google Sheets
 * 
 * Idea is to be able to log any MQTT topic - local or remote 
 * to a Google Sheet
 * 
 * See https://github.com/mitra42/frugal-iot/issues/136
 * 
 * Thanks to Abrar at Kopernik for the idea
*/
#include "_settings.h"

#include "control.h"
#include "control_logger.h"
#include "system_frugal.h"

Control_Logger::Control_Logger(const char* const id, const char* const name, std::vector<IN*> ii)
  : Control(id, name,  ii,   std::vector<OUT*> {})
  {}
Control_Logger::Control_Logger(const char* const id, const char* const name)
  : Control_Logger(id, name,  std::vector<IN*> {})
  {}
void Control_Logger::setup() {
  if (!frugal_iot.time) { // If time not set up
    Serial.println(F("Control_LoggerFS: Time must be set up before Control_Logger"));
  }
  Control::setup(); // Call base class setup
}