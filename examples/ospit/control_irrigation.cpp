/* See control_irrigation.h for what this does, how the single timer serves both halves of the
 * job, and why a sector with no reading is skipped rather than watered.
 */

#include "control_irrigation.h"
#include "misc.h" // for changed()
#include "Frugal-IoT.h"
#include <cmath> // for NAN

// ================= Control_Sector =================================================

Control_Sector::Control_Sector(const char* const id, const char* const name)
: Control(id, name, std::vector<IN*>{}, std::vector<OUT*>{}),
  // Starts as "no reading", so a sector whose moisture input is never wired is skipped rather
  // than watered on the strength of a default of zero.
  moisture(new INfloat(id, "moisture", "Moisture", NAN, 1,
    DEFAULT_sector_moisture_min, DEFAULT_sector_moisture_max,
    DEFAULT_sector_moisture_min, DEFAULT_sector_moisture_max, DEFAULT_sector_moisture_color, true)),
  target(new INfloat(id, "target", "Target", 80, 1,
    DEFAULT_sector_target_min, DEFAULT_sector_target_max,
    DEFAULT_sector_target_min, DEFAULT_sector_target_max, DEFAULT_sector_target_color, true)),
  enable(new INbool(id, "enable", "Enable", false, DEFAULT_sector_enable_color, false)),
  valve(new OUTbool(id, "valve", "Valve", false, DEFAULT_sector_valve_color, true))
{
  inputs.push_back(moisture);
  inputs.push_back(target);
  inputs.push_back(enable);
  outputs.push_back(valve);
}

void Control_Sector::setEnable(const bool v) {
  if (changed(v, enable->value)) {
    enable->value = v;
    enable->send();
  }
}

/* Inputs changed.
 *
 * `enable` is the interlock: dropping it closes the valve at once, whoever dropped it - the
 * sequencer moving on, or a person clearing it from the portal. Raising it does NOT open the
 * valve; only start() does, so the sequencer stays in charge of when watering begins.
 *
 * Nothing here reacts to `moisture` reaching `target`. That is deliberate - the decision to stop
 * belongs to the sequencer, which also has to arm the next sector, and having two places able to
 * close the valve would make "why did it stop?" much harder to answer.
 */
void Control_Sector::act() {
  if (!enable->value && valve->value) {
    valve->set(false);
  }
}

bool Control_Sector::start() {
  bool started = false;
  // isValid() false means the probe published "nan" - no reading at all. Skip without touching
  // the valve; see "Skipping a sector" in the header.
  if (enable->value && moisture->isValid() && (moisture->floatValue() < target->floatValue())) {
    valve->set(true);
    started = true;
  }
  return started;
}

bool Control_Sector::step() {
  bool keepGoing;
  if (!moisture->isValid()) {
    /* The reading disappeared part way through watering.
     *
     * Keep the valve open and let the maximum-duration timer end the sector, rather than closing
     * up immediately. A probe that misses a poll or two on a shared RS485 bus is a great deal more
     * common than a probe that has really gone, and stopping on every dropped reading would
     * under-water the sector every time. The timeout still bounds how much water this can cost.
     *
     * TODO revisit once there is field experience. The alternative - stop and advance - is safer
     * if probes turn out to fail outright more often than they glitch, and this is the one line
     * to change. Note start() takes the opposite view for the same condition, on purpose: opening
     * a valve with no feedback at all is a worse bet than continuing one already open.
     */
    keepGoing = true;
  } else {
    keepGoing = moisture->floatValue() < target->floatValue();
  }
  return keepGoing;
}

void Control_Sector::stop() {
  valve->set(false);
  setEnable(false);
}

// ================= Control_Irrigation =============================================

