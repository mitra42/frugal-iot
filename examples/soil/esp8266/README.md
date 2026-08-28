# ESP8266 in the Arduino IDE - one manual step

`soil.ino.globals.h` in this folder holds the settings (pins, addresses, debug flags) that
PlatformIO takes from `platformio.ini`, for the ESP8266 boards this example supports.

**Using PlatformIO, or an ESP32 board? Ignore this folder entirely** - `platformio.ini` and
`platform.h` already cover you.

**Building for an ESP8266 in the Arduino IDE?** Move the file up one level, so it sits next to
`soil.ino`:

    Sketch > Show Sketch Folder, then drag esp8266/soil.ino.globals.h into the folder above.

Then compile. Skip the step and the build stops with a message pointing back here - deliberately,
because the alternative was a build that succeeded on the library's built-in defaults rather than
this example's settings, which for most boards means the wrong pins. The check is in
`system/frugal.cpp`; the file carries a `FRUGAL_IOT_GLOBALS_FOUND` define that satisfies it.

## Why it is not just left there in the first place

The ESP8266 core only picks this file up under the exact name `<sketch>.ino.globals.h` - that is
`{build.project_name}.globals.h` in the core's `platform.txt`, and `build.project_name` includes
the `.ino`. It is also the only mechanism there is on ESP8266: unlike ESP32, that core does not put
the sketch directory on the include path, so `platform.h` cannot be reached from library sources -
nor, as it turns out, from the sketch itself.

But arduino-cli - and so the Arduino IDE, which embeds it - refuses to recognise a folder as a
sketch at all if it contains any file named `<sketch>.ino*` besides the sketch itself. The example
then disappears from **File > Examples** completely. Verified on arduino-cli 1.5.1: a folder is
listed with `foo.globals.h` or `bar.ino.globals.h` in it, and not listed with `foo.ino.globals.h`.

So the required name and a listed example are mutually exclusive, and the file is parked here.
