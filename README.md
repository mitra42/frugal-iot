# frugal-iot
A platform to enable affordable sensor networks

For now see the Google Doc: [Frugal IoT - enabling low cost sensor networks](https://docs.google.com/document/d/1hOeTFgbbRpiKB_TN9R2a2KtBemCyeMDopw9q_b0-m2I/edit?usp=sharing)
Scroll down to the bottom for links to other docs. 

Also see the subdirectory ./docs

### Installation and testing POC

This is still at POC stage, so installation is non-optimal, and only tested on Macs - assistance appreciated to improve that ! 

Please recheck these instructions as they will probably change often as code evolves to an easier POC setup.

Our current test dev board is a ESP8266 Lolin D1 Mini with a SHT30 Temp & Humidity shield, also from Lolin, 
I expect to start testing other combinations of boards and sensors soon

#### Install software
* fork this repo
* Check you have node & npm & http-server installed `node -v` If not then you'll need nodejs from (nodejs.org)
* cd `html`
* `npm install` # Adds the libraries needd for the html

#### Install mosquitto
* Note, at some point soon we'll have a MQTT server on a known location - watch issue#30.d
* On a mac
  * check you have homebrew installed `brew -v`
  * `brew install mosquitto` worked for me with no problems. 
  * `cd html`
  * `mosquitto -c mosquitto.conf` # Run a mqtt server (which the sensor node will use) and websockets (which the browser uses) on default ports

#### Run http server
* cd `html`
* http-server   
* Not down the IP address it reports (not the `127.0.0.1`, the other one) - this will also be the address of the mqtt server
* open (http://localhost:8080) in your browser

#### Installing on a dev board

* Open this directory in Arduino
* Edit _configuration.h to match your system and functionality required for example uncomment SENSOR_SHT85_WANT to enable that functionality
* Compile and Flash to your dev board (ESP8266 tested, ESP32 should work)
* On a Wifi device such as a phone
* Connect to the wifi node of the board which will have a SSID like esp8266-12345
* Configure the Wifi SSID and Password of your router, 
* Configure the mqtt server address (address of your laptop noted when setting up the http server)
* SAVE and RESTART
* It should now connect to the mqtt server 

If you got the configuration wrong, you'll need (for now - part of issue#22) to 
* erase the settings - on Arduino IDE this is Tools->Erase->All Flash
* recompile/flash.




