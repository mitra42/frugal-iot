/* Frugal IoT - main .h file - includes all the others */

#include "_settings.h" // Load board definitions first
//TO_ADD_ACTUATOR - follow the pattern below and add any variables and search for other places tagged TO_ADD_ACTUATOR
#include "actuator_ledbuiltin.h"

//TO-ADD-SENSOR - add any new sensors here (in alphabetical order)
#include "sensor_soil.h"
#include "sensor_battery.h"
#include "sensor_sht.h"
#include "sensor_dht.h"
#include "sensor_bh1750.h"
#include "sensor_button.h"
#include "sensor_ds18b20.h"
#include "sensor_ina219.h"
#include "sensor_ms5803.h"
#include "sensor_loadcell.h"
#include "sensor_ens160aht21.h"

//TO-ADD-CONTROL
#include "control.h"
#include "control_blinken.h"
#include "control_gsheets.h"
#include "control_hysterisis.h"
#include "control_logger_fs.h"

//TO-ADD-SYSTEM - note dont need to add here if adding in system_frugal.h
#include "system_fs.h"
#include "system_frugal.h"

#include "misc.h" // for lprintf and StringF
