/* Frugal IoT - Control: format remote SHT readings for an HD44780 LCD
 *
 * Receives temperature and humidity from a remote SHT sensor via MQTT,
 * formats a two-line string, and sends it to the lcd/message set-path.
 *
 * Line 1: temperature with 1 decimal place + degree symbol + C
 * Line 2: humidity with 1 decimal place + %RH
 *
 * Note: \xDF is the degree symbol (°) in the standard HD44780 CGROM (ROM A00).
 * If your display uses a different ROM and shows a wrong character, replace
 * "\xDF" with the correct byte value for your display's character table.
 */

#include "_settings.h"
#ifdef ACTUATOR_LCD_WANT

#include "Frugal-IoT.h"
#include "control_lcd_sht.h"

Control_LCD_SHT::Control_LCD_SHT()
: temperature(new INfloat("control_lcd_sht", "temperature", "Temperature", 0.0f, 1, -40, 85, "#ff0000", true)),
  humidity(new INfloat("control_lcd_sht", "humidity", "Humidity", 0.0f, 1, 0, 100, "#0000ff", true)),
  message(new OUTtext("control_lcd_sht", "message", "Message", "", "#ffffff", true)),
  Control("control_lcd_sht", "LCD SHT", std::vector<IN*>{}, std::vector<OUT*>{})
{
  inputs.push_back(temperature);
  inputs.push_back(humidity);
  outputs.push_back(message);
}

void Control_LCD_SHT::act() {
  
  String line1 = temperature->StringValue() + "\xDF" "C";
  String line2 = humidity->StringValue() + "%RH";
  
  /*
  char buf[ACTUATOR_LCD_COLS + 1];
  dtostrf(temperature->floatValue(), 0, temperature->width, buf);
  String line1 = String(buf) + "\xDFC";
  dtostrf(humidity->floatValue(), 0, humidity->width, buf);
  String line2 = String(buf) + "%RH";
  */
  message->set(line1 + "\n" + line2);
}

#endif // ACTUATOR_LCD_WANT
