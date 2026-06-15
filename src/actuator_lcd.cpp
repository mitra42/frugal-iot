/* Frugal IoT - HD44780 LCD display actuator */

#include "_settings.h"
#ifdef ACTUATOR_LCD_WANT

#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include "actuator_lcd.h"
#ifdef ACTUATOR_LCD_DEBUG
  #include "system_i2c.h"
#endif


Actuator_LCD::Actuator_LCD()
: Actuator("lcd", "LCD"),
  input(new INtext("lcd", "message", "Message", "", "white", true))
{
  inputs.push_back(input);
}

void Actuator_LCD::setup() {
  Actuator::setup();
  // Note: hd44780_I2Cexp hardcodes Wire throughout its implementation; it calls
  // Wire.begin() itself inside begin(). Our call here ensures the correct SDA/SCL
  // pins are set first on boards where they differ from the Arduino defaults.
  I2C_WIRE.begin(I2C_SDA, I2C_SCL);

  #ifdef ACTUATOR_LCD_DEBUG
    System_I2C i2c(static_cast<std::uint8_t>(0), &I2C_WIRE); // Allow scanning
    i2c.scan();
  #endif

  // This is where the I2C bus scan runs (when lcd was constructed without an address).
  int status = lcd.begin(ACTUATOR_LCD_COLS, ACTUATOR_LCD_ROWS);
  if (status) {
    // .pio/libdeps/*/hd44780/hd44780.h
    Serial.print("LCD setup failed, status="); Serial.println(status);
    setupFailed();
    return;
  }
  lcd.clear();
  #ifdef ACTUATOR_LCD_DEBUG
    lcd.setCursor(0, 0);
    lcd.print("Starting");
    Serial.print("Found LCD at "); Serial.print(lcd.getProp(hd44780_I2Cexp::Prop_addr));
  #endif
}

void Actuator_LCD::act() {
  lcd.clear();
  String remaining = input->value;
  for (uint8_t row = 0; row < ACTUATOR_LCD_ROWS; row++) {
    int nl = remaining.indexOf('\n');
    String line = (nl >= 0) ? remaining.substring(0, nl) : remaining;
    if (line.length() > 0) {
      lcd.setCursor(0, row);
      //Serial.print("XXX LCD:"),Serial.println(line.substring(0, ACTUATOR_LCD_COLS));
      lcd.print(line.substring(0, ACTUATOR_LCD_COLS));
    }
    if (nl < 0) break;
    remaining = remaining.substring(nl + 1);
  }
  #ifdef ACTUATOR_LCD_DEBUG
    Serial.print(F("\nLCD display: ")); Serial.println(input->value);
  #endif
}
#endif // ACTUATOR_LCD_WANT