Control_Irrigation::Control_Irrigation(const char* const id, const char* const name)
: Control(id, name, std::vector<IN*>{}, std::vector<OUT*>{}),
  // One slot, claimed once, for the lifetime of the device - see CLAUDE.md "Using a sleep-safe
  // timer in a component". It carries the per-sector maximum while a cycle runs and the next
  // start time while idle.
  t(frugal_iot.powercontroller->timer_next()),
  i(0), // Replaced by setup() with sectors.size(), i.e. idle, once the sectors are known
  hour(new INuint16(id, "hour", "Start hour", 3,
    DEFAULT_irrigation_hour_min, DEFAULT_irrigation_hour_max,
    DEFAULT_irrigation_hour_min, DEFAULT_irrigation_hour_max, DEFAULT_irrigation_hour_color, false)),
  minute(new INuint16(id, "minute", "Start minute", 0,
    DEFAULT_irrigation_minute_min, DEFAULT_irrigation_minute_max,
    DEFAULT_irrigation_minute_min, DEFAULT_irrigation_minute_max, DEFAULT_irrigation_minute_color, false)),
  maxminutes(new INfloat(id, "maxminutes", "Max minutes per sector", 5, 1,
    DEFAULT_irrigation_maxminutes_min, DEFAULT_irrigation_maxminutes_max,
    DEFAULT_irrigation_maxminutes_min, DEFAULT_irrigation_maxminutes_max,
    DEFAULT_irrigation_maxminutes_color, false)),
  // Defaults to OFF, as OSPIT's i_nbld does. Something that opens water valves unattended should
  // not start doing so merely because it was flashed - see the note in ospit.ino on turning it on.
  enabled(new INbool(id, "enabled", "Enabled", false, DEFAULT_irrigation_enabled_color, false)),
  tank(new INfloat(id, "tank", "Tank level", NAN, 1,
    DEFAULT_irrigation_tank_min, DEFAULT_irrigation_tank_max,
    DEFAULT_irrigation_tank_min, DEFAULT_irrigation_tank_max, DEFAULT_irrigation_tank_color, true)),
  tankstart(new INfloat(id, "tankstart", "Tank level to start", 25, 1,
    DEFAULT_irrigation_tankstart_min, DEFAULT_irrigation_tankstart_max,
    DEFAULT_irrigation_tankstart_min, DEFAULT_irrigation_tankstart_max,
    DEFAULT_irrigation_tankstart_color, false)),
  tankempty(new INfloat(id, "tankempty", "Tank level to stop", 5, 1,
    DEFAULT_irrigation_tankempty_min, DEFAULT_irrigation_tankempty_max,
    DEFAULT_irrigation_tankempty_min, DEFAULT_irrigation_tankempty_max,
    DEFAULT_irrigation_tankempty_color, false)),
  // Both interlocks default TRUE, so a node with nothing wired to them still irrigates. Wiring
  // one in can only ever stop irrigation, never enable it, which is the right way round.
  power(new INbool(id, "power", "Power ok", true, DEFAULT_irrigation_power_color, true)),
  solar(new INbool(id, "solar", "Input power", true, DEFAULT_irrigation_solar_color, true)),
  pump(new OUTbool(id, "pump", "Pump", false, DEFAULT_irrigation_pump_color, true)),
  active(new OUTuint16(id, "active", "Active sector", 0,
    DEFAULT_irrigation_active_min, DEFAULT_irrigation_active_max, DEFAULT_irrigation_active_color, false))
{
  inputs.push_back(hour);
  inputs.push_back(minute);
  inputs.push_back(maxminutes);
  inputs.push_back(enabled);
  inputs.push_back(tank);
  inputs.push_back(tankstart);
  inputs.push_back(tankempty);
  inputs.push_back(power);
  inputs.push_back(solar);
  outputs.push_back(pump);
  outputs.push_back(active);
}

Control_Sector* Control_Irrigation::addSector(const char* const id, const char* const name) {
  Control_Sector* s = new Control_Sector(id, name);
  sectors.push_back(s);
  // Also a control in its own right, so it gets dispatch(), discover() and a captive-portal
  // section like anything else. Same arrangement as Control_Carousel's children - see gps.ino.
  frugal_iot.controls->add(s);
  return s;
}

void Control_Irrigation::setup() {
  // Idle. This runs on every boot, a deep-sleep wake included, so a cycle interrupted by a sleep
  // is abandoned rather than resumed from stale state - and every valve has just been driven low
  // by Actuator_Digital::setup(). See the header for why that is the outcome we want.
  i = (int8_t)sectors.size();
  Control::setup();
}

bool Control_Irrigation::allowSleep() {
  return !running();
}

// No tank sensor fitted ("nan") never blocks - that is the distinction OSPIT cannot make, where an
// unplugged sender reads as an empty tank and silently stops all irrigation. See sensor_tank.h.
bool Control_Irrigation::tankOk() {
  return !tank->isValid() || (tank->floatValue() > tankempty->floatValue());
}

bool Control_Irrigation::tankCanStart() {
  return !tank->isValid() || (tank->floatValue() >= tankstart->floatValue());
}

bool Control_Irrigation::blocked() {
  return !enabled->value || !power->value || !tankOk();
}

void Control_Irrigation::startNext() {
  while (++i < (int8_t)sectors.size()) {
    if (blocked()) {
      // An interlock opened part way through the sequence. Abandon the whole cycle, not just this
      // sector - which is what OSPIT's "Irrigation Emergency Stop" does.
      i = (int8_t)sectors.size();
    } else {
      sectors[i]->setEnable(true);
      if (sectors[i]->start()) {
        frugal_iot.powercontroller->timer_set(t, (uint32_t)(maxminutes->floatValue() * 60.0f));
        break;
      }
      sectors[i]->setEnable(false); // Did not start - do not leave it looking armed
    }
  }
}

