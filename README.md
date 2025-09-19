# Frugal IoT
A platform to enable affordable sensor networks

The best source of information is the [Wiki here](https://github.com/mitra42/frugal-iot/wiki)

## Installation and testing POC

The repo is working and on multiple devices. 

NOTE - its only tested on Macs - so documentation for other platforms could be improved. 

#### Platforms
The intention is for this library to work in both PlatformIO and Arduino IDE. 

Working in the Arduino IDE is simpler, but seriously, its like working with one hand tied behind your back. We strongly encourage installing Visual Studio with the Platform IO plugin. 

### Getting help 

If you having an installation related question please check the
[Wiki](https://github.com/mitra42/frugal-iot/wiki)
and if you cannot find the answer there, please raise a new issue at (https://github.com/mitra42/frugal-iot/issues) 
make sure the title starts "INSTALLATION: ".

Please note that if you cannot understand how to install then that is probably a problem with the quality of our documentation so I am happy to help! 

### Test and development environment overview 

There are a few components of the environment - with separate repos

* [Dev board](https://www.github.com/mitra42/frugal-iot) - This repo - programmed with PlatformIO in C++
* MQTT Broker - currently off-the-shelf Mosquitto, with a development server running on frugaliot.naturalinnovation.org
* [MQTT Logger](https://www.github.com/mitra42/frugal-iot-logger) logs MQTT messsages to files or Google Sheets for later analysis and graphing.
* [HTTP Management server](https://www.github.com/mitra42/frugal-iot-server) - programmed in Node/Javascript, tested on Macs and Linux and 
  with a demo server at frugaliot.naturalinnovation.org which incorporates the `logger`
* [HTTP/CSS/JS client](https://www.github.com/mitra42/frugal-iot-client). Its not dependent on any platform/framework, and uses Webcomponents for its modularity so it should run in any modern browser (including most phones)
* Stand alone server intended to run on sites with multiple sensors - this project has not yet started, but it will probably be a Raspberry Pi unless we can shoe-horn it into a ESP32

Because the demo server is running on `https://frugaliot.naturalinnovation.org`, it is perfectly possible to develop the firmware without running your own servers, 
or to fork the client, and interact with the servers.  

### Test Harware/Firmware environment
We test on a variety of environments - most often the ESP8266 Lolin D1 Mini; ESP32 S2 mini or C3 pico; and the Lilygo TTGO Lora series and Sonoff R2 switches. More details of supported hardware is on the
[Wiki:Supported Hardware](https://github.com/mitra42/frugal-iot/wiki/Supported-Hardware)


#### Platform IO  basics - skip if you know what you are doing

* In whatever directory you want ....
* git clone https://github.com/mitra42/frugal-iot.git

In PlatformIO
* File > Open > frugal-iot > examples > sht30
* Select the board and port on the bottom bar
* It will install the necessary libraries and compile
* 


#### Arduino IDE basics - skip most of this if you know what you are doing

On https://github.com/mitra42/frugal-iot

* Code > Download Zip

Then In Arduino IDE 
* top menu > Sketch > Include Library > Add .ZIP file and select Zip file opened
* This library should now be available to your own sketches
* File > Examples > scroll down to bottom group "Examples form Custom Libraries"
* Frugal-IoT > select any example SHT30 is simplest
* See notes below on configuring your example
* Tools -> Upload Speed -> 115200
* Tools -> Manage Libraries -> Search for, and install:
  * Libraries used by almost all cases
    * MQTT by Joel Gaehwiler
    * All by ESP32Async ... (careful there are some similarly named forks by other people of these three)
    * Async TCP (for ESP32s)
    * ESP Async TCP (for ESP8266)
    * ESP Async WebServer
  * Libraries only needed if you use specific sensors or actuators
    * SHT85 by Rob Tillaart;
    * DHTNEW also by Rob Tillaart
    * BH1750 from Christopher Laws
    * Button2 by Lennart Hennigs
    * HX711 by Rob Tillaart for Load Sensors
    * adafruit/Adafruit SSD1306@^2.5.0 - if running on a devive ith OLED 
    * adafruit/Adafruit GFX Library@^1.10.13 - if running on a devive ith OLED 
    * jaimi5/LoRaMesher - on LoRa compatable devices to build a mesh network

    * If you add more sensors make sure to add any library requirement here (TO_ADD_SENSOR)
* Tools -> Board 
  * Check you have selected the dev board
  * TODO add instructions, or a link here for adding new boards to an Arduino IDE - most will need to do this. 
* Tools -> Port 
  * Select the port your device is plugged into
* Tools -> Serial Monitor 
  * This can be tricky - it should open at 115200 but might need changing after the first run at which point it should remember


### Installing and testing on a dev board

#### Basic test
* Edit around line 31 of the .ino files `addWiFi` to have your own SSID and Password, 
* You can duplicate this line for multiple WiFi's 
* If you do not add a WiFi that it can see, then a portal will be opened - see below. 
* On PlatformIO there is a nicer alternative way to put the WiFi's and configuration info in data files
  * Add files to the /data directory of your project - the filename is the SSID, and the content is the password
  * PlatformIO -> Platform -> Build FileSystem Image
  * PlatformIO -> Platform -> Upload FileSystem Image
    * Most common cause of failure here, is having a terminal window running Ctrl-C out of it. 
* Add the MQTT server, userid and password, if you aren't going to use `naturalinnovation.org` 
* Check the examples for how to add sensors, controls and actuators. 
* Compile and Flash to your dev board
  * On PlatformIO this is the compile button (✔︎ and > on bottom bar)
  * On Arduino its the ✔︎ and > on the top bar
* If you did not add a valid WiFi
  * On a WiFi device such as a phone
  * Connect to the wifi node of the board which will have a SSID like esp8266-12345
  * Configure the WiFi SSID and Password of your router,
  * Pick the name for your project (use the same one for all your boards),  
    * You can use "developers" for testing,
    * Or pick your own and let us know by either (opening a Github issue)[https://github.com/mitra42/frugal-iot/issues/new/choose], or editing `html/server/config.yaml` and doing a Pull Request
  * Give your board a name and description
  * Click SAVE then RESTART
* The device should now connect to the mqtt server at frugaliot.naturalinnovation.org

If you get the configuration wrong, you'll need (for now - part of issue#22) to 
* erase the settings - on Arduino IDE this is Tools->Erase->All Flash
* recompile/flash.

Open a browser to https://frugaliot.naturalinnovation

#### Other languages
Add to system_language.cpp and system_language.h 
and in [the client repo](https://www.github.com/mitra42/frugal-iot-client) add to `webcomponents.js`

You can define `-D LANGUAGE_DEFAULT "de"` in `platformio.ini` to default to German. 
And you can add `-D LANGUAGE_DE` to `platformio.ini` to *only* compile German (saving program space)
You can add multiple lines to compile multiple languages.
Once the language is selected on the portal (see above) it will be remembered

### Server development

Because of the demo servers on frugaliot.naturalinnovation.org, it is perfectly possible to develop the firmware without running your own servers.
But if you want to do server or client development ... see the README.md on each component's repository.

