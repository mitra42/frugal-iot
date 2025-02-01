# Frugal IoT - Developer FAQ*

## FrugalIoT: File structure

The following conventions apply to node files in the FrugalIoT project
Note this is a draft as of Sept 2024, structure might change as others give input

* `frugal-iot.ino`  - main sketch
* `_configuraton.h`	- configuration defines, will be generated automatically at a later stage
* `_settings.h`		- settings file that conditionally includes based on _configuration.h
* `_locals.h` 		- settings unique to you - i.e. not in GitHub, includes which sensors & actuators wanted
* `actuator_xxx.h` and `.cpp`	- define a header and code for a specific actuator
* `sensor_xxx.h` and `.cpp`	- define header and code for a specific sensor
* `control_xxx.h` and `.cpp`	- define header and code for control mechanisms
* `system_xxx.h` and `.cpp` - define header & code for system utilities. 
* `docs/*` documentation 

Look at `actuator_blinken.h` and `actuator_blinken.c` for an example of using namespace note how the namespace name is the section followed by capitalized item e.g. `aBlinken` or `sClock`

## Adding functionality

### Organizations, Projects, Nodes, Sensors and Topics.
The hierarchy of entities in the FrugalIoT system are broken up as follow.
* _Organization_: Partner entity - e.g. an NGO.  `dev` is reserved for the developers.
* _Project_: A subset of an organization, for example it could be a single location with a group of sensors, or a group of similar sensors in different locations.
* _Node_: A device - with a connection to the net.
* _Sensor_: A sensor on a node.

Topics follow this hierarchy e.g. `dev/lotus/esp8266_ab123/humidity` would be a `humidity` sensor on the physical node
`esp8266_ab123` at the site `lotus` (Lotus Ponds) run by the `dev` organization (developers). 

#### Adding an organization

Each group using Frugal IoT should have an `organization`
- `organizations` will be used for controlling permissions etc
- The organization's abbreviation is the first part of `topics`

To add an Organization: Select
* A short abbreviation - one word, all lower case e.g. "dev"
* A password - typical password rules, no spaces e/.g. "mysecret!1234"
  * (at the moment this isn't very secret, we'll fix this as part of [issue#89[(https://github.com/mitra42/frugal-iot/issues/89)
  
* On `frugaliot.ovation.org` (currently only mitra can do this - create an issue):
  * `mosquitto_passwd -b /etc/mosquitto` <organization abbreviation> password
  * `service mosquitto restart`   # This might not be necessary, its unclear

* Create a configuration file in [frugal-iot-server/config.d/organizations](https://github.com/mitra42/frugal-iot-server/tree/main/config.d/organizations)
  * use the existing dev.yaml as a template
  * Submit a PR as usual (ask us if you dont know how to do this)
* Edit your local.h.
  * Copy and edit a local.h from `_local-template.h` if you don't already have one.
  * edit the `SYSTEM_DISCOVERY_ORGANIZATION` parameter to be the organization abbreviation.

Note: A longer name, and a description are likely to be required for organizations in future.

### Adding a sensor or actuator.

To add a sensor or actuator to the FrugalIot app, 

It has to be hooked into a few places, look for `TO_ADD_ACTUATOR` or `TO_ADD_SENSOR` and needs `.cpp` and `.h` files.

Start an issue - let people know you are working on it, use that issue to ask for help with any problems you hit. 

#### .cpp and .h files

The best way is to cut and edit from an existing example .cpp and .h files.

If its a digital actuator, look at `actuator_relay.cpp` for a simple example, 
or `actuator_ledbuiltin.cpp` for a more complex (subclassing) example.

For an analogue sensor, look at `sensor_analog_example` 
or `sensor_battery` for a more complex (subclassing) example.

e.g. to include a Sensor Foo, add files `sensor_foo.cpp` and `sensor_foo.h`
the namespace will be `sFoo` and class will be `SensorFoo`

In the top of the .cpp and .h files, define defaults for any paramaters,
follow the conventions - some common ones
* SENSOR_FOO_PIN - the active pin
* SENSOR_FOO_ADDRESS - I2C address etc
* SENSOR_FOO_MS - how often, in milliseconds to read it
* SENSOR_FOO_DEBUG - if defined then print copious debugging - wrap all (or almost all) your `Serial.print` with this. 

#### _local.h*
Define a SENSOR_FOO_WANT, and any parameters which overwrite defaults - e.g. its common to `#define SENSOR_FOO_MS 900000` in the `.cpp` to read every 15 minutes, then `#define SENSOR_FOO_MS 10000` to read every 10 seconds when debugging.
  
#### system_discovery.cpp
If it should appear in UI, then needs to include its discovery string (look for TO_ADD_SENSOR) 

#### frugal_iot.ino*
Add the following code at the different TO_ADD_SENSOR places
```
#ifdef SENSOR_FOO_WANT
#include "sensor_foo.h"
#endif
```
There are two different approaches depending on hard coded instances or classes

#### frugal_iot.ino for hard coded
```
#ifdef SENSOR_FOO_WANT
  sFoo::setup();
#endif
```
and if, and only if, there is a loop() function on the sensor or actuator
note that most actuators do not have a loop as they respond to incoming MQTT
and that
```
#ifdef SENSOR_FOO_WANT
  sFoo::loop();
#endif
```
#### frugal_iot.ino for classes
create an instance of the class, setup or loop is handled by the class's code.
```
#ifdef ACTUATOR_RELAY_WANT
Actuator_Digital* a2 = new Actuator_Digital(ACTUATOR_RELAY_PIN, "relay");
#endif
```



