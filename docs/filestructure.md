# FrugalIoT: File structure

The following conventions apply to files in the FrugalIoT project
Note this is a draft as of Sept 2024, structure might change as others give input

* frugal-iot.ino  - main sketch
* _configuraton.h	- configuration defines, will be generated automatically at a later stage
* _settings.h		- settings file that conditionally includes based on _configuration.h
* actuator_xxx.h and .cpp	- define a header and code for a specific actuator
* sensor_xxx.h and .cpp	- define a header and code for a specific sensor
* logic_xxx.h and .cpp	- define a header and code for a specific logic
* comms_xxx.h and .cpp	- define a header and code for a specific comms

Look at actuator_blinken.h and actuator_blinken.c for an example of using namespace

