// Heltec WiFi LoRa 32 V4 pin definitions.
// Source: https://github.com/paulmarx-dev/Heltec-ESP32-LoRa-V4-on-PlatformIO
// and https://wiki.heltec.org/docs/devices/open-source-hardware/esp32-series/three-platform/PlatformIO/

#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define USB_VID 0x303a
#define USB_PID 0x1001

static const uint8_t LED_BUILTIN = 35;
#define BUILTIN_LED  LED_BUILTIN
#define LED_BUILTIN  LED_BUILTIN

static const uint8_t TX = 43;
static const uint8_t RX = 44;

// Default I2C (user-facing header pins, not the OLED)
static const uint8_t SDA = 4;
static const uint8_t SCL = 3;

// SPI (LoRa radio)
static const uint8_t SS   = 8;
static const uint8_t MOSI = 10;
static const uint8_t MISO = 11;
static const uint8_t SCK  = 9;

// ADC-capable pins
static const uint8_t A0  = 1;
static const uint8_t A1  = 2;
static const uint8_t A2  = 3;
static const uint8_t A3  = 4;
static const uint8_t A4  = 5;
static const uint8_t A5  = 6;
static const uint8_t A6  = 7;
static const uint8_t A7  = 8;
static const uint8_t A8  = 9;
static const uint8_t A9  = 10;
static const uint8_t A10 = 11;
static const uint8_t A11 = 12;
static const uint8_t A12 = 13;
static const uint8_t A13 = 14;
static const uint8_t A14 = 15;
static const uint8_t A15 = 16;
static const uint8_t A16 = 17;
static const uint8_t A17 = 18;
static const uint8_t A18 = 19;
static const uint8_t A19 = 20;

// Touch-capable pins
static const uint8_t T1  = 1;
static const uint8_t T2  = 2;
static const uint8_t T3  = 3;
static const uint8_t T4  = 4;
static const uint8_t T5  = 5;
static const uint8_t T6  = 6;
static const uint8_t T7  = 7;
static const uint8_t T8  = 8;
static const uint8_t T9  = 9;
static const uint8_t T10 = 10;
static const uint8_t T11 = 11;
static const uint8_t T12 = 12;
static const uint8_t T13 = 13;
static const uint8_t T14 = 14;

// Onboard peripherals
static const uint8_t Vext     = 36; // external power rail enable (active LOW)
static const uint8_t LED      = 35;

// OLED display (SSD1306, I2C)
static const uint8_t RST_OLED = 21;
static const uint8_t SCL_OLED = 18;
static const uint8_t SDA_OLED = 17;

// LoRa radio (SX1262)
static const uint8_t RST_LoRa  = 12;
static const uint8_t BUSY_LoRa = 13;
static const uint8_t DIO0      = 14;

// GNSS connector — not standard pin names, defined here for reference.
// Use SENSOR_GPS_* build flags in platformio.ini rather than these names
// directly, so the sensor driver stays board-independent.
//   GNSS_RX    = 39   (ESP32 receives from module TX)
//   GNSS_TX    = 38   (ESP32 transmits to module RX)
//   VGNSS_CTRL = 34   (active LOW — set LOW to enable module power)
//   GNSS_WAKE  = 40   (active HIGH — set HIGH to keep module awake)
//   GNSS_RST   = 42   (active LOW reset — set HIGH to release)

#endif /* Pins_Arduino_h */
