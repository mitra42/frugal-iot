/* Frugal IoT - LoRa controller 
 *
 *  TODO - Its a work in progress - see https://github.com/mitra42/frugal-iot/issues/137
 * 
 */

#include "_settings.h"

#ifdef SYSTEM_LORA_WANT
#include "system_lora.h"

#if definded(TTGO_LORA_SX127X_V1)
  #define LORA_SCK 5
  #define LORA_MISO 19
  #define LORA_MOSI 27
  #define LORA_SS 18
  #define LORA_RST 14 // Note 23 on V2
  #define LORA_DIO0 26
#elif defined(TTGO_LORA_SX127X_V2) // V3 is same as V2
  #define LORA_SCK 5
  #define LORA_MISO 19
  #define LORA_MOSI 27
  #define LORA_CS 18 // tutorial caslls it SS
  #define LORA_RST 23 // Note 14 on V1 and 23 in tutorial comments & v21new and 12 in /Users/mitra/Library/Arduino15/packages/esp32/hardware/esp32/3.2.0/variants/ttgo-lora32-v2/pins_arduino.h
  #define LORA_DIO0 26 //Tutorial  calls this DIO0
#else
  #error "Unsupported LORA configuration. Please define either TTGO_LORA_SX127X_V1 or TTGO_LORA_SX127X_V2. or define new BOARD"
#endif

// TODO note that each board only does specific bands: 
// SX1278 does 433MHz 144-148MHz suitable for Asia
// SX1276 does 868MHz (where?);  915MHz (NAmerica and Australia); 923MHz (where?);



//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 915E6
  /* Bob Rader added in comment https://randomnerdtutorials.com/ttgo-lora32-sx1276-arduino-ide/#comment-426149
#define Band 433E6 // Set Band or Frequency in Hz
#define SF 10 // Set Spreading Factor
#define BW 125E3 // Set Bandwidth
#define CR 5 // Set Coding Rate
#define Preamble 255 // Set Preamble
#define SyncWd 0x12 // Set Sync Word
*/



System_LoRa::System_LoRa() : Frugal_Base() {
    // Constructor code here, if needed
}
void System_LoRa::setup() {
    // Setup code for LoRa system
}
void System_LoRa::infrequent() {
    // Code for infrequent tasks, e.g., every 10 seconds
}
void System_LoRa::periodic() {
    // Code for periodic tasks, e.g., every minute
}
void System_LoRa::frequent() {
    // Code for frequent tasks, e.g., every 10 ms
}
#endif // SYSTEM_LORA_WANT