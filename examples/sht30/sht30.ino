/*
 *  Frugal IoT example - SHT30 temperature and humidity sensor
 *
 * Optional: SENSOR_SHT_ADDRESS - defaults to 0x44, (note the D1 shields default to 0x45)
 */

// defines SENSOR_SHT_ADDRESS if dont define here or in platformio.ini
#include "Frugal-IoT.h"

#ifdef ACTUATOR_OLED_WANT
  #include "control_oled_ht.h" // Custom display handler
#endif
// Change the parameters here to match your ... 
// organization, project, id, description
System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "TEMP", "Temporary testing");

void setup() {
  // Battery sensor has to come before pre_setup, all others should come after
  #ifdef SENSOR_BATTERY_PIN
    frugal_iot.configure_battery(SENSOR_BATTERY_PIN); // Adds default battery sensor can specify (pin, Scale)
  #endif
  
  // Configure power handling - type, cycle_ms, wake_ms 
  // power will be awake wake_ms then for the rest of cycle_ms be in a mode defined by type 
  // Loop= awake all the time; 
  // Light = Light Sleep; 
  // LightWiFi=Light + WiFi on (not working); 
  // Modem=Modem sleep - works but negligable power saving
  // Deep - works but slow recovery and slow response to UX so do not use except for multi minute cycles. 
  frugal_iot.configure_power(Power_Loop, 30000, 30000); // Take a reading every 30 seconds - awake all the time

  //frugal_iot.configure_power(Power_Deep, 600000, 30000); // Take a reading every 10 mins deep sleep between

  // Encapsulate setting up and starting serial and read main config also checks power ok.
  // This has to happen AFTER battery and power are setup, and before mqtt and adding sensors actuators etc.
  frugal_iot.pre_setup();
  // ---- Testing per-node enrolment against the Pi (security branch) -------------------------
  // The node has NO broker password compiled in. On its first boot it POSTs to the server's /enrol
  // with the secret below, is issued its own broker credential, and keeps that in LittleFS. The
  // secret grants only enrolment - no read, no write.
  //
  // Three things have to agree for this to work, and a mismatch shows as HTTP 403:
  //   1. the organization and project, which come from SYSTEM_FRUGAL_ORG/PROJECT in platformio.ini
  //      (set there to myfarm/lotus - the Pi knows no organization called "dev")
  //   2. the secret below, which is config.d/secrets.yaml's enrolment_myfarm on the Pi
  //   3. SYSTEM_MQTT_ENROL_URL in platformio.ini, pointing at the Pi's HTTP server on :8080
  //
  // First argument is the MQTT BROKER host; the enrolment URL is separate (see 3 above) because on
  // the Pi they are different ports and different schemes.
  // TODO move this to platformio.ini
  //frugal_iot.configure_mqtt_enrolled("frugaliot.local", "sg8m_6DPs1AQi9tjYsx-eFOAJymK7JApdHSRePluAzc");

  // To go back to the old shared-password behaviour, comment the line above and uncomment this -
  // it still works, and is what every published example still uses:
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");


  // actuator_oled and actuator_ledbuiltin added automatically on boards that have them.

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  //frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));

  // Add sensors, actuators and controls
  frugal_iot.sensors->add(new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true))
    -> powerPins(SENSOR_SHT_POWER3v3_PIN, SENSOR_SHT_POWER0_PIN);
  // If required, add a control - this is just an example
  Control_Hysteresis* cb = new Control_Hysteresis("controlhysteresis", "Control", 50, 1, 0, 100);
  frugal_iot.controls->add(cb);
  cb->outputs[0]->wireTo(frugal_iot.messages->setPath("ledbuiltin/on"));

  #ifdef ACTUATOR_OLED_WANT
    Control_Oled_HT* cos = new Control_Oled_HT("Control OLED");
    frugal_iot.controls->add(cos);
    cos->temperature->wireTo(frugal_iot.messages->path("sht/temperature"));
    cos->humidity->wireTo(frugal_iot.messages->path("sht/humidity"));
    cos->battery->wireTo(frugal_iot.messages->path("battery/battery"));
  #endif

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}

