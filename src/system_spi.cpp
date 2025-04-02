#include "_settings.h"
#ifdef SYSTEM_SPI_WANT
#include <SPI.h>
#include "system_spi.h"

System_SPI::System_SPI(uint8_t cs,  uint64_t clock) : cs(cs), clock(clock) {}

void System_SPI::initialize() {
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);
  SPI.begin();
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
    }
    digitalWrite( cs, HIGH );
    return result;
}
uint16_t System_SPI::read16(uint8_t cmd) {
  return (uint16_t) read(cmd, 2);  
}
// readAndSet should be defined in subclass and call these functions

#endif //SYSTEM_SPI_WANT
