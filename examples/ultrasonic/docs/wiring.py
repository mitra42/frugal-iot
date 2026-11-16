#!/usr/bin/env python3
"""Draws docs/wiring_sr04m2_c3_oled.png (and .svg) - run from anywhere.
Uses the wiring-diagrams skill in ~/.claude/skills/wiring-diagrams."""
import os, sys
sys.path.insert(0, os.path.expanduser("~/.claude/skills/wiring-diagrams/scripts"))
from wiring import Diagram
HERE = os.path.dirname(os.path.abspath(__file__))

# SR04M-2 in trigger/echo mode, powered from 5V, on the C3 OLED 72x40 board ([env:c3_oled_72x40]:
# TRIG 21 = the pin printed TX, ECHO 20 = the pin printed RX).
# Board, down its left edge: V5, GD, V3, RX, TX. Module header, top to bottom: 5V, RX, TX, GND.
# GND reaches the bottom of the header from the second pin, and trigger/echo swap over, so three
# crossings are unavoidable; GND runs as a bus underneath, which also takes the 2k.
DY = -60  # everything up, to close the gap under the subtitle
d = Diagram(1240, 740, "Ultrasonic: SR04M-2 on the ESP32-C3 OLED 72x40 board",
            "TX is GPIO21 (trigger out), RX is GPIO20 (echo in) - as [env:c3_oled_72x40] sets them in platformio.ini")
b = d.board("esp32c3_oled_72x40", x=820, y=150 + DY, width=380)
pitch = b["GD"][1] - b["V5"][1]
m = d.ultrasonic_module(x=156, y=233.6 + DY, pitch=pitch)
d.wire(b["V5"], m["5V"], "power", via=["x=740"])
d.wire(b["TX"], m["RX"], "signal", via=["x=700", "y=" + str(m["RX"][1])], label="TX triggers it", label_at=2)
# Echo: module TX -> 1k -> tap -> board RX; tap -> 2k -> GND
ty = m["TX"][1]
r1 = d.resistor(m["TX"][0], ty, 610, ty, "1k", bands=("#8b4513", "#000", "#d62728", "#c9a227"))
tap = d.junction(610, ty)
d.wire(tap, b["RX"], "signal2", via=["x=650"])
bus = 650 + DY
r2 = d.resistor(610, ty, 610, bus, "2k", bands=("#d62728", "#000", "#d62728", "#c9a227"))
d.wire(m["GND"], b["GD"], "ground", via=["x=505", "y=" + str(bus), "x=790"])
d.junction(610, bus)
d.note(160, 214 + DY, "V5 is 5V only while the board is powered from USB or a 5V supply", size=13, colour="#d62728")
d.note(480, 694 + DY, "1k and 2k make the divider: the module's 5V echo arrives at RX as 3.3V", size=13, colour="#1f77b4")
print(d.save(os.path.join(HERE, "wiring_sr04m2_c3_oled")))
