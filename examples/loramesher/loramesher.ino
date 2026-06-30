/*
 *  Frugal IoT example - LoRaMesher demo - a work in progress
 *
 * Optional:
 *
 */

#include "Frugal-IoT.h"
#include "system_mqtt.h"

#ifdef ACTUATOR_OLED_WANT
  #include "control_oled_sht.h"
  #include "control_oled_loramesher.h"
  #include "control_carousel.h"
  Control_Carousel* carousel;
#endif

// Full MQTT path prefix of the device publishing SHT temperature/humidity
#ifndef LORAMESHER_REMOTE_SHT_DEVICE
  #define LORAMESHER_REMOTE_SHT_DEVICE "dev/lotus/esp32-ac3d7a" //TODO switch to the office one
#endif

// Change the parameters here to match your ...
// organization, project, id, description
System_Frugal frugal_iot("dev", "developers", "loramesher", "LoraMesher Node");


void setup() {
  // Battery sensor has to come before pre_setup, all others should come after
  #ifdef SENSOR_BATTERY_PIN
    frugal_iot.configure_battery(SENSOR_BATTERY_PIN); // Adds default battery sensor can specify (pin, Scale)
  #endif

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

  // Add local wifis here, or see instructions in the wiki for adding via the /data or captive portal
  // frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));

  /* LoRaMesher now added automatically if SYSTEM_LORAMESHER_WANT which is defined in _settings.h for LoRa boards
    // esp_log_level_set(LM_TAG, ESP_LOG_INFO);     // enable INFO logs from LoraMesher - but doesnt seem to work
    frugal_iot.loramesher = new System_LoraMesher(); // Held in a variable as future LoRaMesher will access it directly e.g. from MQTT
    frugal_iot.system->add(frugal_iot.loramesher);
  */

  #ifdef ACTUATOR_OLED_WANT
    Control_Oled_SHT* cos = new Control_Oled_SHT("Control OLED SHT");
    frugal_iot.controls->add(cos);
    cos->temperature->wireTo(LORAMESHER_REMOTE_SHT_DEVICE "/sht/temperature");
    cos->humidity->wireTo(LORAMESHER_REMOTE_SHT_DEVICE "/sht/humidity");
    cos->battery->wireTo(LORAMESHER_REMOTE_SHT_DEVICE "/battery/battery");


    Control_Oled_LoRaMesher* col = new Control_Oled_LoRaMesher("Control OLED");
    frugal_iot.controls->add(col);
    col->battery->wireTo(frugal_iot.messages->path("battery/battery"));
    col->enabled = false; // Second in carousel, starts hidden

    carousel = new Control_Carousel("Carousel");
    frugal_iot.controls->add(carousel);
    carousel->controls.push_back(cos);
    carousel->controls.push_back(col);
  #endif

  #ifdef BUTTON_BUILTIN
    Sensor_Button* button = new Sensor_Button("button", "Button", BUTTON_BUILTIN, "red");
    frugal_iot.buttons->add(button);
    #ifdef ACTUATOR_OLED_WANT
      button->singleClick->wireTo(frugal_iot.messages->setPath("carousel/select/cycle")); // cycles carousel display
    #endif
  #endif

  // Add sensors, actuators and controls
  #ifdef SENSOR_SHT_WANT
    frugal_iot.sensors->add(new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true));
  #endif
  // actuator_oled and actuator_ledbuiltin added automatically on boards that have them.
  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

#ifdef SYSTEM_LORAMESHER_DEBUG
void printAppData() {
  #ifdef ACTUATOR_OLED_WANT
    carousel->controls[carousel->selected]->act(); // Redisplay current carousel item
  #endif
}
#endif

// You can put custom code in here,
void loop()
{
  if (frugal_iot.timeForPeriodic)
  {
    Serial.println(millis());
    // Things which happen once for each sensor read period go here.
    // But note, you do not have to put sensor loops etc here - the loop and periodic functions in
    // each sensor and actuator and control are called from frugal_iot.loop()
    // This is also a good place to put things that check how long since last running
    frugal_iot.loramesher->printNetworkStatus();
    frugal_iot.loramesher->printRouteTable();
    // Serial.print("XXY " __FILE__ " "); Serial.println(__LINE__);
  }
  frugal_iot.loop(); // Do not delete this call to frugal_iot.loop
}
