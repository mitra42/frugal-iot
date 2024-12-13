# Frugal IoT - Developer FAQ*

### Organizations, Projects, Nodes, Sensors and Topics.
The hierarchy of entities in the FrugalIoT system are broken up as follow.
* _Organization_: Partner entity - e.g. an NGO.  `dev` is reserved for the developers.
* _Project_: A subset of an organization, for example it could be a single location with a group of sensors, or a group of similar sensors in different locations.
* _Node_: A device - with a connection to the net.
* _Sensor_: A sensor on a node.

Topics follow this hierarchy e.g. `dev/Lotus Ponds/esp8266_ab123/humidity` would be a `humidity` sensor on the physical node
`esp8266_ab123` at the site `Lotus Ponds` run by the `dev` organization (developers). 

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

