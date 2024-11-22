# FrugalIoT: File structure

The following conventions apply to files in the FrugalIoT project
Note this is a draft as of Sept 2024, structure might change as others give input

* frugal-iot.ino  - main sketch
* _configuraton.h	- configuration defines, will be generated automatically at a later stage
* _settings.h		- settings file that conditionally includes based on _configuration.h
* _locals.h 		- settings unique to you - i.e. not in GitHub, includes which sensors & actuators wanted
* actuator_xxx.h and .cpp	- define a header and code for a specific actuator
* sensor_xxx.h and .cpp	- define header and code for a specific sensor
* logic_xxx.h and .cpp	- define header and code for a specific logic
* comms_xxx.h and .cpp	- define header and code for a specific comms
* system_xxx.h and .cpp - define header & code for system utilities. 
* docs/* documentation 

Look at actuator_blinken.h and actuator_blinken.c for an example of using namespace note how the namespace name is the section followed by capitalized item e.g. aBlinken or sClock

