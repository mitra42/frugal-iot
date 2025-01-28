# OTA - Over The Air updates

The Frugal-IoT project supports Over The Air updates.

This means, developers can (with one temporary restriction) compile code, upload to the server, and expect nodes 
in the field to update themselves. 

### How does OTA work at a high level ?
* A new version of the code is built in the IDE
* That code is uploaded to the server either naturalinnovation.org or any other coded into the `organization`'s nodes
* On startup, or once an hour, the nodes will query the server.
* If a matching package is found, that package is downloaded and the device restarts.

### What options are needed to support OTA
In your _locals.h file you'll need something like. 
```
#define SYSTEM_OTA_WANT   // Compile OTA support 
#define SYSTEM_OTA_KEY "sonoff-r2"  // Tell the device which package to download
#define SYSTEM_OTA_MS 3600000 // Optionally tell the device how often to check for updates - default is 1-hour (360000)
#define SYSTEM_OTA_SERVERPORTPATH "https://frugaliot.naturalinnovation.org/ota_update/" // Optionally tell the node somewhere else to look for updates
```
Note that a device built without OTA support will need manually flashing (via USB) with a OTA supporting version. 

### How do I build a binary

#### On Arduino IDE

The function is under Sketch/Export Compiled Binary
The binary file is put in e.g. `build/esp8266.esp8266.d1_mini/frugal-iot.ino.bin`

#### On PlatformIO or other platforms

This is waiting for a volunteer to try and do this and document it! 

### Directory hierarchy on the server 

There is a directory hierarchy on the server and it is important to put the file in the right place. 

`ota/dev/lotus/esp8266-95a754/frugal-iot.ino.bin` 
would be a binary destined for a specific node 

`ota/dev/lotus/sonoff-r2/frugal-iot.ino.bin` 
would be a binary destined for any device with an `OTA_KEY` of `sonoff-r2` in the `lotus` project. 

`ota/dev/esp8266-sht/frugal-iot.ino.bin` 
would be a binary destined for any device with an `OTA_KEY` of `esp8266-sht` in any project of the `dev` organization.

### Uploading 

See [#Issue#37](https://github.com/mitra42/frugal-iot/issues/37) for updates, 
but we need to have a place for authenticated developers at each organization 
to be able to upload directly to a directory and get a fast turnaround to the device. 

This requires implementing some security on the server, which will also protect different organization's data.

For now - post a comment in the OTA topic, or email me, and I'll setup a secure way to get new binaries from you. 

### OTA and picking the right binary

Each device, either at restart, or periodically (default once an hour), 
will check the server, by default it will send a HTTPS request for:
`frugaliot.naturalinnovation/ota_update/dev/lotus/esp8266-95a754/esp8266-sht`. 

The server finds an appropriate binary, either for that specific node, or for that type of node,
(`esp8266-sht` in the example above).

If a binary is found, the server checks the MD5 of the currently running binary (which is in the HTTP-request) 
and if its not the same, delivers a new binary. 

Note that we do not currently check the version number,
that may change, see [#Issue#37](https://github.com/mitra42/frugal-iot/issues/37)

### How to use this for development in practice. 

With the server being both flashable on a USB *and* via OTA, there needs to be a little care taken to avoid a situation where a node
is flashed over USB and then `updates` itself to the previous version.  I recommend two workflows, either:

#### Flash-first
* While developing
  * Change `SYSTEM_OTA_KEY` to something dummy - like `xxx`. 
  * Flash each version you are testing via USB.
  * The node will try, and fail, to fetch and update from the OTA server.
* When you are satisfied with the version
  * change the `SYSTEM_OTA_KEY` back to the real value
  * Compile a new binary and upload to the server
  * Flash it to the node. 

#### Upload-centric (including remote)
* While developing
  * Upload each build to a node-specific directory e.g. `ota/dev/lotus/esp8266-95a754/frugal-iot.ino.bin`
  * The node should update within an hour (or if you power-cycle it).
* When you are satisfied with the version
  * Move the bin to the generic and update all matching nodes e.g. `ota/dev/lotus/esp8266-95a754/esp8266-sht/frugal-iot.ino.bin`
  * The node will only self-update if this is a new version. 

