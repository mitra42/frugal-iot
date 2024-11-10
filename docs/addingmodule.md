# Frugal IoT - adding a device

To add a device to the FrugalIot app, 
the best way is to cut and edit from an existing example .cpp and .h files.

It has to be hooked into a few places, look for `TO_ADD_ACTUATOR` or `TO_ADD_SENSOR` 

e.g. to include a Sensor Foo, with files sensor_foo.cpp and sensor_foo.h
the namespace will be sFoo

* _configuration.h*: Define a section with any parameters to use that are common across (most) uses
* _local.h*: Define a SENSOR_FOO_WANT, and any parameters likely to be unique to your instance

follow the patterns of other variables i.e. they should all start SENSOR_FOO_


*frugal_iot.ino*: Add the following code at the different TO_ADD_SENSOR places
```
#ifdef SENSOR_FOO_WANT
#include "sensor_foo.h"
#endif
```
```
#ifdef SENSOR_FOO_WANT
  sFoo::setup();
#endif
```
and if, and only if, there is a loop() function on the sensor or actuator
note that most actuators do not have a loop as they respond to incoming MQTT
```
#ifdef SENSOR_FOO_WANT
  sFoo::loop();
#endif
```



include sensor_foo and call the sFoo::setup() and sFoo::loop()
