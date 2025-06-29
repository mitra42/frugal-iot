# Frugal IoT
A platform to enable affordable sensor networks

The best source of informtion is the [Wiki here](https://github.com/mitra42/frugal-iot/wiki)

## Installation and testing POC

The repo is working and on multiple devices. 

HOWEVER - there was a recent refactor and there may be issues caused by that. 
ALSO - its only tested on Macs - so documentation for other platforms could be improved. 

#### Platforms
The intention is for this library to work in both PlatformIO and Arduino IDE. 
However the major refactor recently (June 2025) has (hopefully temporarily) broken Arduino IDE support.

Working in the Arduino IDE is simpler, but seriously, its like working with one hand tied behind your back!
We strongly encourage installing Visual Studio with the Platform IO plugin. 

### Getting help 

If you having an installation related question please check the
[Wiki](https://github.com/mitra42/frugal-iot/wiki)
and if it you cannot find the answer there, please raise a new issue at (https://github.com/mitra42/frugal-iot/issues) 
make sure the title starts "INSTALLATION: ".

Please note that if you cannot understand how to install then that is probably a problem with 
the quality of our documentation so I am happy to help! 

### Test and development environment overview 

There are a few components of the environment - with separate repos

* [Dev board](https://www.github.com/mitra42/frugal-iot) - This repo - programmed with PlatformIO in C++
* MQTT Broker - currently off-the-shelf Mosquitto, with a development server running on frugaliot.naturalinnovation.org
* [MQTT Logger](https://www.github.com/mitra42/frugal-iot-logger) logs MQTT messsages to files for later analysis and graphing.
* [HTTP Management server](https://www.github.com/mitra42/frugal-iot-server) - programmed in Node/Javascript, tested on Macs and Linux and 
  with a demo server at frugaliot.naturalinnovation.org which incorporates the `logger`
* [HTTP/CSS/JS client](https://www.github.com/mitra42/frugal-iot-client). Its not dependent on any platform/framework, and uses Webcomponents for its modularity so it should run in any modern browser (including most phones)
* Stand alone server intended to run on sites with multiple sensors - not yet started, but it will probably be a Raspberry Pi unless we can shoe-horn it into a ESP32

Because the demo server is running on `https://frugaliot.naturalinnovation.org`, it is perfectly possible to develop the firmware without running your own servers, 
or to fork the client, and interact with the servers.  

### Test Harware/Firmware environment
We test on a variety of environments - most often the ESP8266 Lolin D1 Mini; ESP32 S2 mini or C3 pico; and the Lilygo TTGO Lora series and Sonoff R2 switches. More details of supported hardware is on the
[Wiki:Supported Hardware](https://github.com/mitra42/frugal-iot/wiki/Supported-Hardware)


#### Platform IO  basics - skip if you know what you are doing
(June 2025: These need updating since the major refactor)

#### Arduino IDE basics - skip if you know what you are doing
(June 2025: See note above these will need updating when support for Arduino IDE resumes)
* In Arduino IDE 
* Open `~/frugal-iot/`
* Tools -> Upload Speed -> 460800
* Tools -> Manage Libraries -> Search for, and install:
  * Libraries used by almost all cases
    * ESP-WiFiSettings by Juerd Waalboer
    * MQTT by Joel Gaehwiler
  * Libraries only needed if you use specific sensors. 
    * SHT85 by Rob Tillaart;
    * DHTNEW also by Rob Tillaart (SENSOR_DHT_WANT to enable)
    * BH1750 from Christopher Laws (SENSOR_BH1750_WANT to enable)
    * If you add more sensors make sure to add any library requirement here (TO_ADD_SENSOR)
  * Libraries only needed if you use specific sensors. 
    * If you add more actuators make sure to add any library requirement here (TO_ADD_ACTUATOR)
* Tools -> Board 
  * Check you have selected the dev board
  * TODO add instructions, or a link here for adding new boards to an Arduino IDE - most will need to do this. 
* Tools -> Port 
  * Select the port your device is plugged into
* Tools -> Serial Monitor 
  * This can be tricky - it should open at 460800 but might need changing after the first run at which point it should remember


### Installing and testing on a dev board

#### Basic test
(June 2025: These need updating since the major refactor)

* Copy _local-template.h to _local.h. For now leave the ORGANIZATION as "dev" unless you talk to us. 
  Add the MQTT server, userid and password, if you aren't going to use `naturalinnovation.org` 
  Uncomment lines for any sensors you have on the board.
* Compile and Flash to your dev board
* On a Wifi device such as a phone
  * Connect to the wifi node of the board which will have a SSID like esp8266-12345
  * Configure the Wifi SSID and Password of your router,
  * Pick the name for your project (use the same one for all your boards), 
    * Let us know by either (opening a Github issue)[https://github.com/mitra42/frugal-iot/issues/new/choose], or editing `html/server/config.yaml` and doing a Pull Request
  * Give your board a name and description
  * Click SAVE then RESTART
* The device should now connect to the mqtt server at frugaliot.naturalinnovation.org

If you get the configuration wrong, you'll need (for now - part of issue#22) to 
* erase the settings - on Arduino IDE this is Tools->Erase->All Flash
* recompile/flash.

Open a browser to https://frugaliot.naturalinnovation

#### Other languages
At the moment (Nov2024) - see [issue #42](https://github.com/mitra42/frugal-iot/issues/42), language support is being added. 

You can define `-D LANGUAGE_DEFAULT "de"` in `platformio.ini` to default to German. 
And you can add `-D LANGUAGE_DE` to `platformio.ini` to *only* compile German (saving program space)
You can add multiple lines to compile multiple languages.

### Server development

Because of the demo servers on frugaliot.naturalinnovation.org, it is perfectly possible to develop the firmware without running your own servers.
But if you want to do server or client development ... see the README.md on each component's repository.

