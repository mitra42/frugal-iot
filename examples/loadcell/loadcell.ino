/* 
 *  Frugal IoT example - Load Cell
 * 
 */

#include "Frugal-IoT.h"

// Change the parameters here to match your ... 
// organization, project, id, description
System_Frugal frugal_iot("dev", "developers", "loadcell", "Load Cell");

// Define default pins, can override in platformio.ini
#ifndef SENSOR_LOADCELL_DOUTPIN
  #ifdef ESP8266_D1
    #define SENSOR_LOADCELL_DOUTPIN 4
  #elif defined (ARDUINO_LOLIN_S2_MINI)
    #define SENSOR_LOADCELL_DOUTPIN 33
  #endif
#endif
#ifndef SENSOR_LOADCELL_SCKPIN
  #ifdef ESP8266_D1
    #define SENSOR_LOADCELL_SCKPIN 5
  #elif defined (ARDUINO_LOLIN_S2_MINI)
    #define SENSOR_LOADCELL_SCKPIN 35
  #endif
#endif
// How many measurements to take for a reading - it will take the median of these
#ifndef SENSOR_LOADCELL_TIMES
  #define SENSOR_LOADCELL_TIMES 9
#endif
// Can put default calibration here, or override in platformio.ini - will be overridden later by calibration
#ifndef SENSOR_LOADCELL_OFFSET
  #define SENSOR_LOADCELL_OFFSET 0 // Just at first start - this will be calibrated
#endif
#ifndef SENSOR_LOADCELL_SCALE
  #define SENSOR_LOADCELL_SCALE 2000 // Just at first start - this will be calibrated
#endif


void setup() {
  frugal_iot.pre_setup(); // Encapsulate setting up and starting serial and read main config
  // Override MQTT host, username and password if you have an "organization" other than "dev" (developers)
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");

  // Configure power handling - type, cycle_ms, wake_ms 
  // power will be awake wake_ms then for the rest of cycle_ms be in a mode defined by type 
  // Loop= awake all the time; 
  // Light = Light Sleep; 
  // LightWiFi=Light + WiFi on (not working); 
  // Modem=Modem sleep - works but negligable power saving
  // Deep - works but slow recovery and slow response to UX so do not use except for multi minute cycles. 
  frugal_iot.configure_power(Power_Loop, 2000, 2000); // Take a reading every 30 seconds - awake all the time
  
  // Add local wifis here, or see instructions in the wiki for adding via the /data
  //frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));
  
  // Add a new sensor max=2000, color="pink", retain=true, DOUTpin=0, SCKpin=1, times=10, offset=0, scale=2000
  
  frugal_iot.sensors->add(new Sensor_LoadCell("loadcell", "Load Cell", 100000, "pink", true,
    SENSOR_LOADCELL_DOUTPIN, SENSOR_LOADCELL_SCKPIN, SENSOR_LOADCELL_TIMES, SENSOR_LOADCELL_OFFSET, SENSOR_LOADCELL_SCALE)); // DOUT, SCK, times, offset, scale
  
  // TODO-134 add a pair of buttons here hooked up to tare and calibrate
  // actuator_oled and actuator_ledbuiltin added automatically on boards that have them.
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}

