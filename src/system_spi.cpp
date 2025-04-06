/* Frugal IoT System SPI 
 * Support for generic SPI interface
 * 
 * In <SPI.h> labeled SCK MISO MOSI SS
 * https://electronics.stackexchange.com/questions/345393/spi-naming-sdi-mosi-confusion
 * MOSI on dev board connects to SDO on sensor
 * MISO on dev board connects to SDI on sensor
 *
 * D1 Mini supports one HSPI (Hardware SPI) which uses SCLK=14 (D5), MISO=12 (D6), MOSI=13 (D7), CS=15 (D8)
 * I've seen notes that if D8 is pulled high it won't boot - haven't tested yet.
 */
#include "_settings.h"
#ifdef SYSTEM_SPI_WANT
#include <SPI.h>
#include "system_spi.h"

System_SPI::System_SPI(uint8_t cs,  uint64_t clock) : cs(cs), clock(clock) {}

void System_SPI::initialize() {
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);
  SPI.begin(); // Uses default pins for ESP8266 Hardware SPI (HSPI) MISO=12, MOSI=13, SCLK=14
  SPI.setBitOrder( MSBFIRST ); // Maybe needs to be parameter if its ever not MSBFIRST
  SPI.setClockDivider( clock );
}
void System_SPI::send(uint8_t cmd) {
    // This is set for MS5803, parameterize if it is different for other systems
    SPI.setDataMode( SPI_MODE3 );
    digitalWrite( cs, LOW );
    SPI.transfer( cmd );
    delay( 10 );
    digitalWrite( cs, HIGH );
    delay( 5 );
}
uint32_t System_SPI::read(uint8_t cmd, uint8_t bytes) {
    SPI.setDataMode( SPI_MODE3 );
    digitalWrite( cs, LOW );
    SPI.transfer( cmd );
    uint32_t result = 0;
    for (uint8_t i = 0; i < bytes; i++) {
        result = result << 8;
        result |= SPI.transfer( 0x00 ); // read one byte
        delay(10);
    }
    digitalWrite( cs, HIGH );
    delay(10);
    Serial.print("XXX read");  Serial.println(result); 
    return result;
}
uint16_t System_SPI::read16(uint8_t cmd) {
  return (uint16_t) read(cmd, 2);  
}
// readAndSet should be defined in subclass and call these functions

#endif //SYSTEM_SPI_WANT
