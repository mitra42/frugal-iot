# Frugal IoT
A platform to enable affordable sensor networks

Frugal-IoT is a library, on both PlatformIO and Arduino, for ESP32 and ESP8266 sensor nodes. It brings WiFi setup from a phone, MQTT, LoRa meshing, time, sleep modes, and over-the-air updates. Nodes report to [frugal-iot-server](https://github.com/mitra42/frugal-iot-server), with a dashboard ([frugal-iot-client](https://github.com/mitra42/frugal-iot-client)) on phone or laptop, and a logger ([frugal-iot-logger](https://github.com/mitra42/frugal-iot-logger)) that drives the graphs and can output to Google Sheets or Firebase.

### Start here
* **Flash an example onto a board** - [Getting Started](https://github.com/mitra42/frugal-iot/wiki/Getting-Started). Each folder in [`examples/`](examples/) has a README with the steps for PlatformIO and Arduino IDE.
* **Build your own node** - copy the closest example folder, including its `platformio.ini`, and edit the sketch. See [getting setup for development](https://github.com/mitra42/frugal-iot/wiki/Development-%E2%80%90-getting-setup).
* **Everything else** - the [Wiki](https://github.com/mitra42/frugal-iot/wiki), which is updated more often than this README. [CLAUDE.md](CLAUDE.md) describes the library's structure.

### If stuck
* Won't build: download [`scripts/frugal_iot_doctor.py`](scripts/frugal_iot_doctor.py) and run `python3 frugal_iot_doctor.py` in your sketch or project folder. It names the usual problems and how to fix each; paste its output when asking for help.
* Node refused by the dashboard: build it with your organization's enrolment secret - see [Next steps](https://github.com/mitra42/frugal-iot/wiki/Getting-Started#next-steps).
* [Open an issue](https://github.com/mitra42/frugal-iot/issues/new).
