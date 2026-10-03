# Sleep redesign: hardware test (branch `232-lm-sleep`)

This tests the code as of phase 4 (`7fe1a73` plus the MQTT changes after it). If a step fails,
the "If it fails" column says which earlier commit to go back to:

| Commit | Phase |
|---|---|
| `2150277` | 1: cycle timing, `okToSleep`, millis rollover fixes |
| `b0b2dee` | 2 + 3: `Power_Lora` dormant state, `WIFI_PAUSED`, LoRaMesher `OnSleep` and the loop lock |
| `7fe1a73` | 4: MQTT, captive and gateway refusals |

## Setup (both parts)

In `platformio.ini`, under `build_flags_frugaliot`, uncomment:

```ini
    -D SYSTEM_POWER_DEBUG
    -D SYSTEM_WIFI_DEBUG
    -D SYSTEM_MQTT_DEBUG   ; optional - noisy, but shows incoming messages for A3
```

Serial messages you will be watching for:

| Message | Meaning |
|---|---|
| `Power: sleep refused by <id>` | module `<id>` refused sleep, and it will be asked again next `loop()` |
| `Power Management: preparing` | `prepare()` ran: sensors off, WiFi paused if the mode does that |
| `Sleeping for <ms>` | light sleep for the rest of the cycle (less than `cycle - wake` if the sleep was delayed) |
| `Waking for<ms>` | `recover()` ran |
| `WIFI Paused` / `WIFI Disconnected` | WiFi moved into or out of `WIFI_PAUSED` |
| `LoraMesher sleep rq: ... - allowed/refused` | one LoRaMesher sleep slot asking for a light sleep |
| `LoraMesher wakeUp` | end of a run of allowed sleep slots |

Throughout, time the readings: on the dashboard, or with the serial monitor's timestamps.

## Part A: C3 Pico (sht example, env `c3_pico`)

Change only the `configure_power(...)` line in `src/main.cpp` for each step.

| # | `configure_power` | Do | Expect | If it fails |
|---|---|---|---|---|
| A1 | `Power_Loop, 30000, 30000` | Run 2 min | Readings every 30 s. No `preparing`. | phase 1 |
| A2 | `Power_Light, 60000, 20000` | Run 5 min | After ~20 s: `preparing`, then `Sleeping for ~40000`. Readings **exactly** 60 s apart, with no drift over 5 cycles. After each wake: `WIFI Paused` → `WIFI Disconnected` → reconnect, with MQTT connecting on its own pass rather than stalling inside `recover()`. | phase 1 (timing), phase 2 (WiFi) |
| A3 | as A2 | While it is asleep, change a setting from the dashboard (e.g. a min/max) | At the next wake the change arrives (`MQTT incoming`) **before** `preparing`. You may see `sleep refused by mqtt` for up to ~2 s after the last incoming message. | phase 4 |
| A4 | as A2 | Join the node's AP from a phone and load the portal during a wake | `sleep refused by captive` while you keep clicking. Readings still come every 60 s, so periodic work still runs while sleep is refused. Leave the phone idle but connected: it sleeps again ~2 min after your last request. | phase 4 |
| A5 | as A2 | In the portal, set the MQTT hostname to something that does not resolve | MQTT never connects and **does not** refuse sleep: it sleeps at ~20 s each cycle. Put the hostname back afterwards. | phase 4 |
| A6 | `Power_Deep, 120000, 30000` | Run 3 cycles | `Power: Waking from deep` at each boot. Readings every 2 min. | phase 1 |
| A7 | `Power_Lora, 120000, 30000` | Run 3 cycles | At ~30 s: `preparing`, `WIFI Paused`, then **no WiFi messages at all** for ~90 s (no reconnect, no scan); the node stays awake. At 2 min: `Waking for`, WiFi reconnects, MQTT, readings, still on a 2-min schedule. No LoRaMesher on this board, so there are no `sleep rq` lines. | phase 2 |

Optional ESP8266 check (env `d1_mini`, `Power_Deep, 60000, 20000`): it prints `Power: sleep modes
are ESP32 only - using Power_Loop` and takes readings every 60 s without ever printing `preparing`.

## Part B: LoRa pair (loramesher example)

- **Gateway:** has WiFi credentials (so it gets MQTT). Build **without** `SYSTEM_LORAMESHER_SLEEP`, so it runs `Power_Loop`.
- **Node:** **no** WiFi credentials (`uploadfs` with an empty `data/wifi/`), otherwise it gets MQTT during its wake and becomes a gateway. Build **with** `-D SYSTEM_LORAMESHER_SLEEP`. For testing, shorten its cycle in `loramesher.ino` to `Power_Lora, 120000, 30000`.

A USB power meter on the node makes B2 much easier to read.

| # | Do | Expect | If it fails |
|---|---|---|---|
| B1 | Run the gateway alone for a few minutes | Every `sleep rq` is `refused`. WiFi and MQTT stay up. (Before phase 3 the mesh light-slept the gateway and dropped its WiFi.) | phase 3 |
| B2 | Run the node with the gateway up | **First 30 s:** every slot `refused`. **At ~30 s:** `preparing`, `WIFI Paused`. **Then:** mostly `allowed` with `wakeUp` lines, and current should drop. **Last second or so before 2 min:** `refused`, then `Waking for` and a reading that reaches the dashboard through the gateway. Cycle stays on 2 min. | phase 3 (sleeping), phase 2 (dormant timing) |
| B3 | Leave the B2 node running ≥ 30 min | It keeps cycling. If it ever **hangs** (no output, no readings), suspect the loop lock: LoRaMesher allowed a sleep and never called back. Note the last `sleep rq`/`wakeUp` line. | phase 3 |
| B4 | Give the node WiFi credentials (`uploadfs`) and restart it, still with `SYSTEM_LORAMESHER_SLEEP` | Once it has MQTT and takes the gateway role: `sleep refused by loramesher` at each wake end, so it never goes dormant, and the mesh `sleep rq` lines are all `refused`. Readings continue every cycle. | phase 4 |

## Not covered

- **millis() rollover** (49 days). Checked by reading the code only.
- **A connected MQTT whose publishes fail** (`lastSendOk`). Hard to produce on purpose: it needs the broker to drop the socket without the client noticing.
