# Setting up Frugal IoT demo HTML

*Important* At this time, the demo only demonstrates HTML access to MQTT
it is not yet connected to the sensor node development
And in fact in most situations they won't see the laptop unless its 
IP address is configured in `_configuration.h`

To get this demo running required a lot of fiddling
with mosquitto - this is what worked for me on a Mac

I welcome improvements for other platforms and setups.
#### Install javascript libraries
You will need npm [See instructions on npmjs](https://docs.npmjs.com/downloading-and-installing-node-js-and-npm)

Then in this directory
```aiignore
npm install
```
It should install the mqtt library and a package I wrote to simplify web components. 
#### Setting up Mosquitto on my Mac
Install homebrew if not already available

```brew install mosquitto```
There is a configuration file and password file here.
Mosquitto is picky about its config file, it won't work without either the passwords setup or `allow_anonymous true`
There is a demo password file here 
with a single user `public` with password `public`, 
it can be configured with e.g. 
```
mosquitto_password -c ./mosquitto_passwords public
```
(only use `-c` when creating file first time)

Then run it with
```
/opt/homebrew/opt/mosquitto/sbin/mosquitto -c mosquitto.conf
```
And you should see a log file like
```
1728120137: mosquitto version 2.0.18 starting
1728120137: Config loaded from mosquitto.conf.
1728120137: Opening ipv6 listen socket on port 1883.
1728120137: Opening ipv4 listen socket on port 1883.
1728120137: Opening websockets listen socket on port 9012.
1728120137: mosquitto version 2.0.18 running
```
#### Setup http server
```aiignore
brew install http-server
```
Then 
```
http-server
```
Will give a local http-server on port 8080

The web browser should be able to navigate to `http://localhost:8080`
#### Connect local server node to server

Find the IP of your local server

Edit that into `SYSTEM_MQTT_SERVER` in `_configuration.h`
Edit the `SYSTEM_MQTT_PASSWORD` and `SYSTEM_MQTT_PASSWORD` in the same file
Flash the node. 

You should see the connection both in the Serial Monitor of the node, 
and in the window where you started `mosquitto`

#### Connect your web browser
Browse to `https://localhost:8080` or wherever your local http server is.

This should also show up in the `mosquitto` console.

