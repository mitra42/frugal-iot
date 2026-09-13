/* Sequenced, time-of-day irrigation - a port of OSPIT's irrigation.lua onto Frugal-IoT.
 *
 * Two classes:
 *   Control_Sector      one irrigation sector - a moisture probe, a target, and a valve.
 *   Control_Irrigation  the sequencer - owns the clock, the interlocks, the pump, and a list of
 *                       sectors which it runs ONE AT A TIME, in order.
 *
 * What it does, which is what OSPIT does:
 *   - once a day, at a configured local hour:minute, start a cycle
 *   - take each sector in turn; open its valve until either the soil reaches its target or the
 *     per-sector maximum time runs out; then close it and move to the next
 *   - skip a sector whose probe is not reporting (OSPIT's -127, our "nan") without touching its
 *     valve at all - see "Skipping a sector" below, this is load-bearing
 *   - abort the whole cycle if the tank runs dry or the battery interlock opens
 *   - when the last sector is done, arm for the same time tomorrow
 *
 * ---------------------------------------------------------------------------------------------
 * The main loop, and why the timer does two jobs
 *
 * A single sleep-safe timer slot is used for both halves of the job:
 *   while a cycle is running - the maximum time the CURRENT sector's valve may stay open
 *   while idle               - the absolute time the NEXT cycle should start
 *
 * That works because the two are never needed at once, and it means the whole schedule survives
 * deep sleep for free: the timer array lives in RTC_DATA_ATTR (system/power.cpp).
 *
 * The idle branch re-arms from a freshly computed nextStartTime() on every pass, so it is
 * self-correcting: System_Time::set() shifts armed timers to preserve their INTERVAL, which is
 * right for OTA and the watchdog but would slide a wall-clock appointment, and this undoes that
 * on the next cycle. See System_Power::timer_set_to().
 *
 * No RTC_DATA_ATTR state of our own is needed. `i` is reset to "idle" in setup(), which runs on
 * every boot including a deep-sleep wake - so a cycle interrupted by a sleep is abandoned rather
 * than resumed with stale state, and every valve is closed by Actuator_Digital::setup(). That is
 * the safe outcome, and allowSleep() is there to stop it happening in the first place.
 *
 * ---------------------------------------------------------------------------------------------
 * Skipping a sector
 *
 * A sector whose moisture probe publishes "nan" is skipped and its valve is never driven. On
 * OSPIT this is not a nicety - it is how the board is configured. Its third output is either
 * sector 3's valve or a USB supply, and "is probe 3 present?" is the only switch: irrigation.lua
 * skips the sector, and mp2.lua claims the same pin for USB load control, both gated on
 * `shumidity3 == -127`. Here, a sector exists because you constructed one, so the overloading is
 * gone - but the skip remains, because driving a valve you have no feedback from is worse than
 * not watering.
 *
 * ---------------------------------------------------------------------------------------------
 * Pump modes
 *
 * Set CONTROL_IRRIGATION_PUMP_MODE. Hard-coded for now; if these prove useful they become
 * classes rather than a #define.
 *   1 (default) pump runs while any valve is open and the tank has water
 *               - the tank feeds the sectors, the pump pressurises them
 *   2           pump runs while the tank is NOT full - i.e. it fills a header tank
 *   3           as 2, but only while `solar` says there is input power to spare
 */

#ifndef CONTROL_IRRIGATION_H
#define CONTROL_IRRIGATION_H

#include "control/control.h"
#include <vector>
#include <ctime>

#ifndef CONTROL_IRRIGATION_PUMP_MODE
  #define CONTROL_IRRIGATION_PUMP_MODE 1
#endif

/* How far ahead of "now" the next start must be, in seconds.
 *
 * A cycle only begins once now >= the scheduled time, so it always ends at or after it, and the
 * next occurrence naturally lands tomorrow. This margin covers the case where the clock steps
 * BACKWARDS mid-cycle, which would otherwise leave today's slot still in the future and run a
 * second cycle straight away.
 */
#ifndef CONTROL_IRRIGATION_START_MARGIN_S
  #define CONTROL_IRRIGATION_START_MARGIN_S 300
#endif

// How long to wait before looking again when the clock has not been set yet - see nextStartTime()
#ifndef CONTROL_IRRIGATION_NOTIME_RETRY_S
  #define CONTROL_IRRIGATION_NOTIME_RETRY_S 60
#endif

/* One irrigation sector: a moisture probe, the moisture it is aiming for, and a valve.
 *
 * Registered in frugal_iot.controls in its own right (so it gets dispatch(), discover() and the
 * captive portal like any other control) AND held in Control_Irrigation::sectors, which is the
 * same arrangement Control_Carousel uses for its child controls - see examples/gps/gps.ino.
 */
