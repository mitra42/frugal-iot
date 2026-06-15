/* Frugal IoT - HD44780 LCD display actuator
 *
 * Drives an HD44780-compatible LCD via an I2C backpack (PCF8574 expander).
 * The single input "message" accepts a string with lines separated by ASCII 10 (\n).
 * Each line is written to the corresponding row of the display; lines longer than
 * ACTUATOR_LCD_COLS are silently truncated.
 *
 * Required build flags:
 *   ACTUATOR_LCD_WANT         - enable this actuator
 *
 * Optional build flags:
 *   ACTUATOR_LCD_COLS (16)    - display column count
 *   ACTUATOR_LCD_ROWS (2)     - display row count
 *   ACTUATOR_LCD_DEBUG        - enable debug output
 */

#ifndef ACTUATOR_LCD_H
#define ACTUATOR_LCD_H

#include "_settings.h"
#ifdef ACTUATOR_LCD_WANT

#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include "actuator.h"

#ifndef ACTUATOR_LCD_COLS
  #define ACTUATOR_LCD_COLS 16
#endif
#ifndef ACTUATOR_LCD_ROWS
  #define ACTUATOR_LCD_ROWS 2
#endif

class Actuator_LCD : public Actuator {
  public:
    Actuator_LCD();
  protected:
    // No address arg → hd44780_I2Cexp scans the bus for a PCF8574/PCF8574A backpack
    // (tries 0x20–0x27 then 0x38–0x3F). To pin a specific address use e.g.:
    //   hd44780_I2Cexp lcd(0x27);
    hd44780_I2Cexp lcd;
    INtext* input;
    void setup() override;
    void act() override;
};

#endif // ACTUATOR_LCD_WANT
#endif // ACTUATOR_LCD_H
