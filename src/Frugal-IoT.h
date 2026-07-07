/* Frugal IoT - main .h file - includes all the others */

#include "_settings.h" // Load board definitions first
//TO_ADD_ACTUATOR - follow the pattern below and add any variables and search for other places tagged TO_ADD_ACTUATOR
#include "actuator/ledbuiltin.h"
#include "actuator/lcd.h"
#include "actuator/oled.h"

//TO-ADD-SENSOR - add any new sensors here (in alphabetical order)
#include "sensor/soil.h"
#include "sensor/battery.h"
#include "sensor/sht.h"
#include "sensor/dht.h"
#include "sensor/bh1750.h"
#include "sensor/button.h"
#include "sensor/ds18b20.h"
// #include "sensor/ina219.h" // commented out till library identified and tested
#include "sensor/ms5803.h"
#include "sensor/loadcell.h"
#include "sensor/ens160aht21.h"

//TO-ADD-CONTROL
#include "control/control.h"
#include "control/blinken.h"
#include "control/gsheets.h"
#include "control/hysteresis.h"
#include "control/logger_fs.h"

//TO-ADD-SYSTEM - note dont need to add here if adding in system_frugal.h
#include "system/fs.h"
#include "system/frugal.h"

#include "misc.h" // for lprintf and StringF
