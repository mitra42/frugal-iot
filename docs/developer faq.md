# Frugal IoT - Developer FAQ*

## FrugalIoT: File structure

The following conventions apply to files in the FrugalIoT project
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
* A short abbreviation - one word, all lower case
* A password - typical password rules, no spaces
  
* On `naturalinnovation.org` (currently only mitra can do this):
  * `mosquitto_passwd -b /etc/mosquitto` <organization abbreviation> password
  * `service mosquitto restart`   # This might not be necessary, its unclear

* Edit `html/server/config.yaml` in this git repo
  * (ask us if you dont know how to do this)
  * Add under organizations, careful with indentation.
  * Submit a PR as usual
  * Note: this file might move along with the server code to a new repo - ask if its missing. see [issue #84](https://github.com/mitra42/frugal-iot/issues/84)
  * Note: There will be a new `config.d` for per-organization configuration. See [issue #90](https://github.com/mitra42/frugal-iot/issues/90)
* Edit your local.h.
  * Copy and edit a local.h from `_local-template.h` if you don't already have one.
  * edit the `SYSTEM_DISCOVERY_ORGANIZATION` parameter to the be the organization abbreviation.

Note: A longer name, and a description are likely to be required for organizations in future.

### Adding a sensor or actuator.

To add a sensor or actuator to the FrugalIot app, 
It has to be hooked into a few places, look for `TO_ADD_ACTUATOR` or `TO_ADD_SENSOR` and needs `.cpp` and `.h` files.

#### .cpp and .h files

The best way is to cut and edit from an existing example .cpp and .h files.

If its a digital actuator, look at `actuator_relay.cpp` for a simple example, 
or `actuator_ledbuiltin.cpp` for a more complex (subclassing) example.

For an analogue sensor, look at `sensor_analog_example` 
or `sensor_battery` for a more complex (subclassing) example.

e.g. to include a Sensor Foo, add files `sensor_foo.cpp` and `sensor_foo.h`
the namespace will be `sFoo`

#### _configuration.h*
Define a section with any parameters to use that are common across (most) uses
  follow the patterns of other variables i.e. they should all start SENSOR_FOO_

#### _local.h*
Define a SENSOR_FOO_WANT, and any parameters likely to be unique to your instance
  
#### system_discovery.cpp
If it should appear in UI, then needs to include its discovery string

#### frugal_iot.ino*
Add the following code at the different TO_ADD_SENSOR places
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


