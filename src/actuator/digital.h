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
 * Any output-capable pin can be held THROUGH the sleep, RTC pad or not, provided
 * gpio_deep_sleep_hold_en() has been called - which System_Power does. What the RTC pads buy you is
 * the WAKE: an RTC hold survives the reset that ends deep sleep and stays until gpio_hold_dis(), so
 * the pin never stops being driven. A digital hold is dropped at wake, so the pin floats for the
 * couple of hundred milliseconds of boot before setup() re-drives it. For a valve or a pump that is
 * a real glitch, which is why setup() says so.
 *
 * The RTC sets, read out of the IDF's soc_caps.h and rtc_io_channel.h rather than from memory:
 *   ESP32     0, 2, 4, 12-15, 25-27, 32-39 (34-39 are input only, so no use to an actuator)
 *   ESP32-S2  GPIO0-21, so its digital-only pins 33-40 glitch at wake
 *   ESP32-C3  no RTC IO mux at all (SOC_RTCIO_PIN_COUNT is 0), so EVERY pin glitches at wake and
 *             there is nothing to choose between them - no warning is issued
 * The pin is a constructor argument, so none of this can be a compile-time error.
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