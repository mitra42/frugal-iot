# Pins usage on various boards

## D1 Mini compatable
We are using a number of boards compatible with the D1 mini shields. These include

* Lolin D1 Mini itself - ESP8266 
* Lolin C3 Pico - ESP32
* Lolin S2 (?) - ESP8266

This doc is to help align the thinking around sensor shields to avoid pin clashes making it harder to use 
various combinations.

Starting with the top lect of the board as looking down on the shield, and running down that side,
and then down the right hand side.

 D1 | C3| used by
----|---|--------
RST |EN | 
A0  |3  | Only analog on D1, so should be used by soil sensor
    |   | Vbat when pads shorted 
D0  |2  | Always reads 4095 on C3
D5  |1  | Works on C3 for analog
D6  |0  | Works on C3 for analog
D7  |4  | Works on C3 for analog currently using for Soil but not fixed
D8  |5  | Gets error message if used as analog on C3
3V3 |3V3|
    |   | Right hand side
TX  |TX |
RX  |RX |
D1  |10 | Always reads 0 on C3 - used by Relay shield
D2  |8  | Always reads 0 on C3
D3  |7  | Always reads 0 and seems connected to LED
D4  |6  | DHT22 shield
GND |GND|
5V  |5V |

Open questions
* which pins are C3 and D1 using for LED
* Any mapping between other designations e.g. GPIOX and the physical pins
* Which pins are SHT shields using for I2C
