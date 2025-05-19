/* Frugal IoT - Google Sheets
 * 
 * Idea is to be able to log any MQTT topic - local or remote 
 * to a Google Sheet
 * 
 * Thanks to Abrar at Kopernik for the idea
*/
#include "_settings.h"

#ifdef CONTROL_GSHEETS_WANT

#include "control.h"
#include "control_gsheets.h"

Control_Gsheets::Control_Gsheets(const char* name)
  : Control("gsheets", name,  std::vector<IN*> {},   std::vector<OUT*> {})
  {}

IN* INxxx(IOtype t, const char* sensorId) {
  switch (t) {
    // TO-ADD-INXXX
    case BOOL:
      return new INbool(sensorId, nullptr, nullptr, false, nullptr, true);
    case UINT16:
      return new INuint16(sensorId, nullptr, nullptr, 0, 0, 0, nullptr, true);
    case FLOAT:
      return new INfloat(sensorId, nullptr, nullptr, 0, 0, 0, nullptr, true);
    case COLOR:
      return new INcolor(sensorId, nullptr, nullptr, 0, 0, 0, true);
    default:
      return nullptr;
  }
}
void Control_Gsheets::track(IOtype t, String* topicPath) {
  IN* i = INxxx(t, id); 
  i->wireTo(topicPath); // Does a subscription
  inputs.push_back(i);
}
void Control_Gsheets::track(IOtype t, const char* topicPath) {
  track(t, new String(topicPath));
}

#endif // CONTROL_GSHEETS_WANT
