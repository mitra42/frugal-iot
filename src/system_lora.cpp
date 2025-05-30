/* Frugal IoT - LoRa controller 
 *
 *  TODO - Its a work in progress - see https://github.com/mitra42/frugal-iot/issues/137
 * 
 */

#include "_settings.h"

#ifdef SYSTEM_LORA_WANT
#include "system_lora.h"
#include <SPI.h>
#include <LoRa.h> // LoRa library for ESP32

#if defined(TTGO_LORA_SX127X_V1)
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
// SX1278 does BAND 433MHz (Europe) 144-148MHz suitable for Asia
// SX1276 does 868MHz (Europe);  915MHz (NAmerica, SAmerica & Australia); 923MHz (Australia and Asia);
// Code says: 433E6 for Asia; 866E6 for Europe (unclear if 868 or 433); 915E6 (NAmerica); 

/* Bob Rader added in comment https://randomnerdtutorials.com/ttgo-lora32-sx1276-arduino-ide/#comment-426149
#define SF 10 // Set Spreading Factor
#define BW 125E3 // Set Bandwidth
#define CR 5 // Set Coding Rate
#define Preamble 255 // Set Preamble
#define SyncWd 0x12 // Set Sync Word

Not currently used in this code
*/

System_LoRa* lora;

System_LoRa::System_LoRa() : Frugal_Base() {
    // Constructor code here, if needed
}
void System_LoRa::setup() {
    // Setup code for LoRa system
      //SPI LoRa pins
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  //setup LoRa transceiver module
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ); 
  if (!LoRa.begin(SYSTEM_LORA_BAND)) {
    Serial.println("❌ Starting LoRa failed!");
  }
  /* Bob Rader added in comment https://randomnerdtutorials.com/ttgo-lora32-sx1276-arduino-ide/#comment-426149
    LoRa.setSpreadingFactor(SF); // 6-12
    LoRa.setSignalBandwidth(BW); // 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3,
    // 41.7E3, 62.5E3, 125E3, & 250E3
    LoRa.setCodingRate4(CR); // 5 or 8
    LoRa.setPreambleLength(Preamble); // 5 to 65535
    LoRa.setSyncWord(SyncWd); // byte val to use as sync word
  */
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