time_t Control_Irrigation::nextStartTime() {
  time_t next;
  if (!frugal_iot.time || !frugal_iot.time->isTimeSet()) {
    /* No clock yet - NTP has not answered and nobody has set it from the portal.
     *
     * Computing a start time from an unset clock would produce a 1970 date, which is long past,
     * so the timer would fire at once and irrigate at boot. Come back and look again shortly
     * instead. sleepSafeSecs() is the same clock the timer compares against, so this is right
     * whatever epoch the device currently believes in.
     */
    next = (time_t)(frugal_iot.powercontroller->sleepSafeSecs() + CONTROL_IRRIGATION_NOTIME_RETRY_S);
  } else {
    const time_t now = frugal_iot.time->now();
    struct tm lt;
    localtime_r(&now, &lt);
    lt.tm_hour = (int)hour->value;
    lt.tm_min = (int)minute->value;
    lt.tm_sec = 0;
    lt.tm_isdst = -1; // Let mktime decide, rather than inheriting today's flag
    next = mktime(&lt);
    if (next <= (now + CONTROL_IRRIGATION_START_MARGIN_S)) {
      // Today's slot has been and gone - the normal case just after a cycle finishes. Increment
      // the day and let mktime re-normalise, rather than adding 86400: a day that is not 24 hours
      // long still has to land on the same wall-clock time.
      lt.tm_mday += 1;
      lt.tm_hour = (int)hour->value;
      lt.tm_min = (int)minute->value;
      lt.tm_sec = 0;
      lt.tm_isdst = -1;
      next = mktime(&lt);
    }
  }
  return next;
}

void Control_Irrigation::setPump() {
  bool on;
  #if CONTROL_IRRIGATION_PUMP_MODE == 1
    // Tank feeds the sectors; the pump pressurises them while a valve is open.
    on = running() && tankOk();
  #elif CONTROL_IRRIGATION_PUMP_MODE == 2
    // Pump fills a header tank - run whenever it is below the "enough to start" level. Never run
    // when no tank sensor is fitted: with no level to work from, running a pump is the unsafe guess.
    on = tank->isValid() && !tankCanStart();
  #elif CONTROL_IRRIGATION_PUMP_MODE == 3
    // As mode 2, but only while there is input power to spare, e.g. wired from a hysteresis
    // control on the solar input voltage.
    on = tank->isValid() && !tankCanStart() && solar->value;
  #else
    #error CONTROL_IRRIGATION_PUMP_MODE must be 1, 2 or 3 - see control_irrigation.h
  #endif
  pump->set(on);
}

/* An input changed.
 *
 * Acting here as well as in periodically() is what makes the interlocks quick: a battery that has
 * just dropped below its threshold closes the valve now rather than up to one wake-cycle later.
 * The abort path is the same one startNext() takes, so there is only one behaviour to reason about.
 */
void Control_Irrigation::act() {
  if (running() && blocked()) {
    sectors[i]->stop();
    i = (int8_t)sectors.size();
    active->set(0);
  }
  setPump();
}

void Control_Irrigation::periodically() {
  System_Power* p = frugal_iot.powercontroller;
  if (running()) {
    // Three ways a sector's turn ends: its time ran out, an interlock opened, or the soil reached
    // target. stop() closes the valve in all three; startNext() then finds the next sector that
    // will actually run, or leaves us idle.
    if (p->timer_expired(t) || blocked() || !sectors[i]->step()) {
      sectors[i]->stop();
      startNext();
    }
  } else if (p->timer_expired(t) && !blocked() && tankCanStart()) {
    i = -1; // startNext() pre-increments, so this starts the search at sector 0
    startNext();
  }
  if (!running()) {
    /* Re-arm for the next run, every pass while idle.
     *
     * Doing it unconditionally is what makes this self-correcting: System_Time::set() shifts armed
     * timers to preserve their interval, which would slide a wall-clock appointment, and this
     * overwrites that with a freshly computed absolute time. Cheap, and no state to get wrong.
     *
     * TODO this writes off the whole day if an interlock happens to be open at the scheduled
     * moment - a tank still refilling at 03:00, or a momentary battery dip - because the next
     * start computed here is tomorrow. OSPIT instead retries for the rest of the scheduled hour:
     * its emergency stop clears the active valve but leaves irrigation_done false, so the 6-second
     * loop re-triggers. Worth adding a retry window here if that turns out to matter in practice.
     */
    p->timer_set_to(t, (uint32_t)nextStartTime());
  }
  active->set(running() ? (uint16_t)(i + 1) : 0);
  setPump();
}
