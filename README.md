# Frugal IoT
A platform to enable affordable sensor networks

The best source of information is the [Wiki here](https://github.com/mitra42/frugal-iot/wiki)

Frugal-IoT is a library, available on both PlatformIO and Arduino repositories. 

It is intended to run with a backend server in the [frugal-iot-server](https://github.com/mitra42/frugal-iot-server) repo; and with [frugal-iot-client](https://github.com/mitra42/frugal-iot-client) which works on mobile or laptop.  There is also a server-side logging tool, that drives the graphs in the client, or can output (live) to Firebase and Google Sheets.

It enables really simple applications to include a full range of functionality, such as wireless & MQTT reconnections; LoRa meshing; access to functions like time; various sleep modes. 

It works on an expanding variety of dev boards; with an expanding range of sensors, and in an expanding range of languages.

Your organization's enrolment secret goes in a file that is never committed, made from one of
the [`templates/`](templates/): `platformio-secrets-local.ini` beside `platformio.ini` for
PlatformIO, or `platform-secrets-local.h` beside the `.ino` for the Arduino IDE. Each template says
how. Without it a node still builds but is refused until an administrator approves it.

If a sketch will not build, download [`scripts/frugal_iot_doctor.py`](scripts/frugal_iot_doctor.py)
and run `python3 frugal_iot_doctor.py` in your sketch or PlatformIO project folder. It checks for the
usual problems - missing libraries, two copies of the library, a `platformio.ini` copied from an
example - says how to fix each one, and its output is what to paste when asking for help.

But ... please see the [Wiki here](https://github.com/mitra42/frugal-iot/wiki), which - unlike this README is updated regularly.  

CLAUDE.md can also be a good place for more structured description. 
