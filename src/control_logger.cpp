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

#ifdef CONTROL_LOGGER_WANT

#include "control.h"
#include "control_logger.h"

Control_Logger::Control_Logger(const char* const id, const char* const name, std::vector<IN*> ii)
  : Control(id, name,  ii,   std::vector<OUT*> {})
  {}
Control_Logger::Control_Logger(const char* const id, const char* const name)
  : Control_Logger(id, name,  std::vector<IN*> {})
  {}

#endif // CONTROL_LOGGER_WANT