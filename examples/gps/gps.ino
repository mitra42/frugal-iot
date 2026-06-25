/*
 * Frugal-IoT GPS example — Heltec WiFi LoRa 32 V4 + Quectel L76K
 *
 * Publishes to MQTT topics under dev/developers/gps/:
 *   latitude, longitude, altitude, speed, course, hdop, satellites
 *   position  (ISO 6709 combined string, e.g. "+27.591600+086.564000+8850.0/")
 *   utc_time  (HH:MM:SS)
 *
 * The onboard OLED shows lat/lon/alt/speed/course/satellites/hdop in real time.
 *
 * Power note: Power_Loop with a 1-second cycle matches the L76K's 1 Hz NMEA output.
 * readValidateConvertSet() blocks for up to SENSOR_GPS_READ_TIMEOUT_MS (default 1100 ms)
 * to obtain a fresh fix, so the effective cycle is ~1.1 s minimum. For slower readings
 * increase cycle_ms; the UART flush in readValidateConvertSet() handles buffer overflow
 * automatically. See sensor_gps.cpp for a full discussion of the tradeoffs.
 */

#include "Frugal-IoT.h"
#include "sensor_gps.h"
#ifdef ACTUATOR_OLED_WANT
  #include "control_oled_gps.h"
  #include "control_oled_loramesher.h"
  #include "control_carousel.h"
  Control_Carousel* carousel;
#endif

System_Frugal frugal_iot("dev", "developers", "gps", "GPS Sensor");

void setup() {
  // Power_Loop: always awake, 1-second cycle (matches GPS 1 Hz update rate).
  frugal_iot.configure_power(Power_Loop, 1000, 1000);
  frugal_iot.pre_setup();
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");

  // Heltec V4 GNSS connector: RX=GPIO39, TX=GPIO38, power=GPIO34 (active LOW).
  // All pin and baud defaults come from SENSOR_GPS_* defines in platformio.ini.
  // Serial1 is used per Heltec example code; the ESP32-S3 remaps it to GPIOs 39/38.
  Sensor_GPS* gps = new Sensor_GPS("GPS", &Serial1,
      SENSOR_GPS_RX_PIN, SENSOR_GPS_TX_PIN);
  frugal_iot.sensors->add(gps);

  #ifdef ACTUATOR_OLED_WANT
    Control_Oled_GPS* cog = new Control_Oled_GPS("OLED GPS");
    frugal_iot.controls->add(cog);
    cog->latitude->wireTo(  frugal_iot.messages->path("gps/latitude"));
    cog->longitude->wireTo( frugal_iot.messages->path("gps/longitude"));
    cog->altitude->wireTo(  frugal_iot.messages->path("gps/altitude"));
    cog->speed->wireTo(     frugal_iot.messages->path("gps/speed"));
    cog->course->wireTo(    frugal_iot.messages->path("gps/course"));
    cog->satellites->wireTo(frugal_iot.messages->path("gps/satellites"));
    cog->hdop->wireTo(      frugal_iot.messages->path("gps/hdop"));

    // Add LoRaMesher debug display
    Control_Oled_LoRaMesher* col = new Control_Oled_LoRaMesher("Control OLED");
    frugal_iot.controls->add(col);
    col->battery->wireTo(frugal_iot.messages->path("battery/battery"));
    col->enabled = false; // Second in carousel, starts hidden

    // Define a carousel for the displays and add cog
    carousel = new Control_Carousel("Carousel");
    frugal_iot.controls->add(carousel);
    carousel->controls.push_back(cog);
    carousel->controls.push_back(col);

  #endif

  // Support button and use it cycle carousel
  #ifdef BUTTON_BUILTIN
    Sensor_Button* button = new Sensor_Button("button", "Button", BUTTON_BUILTIN, "red");
    frugal_iot.buttons->add(button);
    #ifdef ACTUATOR_OLED_WANT
      button->singleClick->wireTo(frugal_iot.messages->setPath("carousel/select/cycle")); // cycles carousel display
    #endif
  #endif

  frugal_iot.setup();
}

void loop() {
  frugal_iot.loop();
}

// Callback from LoRaMesher
#ifdef SYSTEM_LORAMESHER_DEBUG
void printAppData() {
  #ifdef ACTUATOR_OLED_WANT
    carousel->controls[carousel->selected]->act(); // Redisplay current carousel item
  #endif
}
#endif
