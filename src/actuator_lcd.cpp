/* Frugal IoT - HD44780 LCD display actuator */

#include "_settings.h"
#ifdef ACTUATOR_LCD_WANT

#include <Arduino.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include "actuator.h"
#include "actuator_lcd.h"

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
    // Print the actual GPIO numbers Wire is using so wiring can be verified.
    // If 5V power is used for the backpack, its pull-up resistors will drive
    // SDA/SCL to 5V — ESP32 GPIOs are NOT 5V-tolerant. Use 3.3V instead.
    Serial.print(F("LCD I2C SDA=gpio")); Serial.print(I2C_SDA);
    Serial.print(F(" SCL=gpio")); Serial.println(I2C_SCL);

    // TODO move this scanning to system_i2c
    // Scan I2C bus before handing control to hd44780 so we can see what's there.
    // hd44780_I2Cexp auto-detect only scans 0x20-0x27 (PCF8574/MCP23008) and
    // 0x38-0x3F (PCF8574A). If your backpack address falls outside those ranges
    // it will not be found and begin() returns RV_ENXIO.
    Serial.println(F("LCD I2C scan:"));
    bool found = false;
    delay(1000); // TOOD-XXX remove this once sure what needed
    for (uint8_t addr = 1; addr < 127; addr++) {
      I2C_WIRE.beginTransmission(addr);
      if (I2C_WIRE.endTransmission() == 0) {
        Serial.print(F("  device at 0x")); Serial.println(addr, HEX);
        found = true;
      }
    }
    if (!found) Serial.println(F("  nothing found - check wiring and that SDA/SCL are correct gpio numbers above"));
  #endif

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
