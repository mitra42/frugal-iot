#ifndef LOCAL_H
#define LOCAL_H
/* 
* Being obsoleted - best to ignore this file and use platformio.ini
*/

// Organization behind this set of devices. Also used for MQTT userid

//TODO-141 these need to be defined somewhere, probably in platformio.ini
#ifdef KOPERNIK1
  #define SYSTEM_DISCOVERY_ORGANIZATION "kopernik"
  #define SYSTEM_MQTT_USER SYSTEM_DISCOVERY_ORGANIZATION  // Edit if you use something other than the organization name for MQTT user
  // Specify the password for your organization on the MQTT server
  #define SYSTEM_MQTT_PASSWORD "findingwhatworks"
#else
  #define SYSTEM_DISCOVERY_ORGANIZATION "dev"
  #define SYSTEM_MQTT_USER SYSTEM_DISCOVERY_ORGANIZATION  // Edit if you use something other than the organization name for MQTT user
  // Specify the password for your organization on the MQTT server
  #define SYSTEM_MQTT_PASSWORD "public"
#endif
#define SYSTEM_MQTT_SERVER "frugaliot.naturalinnovation.org"

// It can be slow developing by resting the portal each time, so if wanted define SSID and PASSWORD here, 
// just don't forget to comment these out before release
// If you define SSID and PASSWORD it will always connect to this - overriding whatever is set in the portal - this is NOT what you usually want.
// generally 
  // SSID_1 through SSID_9 define WiFi networks it will try if they show up in the scan, 
  // it will also remember any it successfully connects to after entering in the portal (Tools->Erase All Flash in arduino IDE to delete)
#if defined(BOARDMAGI5) || defined(BOARDMAGI2) || defined(BOARDMAGI1) || defined(BOARDMAGI6)
  #define SYSTEM_WIFI_SSID_1 "Ima"
  #define SYSTEM_WIFI_PASSWORD_1 "Feb346529"
  #define SYSTEM_WIFI_SSID_2 "Ryoko_30CFC3" // Muama device
  #define SYSTEM_WIFI_PASSWORD_2 "12345678"
#elif defined(KOPERNIK1)
  #define SYSTEM_WIFI_SSID_1 "KOPERNIK"
  #define SYSTEM_WIFI_PASSWORD_1 "findingwhatworks"
#elif defined(BALIWIFI)
  #define SYSTEM_WIFI_SSID_3 "La Luz 1"
  #define SYSTEM_WIFI_PASSWORD_3 "somosluz"
  #define SYSTEM_WIFI_SSID_4 "Garden Kafe"
  #define SYSTEM_WIFI_PASSWORD_4 "gardenonline"
  #define SYSTEM_WIFI_SSID_5 "GEREBIG VILLA 2"
  #define SYSTEM_WIFI_PASSWORD_5 "KADEKGRB"
  #define SYSTEM_WIFI_SSID_6 "GREEN RABBIT"
  #define SYSTEM_WIFI_PASSWORD_6 "FODLAB2020"
  #define SYSTEM_WIFI_SSID_7 "Zest"
  #define SYSTEM_WIFI_PASSWORD_7 "#zestubud"
  #define SYSTEM_WIFI_SSID_8 "GEREBIG VILLA 1"
  #define SYSTEM_WIFI_PASSWORD_8 "GRB575800"
#else
  #define SYSTEM_WIFI_SSID_1 "EXETEL 8BFC34 2.4G"
  #define SYSTEM_WIFI_PASSWORD_1 "urGGFHQN"
  #define SYSTEM_WIFI_SSID_2 "EXETEL 8BFC34 5G"
  #define SYSTEM_WIFI_PASSWORD_2 "urGGFHQN"
  #define SYSTEM_WIFI_SSID_3 "LOVEStARS"
  #define SYSTEM_WIFI_PASSWORD_3 "lotusponds1"
  #define SYSTEM_WIFI_SSID_4 "WiFi-A634E9" // Paseyo
  #define SYSTEM_WIFI_PASSWORD_4 "64346298"
#endif
  // The rest of this list are included for development purposes - its where I code ! 
  #define SYSTEM_WIFI_SSID_9 "Silver"
  #define SYSTEM_WIFI_PASSWORD_9 "O2IL-w8k2-w6ki-TzqG"

#ifdef KOPERNIK1
  #ifndef ESP32
    #error should be using "ESP32 WROVER MODULE" in the IDE, and looks like you are using a ESP32 board
  #endif
  #define ESP32_WROVER_DEV
  #define SYSTEM_WIFI_DEVICE "Kopernik pump control"
  #define SYSTEM_DISCOVERY_DEVICE_DESCRIPTION KOPERNIK1
  // main.cpp - relay;  pin=4
  // main.cpp - soil sensor; pin=36  SENSOR_ANALOG_ATTENUATION=ADC_11db
  //#define SYSTEM_OTA_KEY "kopernik1"
  #define SYSTEM_WIFI_PROJECT "lotus"
  //#define LOCAL_DEV_WANT // Include custom code
#endif //KOPERNIK1



#endif //LOCAL_H