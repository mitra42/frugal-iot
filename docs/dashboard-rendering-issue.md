# Dashboard: Control and Actuator sections appear empty when expanded

## Problem

When viewing a node on the dashboard, sections for Controls (e.g. `climate`) and Actuators (e.g. `heating`, `humidifier`) appear in the node listing but show no content when expanded. Sensor sections (e.g. `SHT`) render correctly with colored bars and values.

## Expected behavior

- **Actuators** (`heating`, `humidifier`): Should show the boolean on/off state
- **Controls** (`climate`): Should show input values (temperature, humidity readings and setpoints) and output states (temp_out, humidity_out)

## Root cause

The dashboard web components (`<mqtt-topic>`) render float sensor outputs as colored bars using `min`, `max`, `color`, and value metadata. Controls and actuators publish different data patterns that the dashboard doesn't handle:

1. **Actuator inputs** are `INbool` (on/off) — no `min`/`max` range, so no bar renders
2. **Control inputs** (`INfloat`) publish `min`/`max`/`color` metadata on discover, but wired input values (received from sensors) are internal state and not re-published to MQTT under the control's topic
3. **Control outputs** (`OUTbool`) are boolean — same issue as actuators

## MQTT topics published (verified working)

```
dev/developers/esp8266-a7c0c4/climate/temperature/min=-40.0
dev/developers/esp8266-a7c0c4/climate/temperature/max=80.0
dev/developers/esp8266-a7c0c4/climate/temperature/color=black
dev/developers/esp8266-a7c0c4/climate/humidity_hysteresis=5.0
dev/developers/esp8266-a7c0c4/climate/humidity_hysteresis/min=0.0
dev/developers/esp8266-a7c0c4/climate/humidity_hysteresis/max=50.0
dev/developers/esp8266-a7c0c4/heating/on=true
dev/developers/esp8266-a7c0c4/humidifier/on=true
```

The data reaches MQTT correctly — the dashboard just doesn't render it.

## Possible fixes

- Add boolean rendering support (toggle/indicator) to `<mqtt-topic>` for `INbool`/`OUTbool`
- Have controls re-publish wired input values under their own topic so the dashboard can display them
- Use distinct colors instead of `"black"` for control inputs/outputs so bars are visible

## Context

Discovered while testing `Control_Climate` from #195. The control logic works correctly (verified via `CONTROL_CLIMATE_DEBUG` serial output), this is purely a dashboard rendering issue.
