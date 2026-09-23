// Deep Sleep issues: none - read fresh on each wake.
#ifndef SENSOR_BH1750_H
#define SENSOR_BH1750_H

#include <Arduino.h>

// Include the superclass
#include "sensor/float.h"
#include "system/i2c.h"

// Include the library we are building on
#include <BH1750.h>

// Define a default for the sensor address if not overridden in main.cpp / bh1750.ino
// Default is 0x23 if addr is floating or tied to ground, if tied to Vcc its 0x5C
#ifndef SENSOR_BH1750_ADDRESS
  // Confirmed also for Lilygo HiGrow
    #define SENSOR_BH1750_ADDRESS 0x23  // This is the default also in the BH1750 library
#endif

// Default power control pins - can be overridden via constructor parameters
#ifndef SENSOR_BH1750_POWER0_PIN
  #define SENSOR_BH1750_POWER0_PIN 0xff
#endif
#ifndef SENSOR_BH1750_POWER3v3_PIN
  #define SENSOR_BH1750_POWER3v3_PIN 0xff
#endif

// Define the class, in terms of a parent class
class Sensor_BH1750 : public Sensor_Float {
  public:
    // Define its constructor - used in main.cpp or *.ino
    Sensor_BH1750(const char* const id, const char * const name, const uint8_t addr, TwoWire* wire, const bool retain);
  protected:
    // Define some variables
    const uint8_t addr; // I2C address
    TwoWire* wire; // The I2C interface used
    /* The shared bus, for begin() and for power - the library does the talking.
     *
     * This used to be a bare wire->begin() in setup(), which is why the note there said "potential
     * conflict with I2C on SHT. TODO-115"; going through System_I2C de-duplicates the begin() per
     * bus like every other I2C sensor, and is what gives powerPins() somewhere to land.
     */
    System_I2C interface;
    // powerPins() applies to the shared bus, not to this object - see system/interface.h
    System_Interface* powerInterface() override { return interface.bus(); }
    BH1750 lightmeter; // The data from the library

    // Define any functions we override
    void setup() override;
    float readFloat() override;
    bool validate(const float v) override;
};
#endif // SENSOR_BH1750_H