#!/usr/bin/env python3
"""Draws docs/wiring_d1_mini.png (and .svg) - run from anywhere.
Uses the wiring-diagrams skill in ~/.claude/skills/wiring-diagrams. Based on kopernik_soil's."""
import os, sys
sys.path.insert(0, os.path.expanduser("~/.claude/skills/wiring-diagrams/scripts"))
from wiring import Diagram
HERE = os.path.dirname(os.path.abspath(__file__))
CHECK = "Check soil sensor connections, it varies from brand to brand"

# The probe points right, so its header reads GND, VCC, AOUT top to bottom, and sits level with G
# so ground runs straight across. A0 and 3V3 are both on the far side: they go under the board,
# 3V3 (nearer the corner) on the inner track to VCC, A0 on the outer one to AOUT - no crossings.
d = Diagram(1240, 860, "soil on a Wemos D1 mini",
            "The SHT temperature and humidity sensor is not shown - see the table in README.md")
b = d.board("wemos_d1_mini", x=150, y=140, width=360)
gy = b["G"][1]
p = d.soil_probe(x=680, y=gy - 22, facing="left")
d.wire(b["G"], p["GND"], "ground")
d.wire(b["3V3"], p["VCC"], "power", via=["x=126", "y=680", "x=600"])
d.wire(b["A0"], p["AOUT"], "signal", via=["x=106", "y=720", "x=630"], label="A0 reads the probe", label_at=2)
d.note(p.box[0] + 20, p.box[1] + p.box[3] + 34, CHECK, size=13, bold=True)
print(d.save(os.path.join(HERE, "wiring_d1_mini")))
