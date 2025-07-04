/* 
 *  Frugal IoT example - Everything - for testing
 *
 * This example is for testing all the sensors, actuators and controls that are available in the Frugal IoT library.
 *
 * Its great if you make a change to check that this compiles OK.  
 */

#include "frugal_iot.h"

// Change the parameters here to match your ... 
// organization, project, id, description
System_Frugal frugal_iot("dev", "developers", "all", "Compilation test of all modules");

void setup() {
  frugal_iot.startSerial(); // Encapsulate setting up and starting serial

  // Override MQTT host, username and password if you have an "organization" other than "dev" (developers)
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");

  // Configure power handling - type, cycle_ms, wake_ms 
  // power will be awake wake_ms then for the rest of cycle_ms be in a mode defined by type 
  // Loop= awake all the time; 
  // Light = Light Sleep; 
  // LightWiFi=Light + WiFi on (not working); 
  // Modem=Modem sleep - works but negligable power saving
  // Deep - works but slow recovery and slow response to UX so do not use except for multi minute cycles. 
  frugal_iot.configure_power(Power_Loop, 30000, 30000); // Take a reading every 30 seconds - awake all the time

  // system_oled and actuator_ledbuiltin added automatically on boards that have them.

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));
  

  // SYSTEM
  // system_oled is added automatically on boards that have them.
  frugal_iot.system->add(frugal_iot.time = new System_Time());
  #ifdef SYSTEM_SD_WANT // Doesnt work on all boards, so only add if defined
    System_SD* fs_SD = new System_SD(SYSTEM_SD_PIN);
    frugal_iot.system->add(fs_SD);
  #endif
  System_LittleFS* fs_LittleFS = new System_LittleFS();
  frugal_iot.system->add(fs_LittleFS);

  #ifdef SYSTEM_LORAMESHER_WANT // Conditionally compiled as fails on non-Lora boards
    // Held in a variable as future LoRaMesher will access it directly e.g. from MQTT
    frugal_iot.system->add(frugal_iot.loramesher = new System_LoraMesher());
  #endif

  // ACTUATORS
  // actuator_ledbuiltin added automatically on boards that have them.

  frugal_iot.actuators->add(new Actuator_Digital("relay", "Relay", 3, "purple"));

  // SENSORS
  frugal_iot.sensors->add(new Sensor_Analog("salt", "Salt", 34, 4, 0, 5000, "green", true));

  // Battery only available on some boards, so only add if defined
  #if defined(ARDUINO_ESP8266_WEMOS_D1MINIPRO) || defined(ARDUINO_LOLIN_C3_PICO) || defined(LILYGOHIGROW)
    // Battery sensor on pin 33 - but note battery didnt arrive so not tested TODO test
    frugal_iot.sensors->add(new Sensor_Battery(33, 6.6F));  // TODO-57 will rarely be as simple as this
  #endif

  // Light sensor BH1750 TODO see notes in sensor_bh1750.cpp,h about I2C pin conflicts
  frugal_iot.sensors->add(new Sensor_BH1750("lux", "Lux", SENSOR_BH1750_ADDRESS, true));

  // Button on pin 35 - not sure if this is tested yet
  frugal_iot.sensors->add(new Sensor_Button("button", "Button", 35, "purple"));

  frugal_iot.sensors->add(new Sensor_DHT("DHT", 16, true));

  frugal_iot.sensors->add(new Sensor_ensaht("ensaht","ENS160 AHT21"));

  // Add a new sensor max=2000, color="pink", retain=true, DOUTpin=0, SCKpin=1, times=10, offset=0, scale=2000
  frugal_iot.sensors->add(new Sensor_LoadCell("loadcell", "Load Cell", 2000, "pink", true,
    1, 0, 10, 0, 2000)); // DOUT, SCK, times, offset, scale

  // M<S5803 is set via jumper to 76 or 77
  frugal_iot.sensors->add(new Sensor_ms5803("pressure", "Pressure", 0x77));

  Sensor_SHT* ss = new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &Wire, true); // Create SHT30 sensor
  frugal_iot.sensors->add(ss); // Add SHT30 sensor

  // Soil sensor 0%=4095 100%=0 pin=32 smooth=0 color=brown
  frugal_iot.sensors->add(new Sensor_Soil("soil", "Soil", 4095, 0, 32, 0, "brown", true));

  // CONTROLS
  Control* cb = new ControlBlinken("blinken", "Blinken", 5, 2);
  frugal_iot.controls->add(cb);
  cb->outputs[0]->wireTo(frugal_iot.mqtt->path("ledbuiltin/id")); //TODO-141 probably wont work as mqtt not setup yet

  ControlHysterisis* cb = new ControlHysterisis("control", "Control", 50, 1, 0, 100);
  frugal_iot.controls->add(cb);
  cb->outputs[0]->wireTo(frugal_iot.mqtt->path("relay/on"));

  // Must be after sensor_sht for default wiring below
  Control_Logger* clfs = new Control_LoggerFS(
    "Logger",
    #ifdef SYSTEM_SD_WANT
      fs_SD, // Use SD card for logging
    #else
      fs_LittleFS, // TODO-110 Using LittleFS for testing for now
    #endif
    "/",
    0x02, // Single log.csv with topicPath, time, value
    std::vector<IN*> {
      //INtext(const char * const sensorId, const char * const id, const char* const name, String* value, const char* const color, const bool wireable)
      new INtext("Logger", "log1", "log1", nullptr, "black", true),
      new INtext("Logger", "log2", "log2", nullptr, "black", true),
      new INtext("Logger", "log3", "log3", nullptr, "black", true)
      });
  frugal_iot.controls->add(clfs);
  // Wire the logger to the temperature sensor, it could be left blank and wired in the UX to a remote sensor
  clfs->inputs[0]->wireTo(ss->temperature); // Wired to the temperatur sensor

  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop();
}

