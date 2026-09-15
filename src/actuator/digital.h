// Deep Sleep issues: handled - the pin is held through the sleep unless preserveDuringSleep(false). See below.
#ifndef ACTUATOR_DIGITAL_H
#define ACTUATOR_DIGITAL_H

#include "actuator/actuator.h" // Superclass

/* Deep sleep and this pin - see Actuator::preserveDuringSleep in actuator.h for the choice.
 *
 * An ESP32 releases every GPIO when it enters deep sleep, so an output would otherwise go wherever
 * the board's pull resistors take it. The FF-OpenMPPT board is the example that prompted this: it
 * pulls its load switch UP, so a node sleeping to save power could have switched its load back ON.
 *
 * Holding is three things, and missing any one of them looks like it works until it does not:
 *   prepare()  gpio_hold_en() on the pin, before the sleep
 *   recover()  gpio_hold_dis(), then re-assert
 *   setup()    gpio_hold_dis() before pinMode - and this is the one that matters after a DEEP
 *              sleep. recover() IS reached on that path (System_Power::setup() calls it when
 *              RTC_DATA_ATTR wake_count says we woke rather than powered on), but the system group
 *              is set up LAST, so an actuator's own setup() has already run by then. A held pin
 *              silently ignores pinMode and digitalWrite, so releasing it late would be too late.
 *
 * Plus gpio_deep_sleep_hold_en() once, in System_Power, or the holds are dropped as the chip
 * powers down the digital domain.
 *
 * Not every pad can be held. The RTC-capable set differs between ESP32, S2, S3 and C3, and the
 * pin is a constructor argument rather than a constant, so this cannot be a compile-time error -
 * setup() says so on the serial port instead. Whether a non-RTC pad really holds through deep
 * sleep on a given chip is one of the things HARDWARE-QUESTIONS.md asks a tester to measure.
 */
class Actuator_Digital : public Actuator {
  public: 
    Actuator_Digital(const char * const id, const char * const name, const uint8_t pin, const char* color);
  protected:
    uint8_t pin;
    INbool* input;
    void setDefaultColor(char* color);
    void act() override;
    virtual void set(const bool v);
    virtual void setup() override;
    void prepare() override; // Hold the pin, so a deep sleep does not release it
    void recover() override; // Release and re-assert, after a light sleep
    void captiveLines(AsyncResponseStream* response);
}; // Class Actuator_Digital

#endif // ACTUATOR_DIGITAL_H