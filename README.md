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
Please note that from our perspective, if you cannot understand how to install that is probably a problem with 
the quality of our documentation ! 

### Test environment
Our current test dev board is a ESP8266 Lolin D1 Mini with a SHT30 Temp & Humidity shield, also from Lolin, 
I expect to start testing other combinations of boards and sensors soon. 
Instructions for other board/sensor combinations are welcome. 

I've also tested on a Lolin C3 Pico (ESP32-C3), but do not test every git push on that board. 

### Install prerequisites
* Check you have node & npm & http-server installed `node -v` If not then you'll need nodejs from (nodejs.org)

### Install Repo
* clone this repo, and `cd` into wherever it is installed.
* For the purpose of these instructions we will assume that is `~/frugal-iot` but it could be anywhere.
* cd ~/frugal-iot/html`
* `npm install` # Adds the libraries needd for the html
* cd ~/frugal-iot

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
* Copy _local-template.h to _local.h and edit to put your organization, MQTT server, userid and password, and which devices you have on the board.
* Check _configuration.h t - for now this is a good place to set how much debugging you want.  TODO maybe move to _local.h
* Compile and Flash to your dev board
* On a Wifi device such as a phone
  * Connect to the wifi node of the board which will have a SSID like esp8266-12345
  * Configure the Wifi SSID and Password of your router,
  * Pick the name for your project (use the same one for all your boards), and also give your board a name and description
  * Click SAVE then RESTART
* The device should now connect to the mqtt server at naturalinnovation.org

If you get the configuration wrong, you'll need (for now - part of issue#22) to 
* erase the settings - on Arduino IDE this is Tools->Erase->All Flash
* recompile/flash.

### For now run http server locally (but watch issue#41 for server development)
* At some point the html code will be someewhere you can run it, but for now - run on your own machine
* cd `~/frugal-iot/html`
* edit index.html so that it connects to your project (as configured on your board)
  * for example if your project name is `My home` then the mqtt-projct should point at `topic="dev/Lotus Ponds/"`
* Run `http-server` which will start a trivial http-server on port 8080
* open (http://localhost:8080) in your browser