class Control_Sector : public Control {
  public:
    Control_Sector(const char* const id, const char* const name);
    INfloat* moisture; // Wire to a soil probe, e.g. soil1/humidity. "nan" here means "skip me"
    INfloat* target;   // Stop watering at this moisture - OSPIT's i_lvlN
    /* Set by Control_Irrigation: tank ok AND power ok AND it is this sector's turn.
     *
     * An IN rather than a plain bool so it is visible, loggable and settable: clearing it from
     * the UX or over MQTT closes the valve immediately (see act()), which makes it a usable
     * per-sector interlock as well as internal state.
     */
    INbool*  enable;
    OUTbool* valve;    // Wire to an Actuator_Digital, e.g. setPath("valve1/on")

    /* Begin watering this sector, if it should be watered at all.
     *
     * False - and the valve untouched - when there is no reading (see "Skipping a sector"), when
     * the soil is already at or above target, or when enable is false. True means the valve has
     * been opened and the caller should arm the maximum-duration timer.
     */
    bool start();
    /* True while this sector should keep watering. Called once per cycle by the sequencer, which
     * stops the sector when this returns false OR the maximum time expires.
     */
    bool step();
    void stop(); // Close the valve and drop enable. Safe to call when not running.
    /* Set `enable` and publish the change.
     *
     * IN has no set() of its own - the library's inputs are normally driven from a wire or from
     * MQTT, not from code - so this writes the value and calls send() to put it on the bus, which
     * is what makes the sector's state visible in the UX while a cycle runs.
     */
    void setEnable(bool v);
  protected:
    void act() override;
};

class Control_Irrigation : public Control {
  public:
    Control_Irrigation(const char* const id, const char* const name);
    std::vector<Control_Sector*> sectors;
    /* Create a sector, add it to this sequencer AND to frugal_iot.controls.
     *
     * Sectors run in the order they are added. Returns the sector so the sketch can wire its
     * moisture input and valve output.
     */
    Control_Sector* addSector(const char* const id, const char* const name);

    INuint16* hour;       // Local hour to start - OSPIT's i_hr
    INuint16* minute;     // Local minute to start - OSPIT has no equivalent, it starts on the hour
    INfloat*  maxminutes; // Longest any one valve may stay open - OSPIT's i_vlv_opn
    INbool*   enabled;    // Master switch - OSPIT's i_nbld
    /* Tank level in %, wired from Sensor_Tank. "nan" means no tank sensor is fitted, and is
     * deliberately NOT treated as an empty tank - that is the distinction OSPIT cannot make, and
     * on OSPIT an unplugged sender silently stops all irrigation. See sensor_tank.h.
     */
    INfloat*  tank;
    INfloat*  tankstart;  // Do not BEGIN a cycle below this level
    INfloat*  tankempty;  // ABORT a running cycle below this level
    INbool*   power;      // True when it is electrically safe to run - OSPIT's low_voltage_disconnect_state
    INbool*   solar;      // Input power available. Only consulted by pump mode 3
    OUTbool*  pump;       // Wire to the pump/load Actuator_Digital
    OUTuint16* active;    // Sector currently running, 1-based; 0 when idle

    /* False while a cycle is running, i.e. "do not sleep now".
     *
     * Nothing calls this yet - Mitra is building the sleep-management side separately, and this is
     * the hook it will use. Deep sleep mid-cycle would abandon the cycle (see the note on `i`
     * above) and, worse, leave a valve open with the watchdog and MQTT down.
     * TODO may consider holding GPIO with RTC when asleep, which would allow a cycle to span a
     * sleep rather than having to prevent one.
     */
    bool allowSleep();

    void setup() override;
    void periodically() override;
  protected:
    uint8_t t;  // Sleep-safe timer slot - see "The main loop" above. Claimed in the constructor.
    int8_t  i;  // Sector being watered; >= sectors.size() means idle. Set to idle in setup().
    bool running() { return i < (int8_t)sectors.size(); }
    /* True when a running cycle must stop right now: master switch off, power interlock open, or
     * the tank has fallen to the empty threshold. Checked every cycle AND before each sector, so
     * it aborts the sequence rather than merely the current sector.
     */
    bool blocked();
    bool tankOk();      // Tank is fitted and above the empty threshold, or no tank is fitted
    bool tankCanStart(); // As tankOk, but against the higher "enough to bother starting" threshold
    void startNext();   // Advance i to the next sector that will run; leaves it idle if none will
    time_t nextStartTime(); // Absolute epoch of the next hour:minute, always in the future
    void setPump();
    void act() override;
};

#endif // CONTROL_IRRIGATION_H
