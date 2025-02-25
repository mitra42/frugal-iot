# Frugal IoT
A platform to enable affordable sensor networks

For now see the Google Doc: [Frugal IoT - enabling low cost sensor networks](https://docs.google.com/document/d/1hOeTFgbbRpiKB_TN9R2a2KtBemCyeMDopw9q_b0-m2I/edit?usp=sharing)
Scroll down to the bottom for links to other docs. 

Also see the subdirectory ./docs

## Installation and testing POC

This is still at POC stage, so installation is non-optimal, and only tested on Macs - assistance appreciated to improve that ! 

Please recheck these instructions as they will probably change often as code evolves to an easier POC setup, 
i.e. installation of each early version might be different !

### Getting help 

If you having an installation related question please check the (./docs/develper faq.md) 
and if it you cannot find the answer there, please raise a new issue at (https://github.com/mitra42/frugal-iot/issues) 
make sure the title starts "INSTALLATION: ".

Please note that if you cannot understand how to install that is probably a problem with 
the quality of our documentation so happy to help! 

### Test and development environment overview 

There are a few components of the environment - with separate repos

* [Dev board](https://www.github.com/mitra42/frugal-iot) - programmed from the Arduino IDE in C++
* MQTT Broker - currently off-the-shelf Mosquitto, with a development server running on frugaliot.naturalinnovation.org
* [MQTT Logger](https://www.github.com/mitra42/frugal-iot-logger) logs MQTT messsages to files for later analysis and graphing.
* [HTTP Management server](https://www.github.com/mitra42/frugal-iot-server) - programmed in Node/Javascript, tested on Macs and Linux and 
  with a demo server at frugaliot.naturalinnovation.org:8080 which incorporates the `logger`
* [HTTP/CSS/JS client](https://www.github.com/mitra42/frugal-iot-client). Its not dependent on any platform/framework, and uses Webcomponents 
  for its modularity so it should run in any modern browser (including most phones)
* Stand alone server intended to run on sites with multiple sensors - not yet started, but it will probably be a Raspberry Pi unless we can shoe-horn it into a ESP32

Because the demo servers on `https://frugaliot.naturalinnovation.org`, it is perfectly possible to develop the firmware without running your own servers, 
or to fork the client, and interact with the servers.  

### Test Harware/Firmware environment
Our current test dev board is a ESP8266 Lolin D1 Mini with a SHT30 Temp & Humidity shield, also from Lolin, 
I expect to start testing other combinations of boards and sensors soon. 
Instructions for other board/sensor combinations are welcome. 

I also test on Lolin C3 Pico (ESP32-C3), and the code has been run on SONOFF R2 digital switches, and on the D1 Mini Pro (Green); 


#### Arduino IDE basics - skip if you know what you are doing
* In Arduino IDE 
* Open `~/frugal-iot/`
* Tools -> Upload Speed -> 460800
* Tools -> Manage Libraries -> Search for, and install:
  * Libraries used by almost all cases
    * ESP-WiFiSettings by Juerd Waalboer
    * MQTT by Joel Gaehwiler
  * Libraries only needed if you use specific sensors. 
    * SHT85 by Rob Tillaart; (SENSOR_SHT_WANT to enable SHT3X or SHT85 series sensors)
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
* Copy _local-template.h to _local.h. For now leave the ORGANIZATION as "dev" unless you talk to us. 
  Add the MQTT server, userid and password, if you aren't going to use `naturalinnovation.org` 
  Uncomment lines for any sensors you have on the board.
* Check _configuration.h t - for now this is a good place to set how much debugging you want.  TODO maybe move to _local.h
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

You can define `#define LANGUAGE_DEFAULT "de"` in `_local.h` to default to German. 
And you can add `#define LANGUAGE_DE` to `_local.h` to *only* compile German (saving program space)
You can add multiple lines to compile multiple languages.

To only compile one language (which saves memory) go into Arduino/libraries/ESP-WiFiSettings/WiFiSettings_strings.h 
and add one or more lines at the top like `#define LANGUAGE_DE`. 

Unfortunately we haven't found a way to control the library compilation from the `_local.h` file.

### Server development

Because of the demo servers on frugaliot.naturalinnovation.org, it is perfectly possible to develop the firmware without running your own servers.
But if you want to do server or client development ... see the README.md on each component's repository.




