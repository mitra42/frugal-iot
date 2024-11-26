# frugal-iot
A platform to enable affordable sensor networks

For now see the Google Doc: [Frugal IoT - enabling low cost sensor networks](https://docs.google.com/document/d/1hOeTFgbbRpiKB_TN9R2a2KtBemCyeMDopw9q_b0-m2I/edit?usp=sharing)
Scroll down to the bottom for links to other docs. 

Also see the subdirectory ./docs

## Installation and testing POC

This is still at POC stage, so installation is non-optimal, and only tested on Macs - assistance appreciated to improve that ! 

Please recheck these instructions as they will probably change often as code evolves to an easier POC setup, 
i.e. installation of each early version might be different !

### Getting help 
If you having an installation related question please check the (./FAQ.md) 
and if it you cannot find the answer there, please raise a new issue at (https://github.com/mitra42/frugal-iot/issues) 
make sure the title starts "INSTALLATION: ".
Please note that if you cannot understand how to install that is probably a problem with 
the quality of our documentation so happy to help! 

### Test and development environment overview 
There are a few components of the environment

* Dev board - programmed from the Arduino IDE in C++
* MQTT Broker - currently off-the-shelf Mosquitto, with a development server running on naturalinnovation.org
* HTTP Management server - programmed in Node/Javascript, tested on Macs and Linux and with a demo server at naturalinnovation.org:8080
* HTTP/CSS/JS client. Its not dependent on any platform, and uses Webcomponents for its modularity so it should run in any modern browser (including most phones)
* Stand alone server intended to run on sites with multiple sensors - not yet started, but it will probably be a Raspberry Pi unless we can shoe-horn it into a ESP32

Because of the demo servers on naturalinnovation.org, it is perfectly possible to develop the firmware without running your own servers. 

### Test Harware/Firmware environment
Our current test dev board is a ESP8266 Lolin D1 Mini with a SHT30 Temp & Humidity shield, also from Lolin, 
I expect to start testing other combinations of boards and sensors soon. 
Instructions for other board/sensor combinations are welcome. 

I've also tested on a Lolin C3 Pico (ESP32-C3), but do not test every git push on that board. 


#### Arduino IDE basics - skip if you know what you are doing
* In Arduino IDE 
* Open `~/frugal-iot/`
* Tools -> Upload Speed -> 460800
* Tools -> Manage Libraries -> Search for, and install:
  * Libraries used by almost all cases
    * ESP-WiFiSettings by Juerd Waalboer
    * MQTT by Joel Gaehwiler
  * Libraries only needed if you use specific sensors. 
    * SHT85 by Rob Tillaart; (SENSOR_SHT85_WANT to enable SHT3X or SHT85 series sensors)
    * DHTNEW also by Rob Tillaart (SENSOR_DHT_WANT to enable)
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
* Copy _local-template.h to _local.h. For now leave the organization as "dev" unless you talk to us. 
  Put the MQTT server, userid and password, and which sensors you have on the board.
* Check _configuration.h t - for now this is a good place to set how much debugging you want.  TODO maybe move to _local.h
* Compile and Flash to your dev board
* On a Wifi device such as a phone
  * Connect to the wifi node of the board which will have a SSID like esp8266-12345
  * Configure the Wifi SSID and Password of your router,
  * Pick the name for your project (use the same one for all your boards), and also give your board a name and description
    * Note for the moment [issue #63](https://github.com/mitra42/frugal-iot/issues/63) this should be `Lotus Ponds`
  * Click SAVE then RESTART
* The device should now connect to the mqtt server at naturalinnovation.org

If you get the configuration wrong, you'll need (for now - part of issue#22) to 
* erase the settings - on Arduino IDE this is Tools->Erase->All Flash
* recompile/flash.

Open a browser to http://naturalinnovation:8080 
- note this will move to a https address with its own domain name 

NOTE For now - index.html is hard coded to only worth with the "Lotus Ponds" project,
I expect this to change very soon. See [issue #63](https://github.com/mitra42/frugal-iot/issues/63)

### Server development

Because of the demo servers on naturalinnovation.org, it is perfectly possible to develop the firmware without running your own servers.
But if you want to do server or client development ...

#### Install prerequisites
* Check you have node & npm & http-server installed `node -v` If not then you'll need nodejs from (nodejs.org)

### Install Repo
* clone this repo, and `cd` into wherever it is installed.
* For the purpose of these instructions we will assume that is `~/frugal-iot` on your laptop;  but it could be anywhere.
* cd ~/frugal-iot/html`
* `npm install` # Adds the libraries needed for the html
* node ./Main.js 

Then open a browser pointing at for example localhost:8080 





