# LoRaMesher example — two T-Beam test

Flash a pair of TTGO T-Beam v1.1 boards as a LoRaMesher gateway + node and watch
the mesh form. EU 868 MHz by default (see `platformio.ini`).

## Environments

| Env             | Role    | Board              |
| --------------- | ------- | ------------------ |
| `gateway_tbeam` | Gateway | TTGO T-Beam v1.1   |
| `node_tbeam`    | Node    | TTGO T-Beam v1.1   |

## Quick start

Plug both boards in, identify their ports (`ls /dev/ttyUSB*` on Linux / WSL, or
Device Manager → *Ports (COM & LPT)* on Windows), then run one of the helper
scripts from this directory.

### Linux / macOS / WSL

```bash
./upload_monitor.sh                             # defaults: GW=/dev/ttyUSB0, ND=/dev/ttyUSB1
./upload_monitor.sh /dev/ttyUSB2 /dev/ttyUSB3   # custom ports
SKIP_UPLOAD=1 ./upload_monitor.sh               # monitor only
BAUD=115200  ./upload_monitor.sh                # override baud
```

### Windows PowerShell

```powershell
.\upload_monitor.ps1                            # defaults: COM3 / COM4
.\upload_monitor.ps1 -GatewayPort COM5 -NodePort COM7
.\upload_monitor.ps1 -SkipUpload
.\upload_monitor.ps1 -Baud 115200
```

First-run only, if execution policy blocks the script:

```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

## What the scripts do

Three phases, each fully parallel across the two boards:

1. **Compile** — `pio run -e gateway_tbeam` and `pio run -e node_tbeam` in parallel.
2. **Upload** — `pio run -t upload` to each port in parallel.
3. **Monitor** — `pio device monitor -b 460800` on each port in parallel.

Every serial line gets a `YYYY-MM-DD HH:MM:SS` prefix. Each run creates
`logs/<timestamp>/` containing:

```
build_gw.log    build_nd.log       # compile output
upload_gw.log   upload_nd.log      # upload output
loramesher_gw.log  loramesher_nd.log   # serial monitor (one per device)
```

The console also shows both streams live, prefixed `[GW]` / `[ND]` for visual separation.

Ctrl-C stops everything cleanly.

## Manual equivalent

If you'd rather run things by hand, two terminals:

```bash
pio run -e gateway_tbeam -t upload -t monitor --upload-port /dev/ttyUSB0
pio run -e node_tbeam    -t upload -t monitor --upload-port /dev/ttyUSB1
```

## Success signals

- Serial: `LoRaMesher started successfully` on both boards.
- Serial: `Route updated - Destination: 0x…, Hops: 1` within ~60 s.
- OLED: role changes from *Unconnected* → *Gateway* / *Node*.
- Gateway joins WiFi (captive portal SSID `FrugalIoT-loramesher-<chipid>` on
  first boot).

## Troubleshooting

- **Radio silent / empty routing table after 2 min** — on T-Beam, check that
  the AXP192 LDO2 rail (LoRa 3.3 V) is enabled before `frugal_iot.pre_setup()`
  runs.
- **Ports flip between boots** — if more than two USB-serial devices are
  attached, `/dev/ttyUSB0` vs `ttyUSB1` (or COM numbering) can swap. Pass
  ports explicitly.
- **Windows: `ImportError: cannot import name 'lfs' from … littlefs`** — the
  PIO venv's `littlefs-python` is broken. Repair with:
  ```powershell
  & "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" -m pip install --upgrade --force-reinstall littlefs-python
  ```
- **Output is random symbols / mojibake** — baud mismatch. `pio device monitor`
  with `--port` does not always inherit `monitor_speed` from `platformio.ini`;
  the scripts pass `-b 460800` explicitly. If your firmware uses a different
  speed, override with `BAUD=…` (bash) or `-Baud …` (PowerShell).
