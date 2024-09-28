# Frugal IoT - adding a device

To add a device to the FrugalIot app, 
the best way is to cut and edit from an existing example .cpp and .h files.

It has to be hooked into a few places:

e.g. to include a Sensor Foo, with files sensor_foo.cpp and sensor_foo.h
the namespace will be sFoo

*_configuration.h*: Define a SENSOR_FOO_WANT, and any parameters to be used,
follow the patterns of other variables i.e. they should all start SENSOR_FOO_

*_settings.h*: Include the .h files 

*frugal_iot.ino*: call the sFoo::setup() and sFoo::loop()
