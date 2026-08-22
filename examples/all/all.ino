/* 
 *  Frugal IoT example - All modules - this is just for compilation testing
 * 
 */

#include "Frugal-IoT.h"
// Change the parameters here to match your ... 
// organization, project, id, description
System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "all", "All sensors - just for testing"); 

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
  frugal_iot.configure_power(Power_Loop, 30000, 30000); // Take a reading every 30 seconds - awake all the time

  // actuator_oled and actuator_ledbuiltin added automatically on boards that have them.

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));

  #ifdef SYSTEM_LORA_WANT
    //esp_log_level_set(LM_TAG, ESP_LOG_INFO);     // enable INFO logs from LoraMesher - but doesnt seem to work
    frugal_iot.loramesher = new System_LoraMesher(); // Held in a variable as future LoRaMesher will access it directly e.g. from MQTT
    frugal_iot.system->add(frugal_iot.loramesher);
  #endif

  // Add sensors, actuators and controls

  // Battery sensor on pin 33
  frugal_iot.sensors->add(new Sensor_Battery(33, 2));

  // Light sensor BH1750 TODO see notes in sensor_bh1750.cpp,h about I2C pin conflicts
  frugal_iot.sensors->add(new Sensor_BH1750("lux", "Lux", SENSOR_BH1750_ADDRESS, &I2C_WIRE, true));
    
  // Button on pin 35 - not sure if this is tested yet
  frugal_iot.sensors->add(new Sensor_Button("button", "Button", 35, "purple"));

  #ifndef SENSOR_DHT_PIN
    #ifdef GPIO_NUM_16
      #define SENSOR_DHT_PIN GPIO_NUM_16 // Lilygo HiGrow
    #else 
      #define SENSOR_DHT_PIN 16 // For esp8266 its often D0 - could really be on any digital pin
    #endif
  #endif
  frugal_iot.sensors->add(new Sensor_DHT("DHT", SENSOR_DHT_PIN, true));
  
  frugal_iot.sensors->add(new Sensor_DS18B20("ds18b20", "Soil Temperature", 5, 0, true));

  // The ENS160+AHT21 board is two chips, and so two sensors. The ENS160 needs an ambient
  // temperature and humidity for its compensation, and wires itself to the AHT21's outputs
  // unless told otherwise - so nothing extra is needed here.
  frugal_iot.sensors->add(new Sensor_AHT21("AHT21"));
  frugal_iot.sensors->add(new Sensor_ENS160("ENS160"));

  // Add a new loadcell sensor max=2000, color="pink", retain=true, DOUTpin=0, SCKpin=1, times=10, offset=0, scale=2000
  // Define default pins, can override in platformio.ini
  #ifndef SENSOR_LOADCELL_DOUTPIN
    #define SENSOR_LOADCELL_DOUTPIN 4
  #endif
  #ifndef SENSOR_LOADCELL_SCKPIN
    #define SENSOR_LOADCELL_SCKPIN 5
  #endif
  // How many measurements to take for a reading - it will take the median of these
  #ifndef SENSOR_LOADCELL_TIMES
    #define SENSOR_LOADCELL_TIMES 9
  #endif
  // Can put default calibration here, or override in platformio.ini - will be overridden later by calibration
  #ifndef SENSOR_LOADCELL_OFFSET
    #define SENSOR_LOADCELL_OFFSET 0
  #endif
  #ifndef SENSOR_LOADCELL_SCALE
    #define SENSOR_LOADCELL_SCALE 2000
  #endif
  frugal_iot.sensors->add(new Sensor_LoadCell("loadcell", "Load Cell", DEFAULT_loadcell_loadcell_max, DEFAULT_loadcell_loadcell_color, true,
    SENSOR_LOADCELL_DOUTPIN, SENSOR_LOADCELL_SCKPIN, SENSOR_LOADCELL_TIMES, SENSOR_LOADCELL_OFFSET, SENSOR_LOADCELL_SCALE)); // DOUT, SCK, times, offset, scale

  // MS5803 is set via jumper to 76 or 77
  frugal_iot.sensors->add(new Sensor_ms5803("ms5803", "MS5803", 0x77));

  // Bosch pressure/humidity family. All three default to 0x76, which is a real address clash on
  // hardware - fine here, since nothing in this sketch is ever meant to be run.
  frugal_iot.sensors->add(new Sensor_BMP280("BMP280"));
  frugal_iot.sensors->add(new Sensor_BME280("BME280"));
  frugal_iot.sensors->add(new Sensor_BME680("BME680"));

  // Current/voltage/power monitor. Address, shunt and max current all have defaults.
  frugal_iot.sensors->add(new Sensor_INA219("ina219", "Current"));

  // Dissolved oxygen - an analog probe with a wireable temperature input for compensation, which
  // makes it the only Sensor that has an IN. Wired to the SHT below, once that exists.
  Sensor_DissolvedOxygen* dox = new Sensor_DissolvedOxygen("do", "Dissolved Oxygen", 36);
  frugal_iot.sensors->add(dox);

  // Temperature and Humidity sensor (SHT30)
  Sensor_SHT* sht;
  frugal_iot.sensors->add(sht = new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true));
  
  // The dissolved oxygen probe compensates against water temperature; the SHT will do here.
  dox->watertemperature->wireTo(sht->temperature->path());

  // Soil sensor 0%=4095 100%=0 pin=32 smooth=0 color=brown
  frugal_iot.sensors->add(new Sensor_Soil("soil", "Soil", 32, 4095, -100.0/4095, "brown", true));
  
  // The salt sensor (for LilyGo HiGrow) does not seem to work - got incorrect readings. TODO debug
  // Salt sensor 0%=0 100%=5000 pin=34 color=green
  frugal_iot.sensors->add(new Sensor_Analog("salt", "Salt", 34, 1, 0, 100, 0, 0.02, "green", true));
  
  // GPS over a hardware UART. ESP32 only: the ESP8266 envs in this example have no second UART
  // that can receive, and Sensor_GPS needs to read NMEA.
  #ifdef ESP32
    #ifndef SENSOR_GPS_RX_PIN
      #define SENSOR_GPS_RX_PIN 20
    #endif
    #ifndef SENSOR_GPS_TX_PIN
      #define SENSOR_GPS_TX_PIN 21
    #endif
    frugal_iot.sensors->add(new Sensor_GPS("GPS", &Serial1,
      SENSOR_GPS_RX_PIN, SENSOR_GPS_TX_PIN));
  #endif

  // Ultrasonic depth sensor, over Modbus/RS485. Needs three defines together - the slave id turns
  // on SYSTEM_MODBUS_WANT, which then #errors without the UART pins - so it is off unless
  // platformio.ini asks for it. See the note beside SENSOR_ULTRASONIC_SLAVE_ID there.
  #ifdef SENSOR_ULTRASONIC_SLAVE_ID
    // One System_RS485 per physical bus, shared by every slave on it. It is a plain class, not a
    // System_Base, so it is not added to frugal_iot.system - the sensor holds the pointer and
    // calls initialize() from its own setup(). Pins default from the SYSTEM_RS485_* defines.
    System_RS485* rs485 = new System_RS485(&Serial1);
    frugal_iot.sensors->add(new Sensor_Ultrasonic("ultrasonic", "Depth",
      DEFAULT_ultrasonic_ultrasonic_max, DEFAULT_ultrasonic_ultrasonic_color, true, rs485));
  #endif

  // ========= Actuators  ==============
  // Note Actuator_LedBuiltin added automatically if a pin is defined

  // Character LCD over I2C. Only when platformio.ini asks for it, since ACTUATOR_LCD_WANT is what
  // compiles the class in at all.
  #ifdef ACTUATOR_LCD_WANT
    frugal_iot.actuators->add(new Actuator_LCD());
  #endif

  // TODO-115 there is also a relay pin 19 on LilyGo - haven't tested it yet 
  #ifndef ACTUATOR_DIGITAL_PIN
    #ifdef RELAY_BUILTIN
      #define ACTUATOR_DIGITAL_PIN RELAY_BUILTIN
    #else
      #define ACTUATOR_DIGITAL_PIN 19
    #endif
  #endif
  frugal_iot.actuators->add(new Actuator_Digital("relay", "Relay", ACTUATOR_DIGITAL_PIN, DEFAULT_relay_on_color));

  // Controls that can be wire here, or in the UX
  Control* cb = new Control_Blinken("blinken", "Blinken", 5, 2);
  frugal_iot.controls->add(cb);
  cb->outputs[0]->wireTo(frugal_iot.messages->setPath("ledbuiltin/id"));

  Control_Hysteresis* ch = new Control_Hysteresis("controlhysteresis", "Control", 50, 1, 0, 100);
  frugal_iot.controls->add(ch);
  ch->outputs[0]->wireTo(frugal_iot.messages->setPath("ledbuiltin/on"));

  // Carousel cycles between Control_Oled displays, so it only makes sense on a board with one.
  #ifdef ACTUATOR_OLED_WANT
    frugal_iot.controls->add(new Control_Carousel("Carousel"));
  #endif

  //=================================LOGGER======
  // Add time if needed, which is currently only for data logging.
  frugal_iot.system->add(frugal_iot.time = new System_Time());
  #ifdef SYSTEM_SD_WANT
    System_SD* fs_SD = new System_SD(SYSTEM_SD_PIN); 
    frugal_iot.system->add(fs_SD);
  #else // If no SD then use LittleFS
    // LittleFS is always added - for configuration
    /*
    System_LittleFS* fs_LittleFS = new System_LittleFS();
    frugal_iot.system->add(fs_LittleFS);
    */
  #endif

  // Must be after sensor_sht for default wiring below
  Control_Logger* clfs = new Control_LoggerFS(
    "Logger",
    #ifdef SYSTEM_SD_WANT
      fs_SD, // Use SD card for logging
    #else
      frugal_iot.fs_LittleFS, // TODO-110 Using LittleFS for testing for now
    #endif
    "/",
    0x02, // Single log.csv with topicPath, time, value
    std::vector<IN*> {
    
      //INtext(const char * const sensorId, const char * const id, const char* const name, String* value, const char* const color, const bool wireable)
      new INtext("Logger", "log1", "log1", String(), "black", true),
      new INtext("Logger", "log2", "log2", String(), "black", true),
      new INtext("Logger", "log3", "log3", String(), "black", true)
      });
  frugal_iot.controls->add(clfs);
  // Wire the logger to the temperature sensor, it could be left blank and wired in the UX to a remote sensor
  clfs->inputs[0]->wireTo(sht->temperature->path()); // Wired to the temperatur sensor  // TODO-141 probably breaks as MQTT wont have setup yet
  // ==== GSHEETS 

  #ifndef CONTROL_GSHEETS_URL
    #define CONTROL_GSHEETS_URL "12345" // just for compilation testing
  #endif
  Control_Gsheets* cg = new Control_Gsheets("gsheets demo", CONTROL_GSHEETS_URL);
  frugal_iot.controls->add(cg);
  cg->track("temperature", frugal_iot.messages->path("sht/temperature")); // TODO-141 probably wont work as MQTT not setup
  // TODO backport this level of simplicity to Logger


 

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}

