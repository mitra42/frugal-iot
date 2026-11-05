# OTA Build & Publish Plan

Tooling to rebuild node firmware for the OTA keys an organization actually runs, and get the
binaries onto the server. One organization per run.

## Status

**Complete** (reviewed 2026-11-05), and in regular use: this is how releases are done now. All
three scripts exist —
`scripts/ota_build.py` here, `ota_keys.py` / `ota_upload.py` / `ota_aliases.tsv` in
`frugal-iot-demo/scripts/` — and §10 is the runbook for them. T0–T10 in §9 are done; §9 still
lists T7–T10 as to-do because it was not updated once the tooling was written.

Also out of date: the line at the top of §10 saying the upload "has no script yet" (§10.6 and
§10.7 describe the script that now exists), and the sample `ota_aliases.tsv` in §4 (the live file
also moves `temp_supermini-4x` to `bme680_supermini`, above the general rules).

Open questions: none. Ongoing upkeep, not a plan item: delete alias lines as they self-retire
(§10.8).

*(The original status line read: "plan, nothing built yet.")*

Revision 9 — incorporates the prefix fixes (`commonground`→`commonroom`, `lcd_sht`→`lcd_ht`),
the decisions on O1-O5, the alias / per-node cases raised in review, and the **T0-T3
verification results**, which forced a redesign of how the local library is supplied (D1).

---

## 1. What is already true (verified, not assumed)

### 1.1 The OTA key is two build flags

`system/ota.cpp:192` sends `SYSTEM_OTA_PREFIX "_" SYSTEM_OTA_SUFFIX` as the retained `ota/key`.
Prefix is per-example, suffix per-env.

**Prefix rule — `SYSTEM_OTA_PREFIX == directory name`: holds everywhere.** No example defines
more than one prefix across its envs either.

**Suffix rule — `SYSTEM_OTA_SUFFIX == env.lower()`: 2 rows violate it**, one accepted
(`lilygohigrow`), one open (`commonroom`). A lint warning, not an assertion. See §1.6.

One env has no suffix, so cannot be OTA'd: `sonoff/r2` — deliberate and documented in the ini
(*"No OTA_KEY for Sonoff, there is not enough space (code > flash/2)"*).

### 1.2 PlatformIO's own config parser resolves this correctly

```python
from platformio.project.config import ProjectConfig
c = ProjectConfig('examples/sht/platformio.ini')
c.get('env:d1_mini', 'build_flags')   # ${common.build_flags} already interpolated
```

Across all 20 examples: **222 envs → 221 distinct OTA keys, zero collisions.** Every key maps to
exactly one (example, env).

Do *not* reimplement with regex/awk. `generate_platform_h.py` is a line-oriented regex converter
and `arduino_compile.bash` uses `awk`; both mis-parse `[env:d1_mini] ; also for...` trailing
comments. `ProjectConfig` does not.

### 1.3 Do not use prefix/starting-string matching to find the example — O-review

The suggested rule was "find a directory that is a starting string of the key". It is both
**unsafe and insufficient**, so the plan does not use it:

- **False positive:** `sht30_c3_pico` starts with `sht`, so the rule maps it to `examples/sht` —
  but the remainder is `30_c3_pico`, which is not an env. A longest-prefix match would silently
  produce a wrong or unbuildable answer.
- **Misses the case it was meant for:** `lcd_sht_c3_pico` does **not** start with `lcd_ht`. A
  rename in the other direction is invisible to prefix matching.

The exact 221-key map from §1.2 plus an explicit alias table (§4) covers both, and the alias
table is the human-specified mapping you already said these cases need.

### 1.4 "The nodes table" means `/config.json`, not the `nodes` SQL table

- `GET /nodes_list/:org` reads the SQL `nodes` table: `project, nodeid, lora, enrolled_at` plus a
  merged `state` (enrolled / approved / denied / failed). **No OTA key in it.**
- The dashboard's Nodes table (`admin.js:1073`) reads `/config.json`, whose node entries are live
  retained MQTT values — including `ota/key`. That is the OTA Key column, and it is what the node
  actually has, hence what it will ask for.

Step 1 pulls `/config.json` and merges `/nodes_list/:org` for the enrolment state.

### 1.5 The server side needs no changes

- `POST /ota_update` — multipart, session cookie, fields `organization`, `project`, `otakey`,
  `file`. Lands at `ota/<org>/<project|+>/<otakey>/firmware.bin`.
  **Field order is load-bearing:** multer fills `req.body` only from parts arriving *before* the
  file, and the OTAUPDATE permission check runs while the file is being stored.
- `GET /ota_list/:org` — existing keys. `GET /ota_get/:org/*path` — download a binary.
- `POST /login` — form login, sets a session cookie. Success redirects to `req.body.url`, failure
  to the login page with `messagetype=error`. **Check the `Location` header** — both are 302, so
  "did it 302" passes on a wrong password.
- Lookup order is `[project, node]`, `['+', node]`, `[project, attribs]`, `['+', attribs]`. Two
  things follow: a board-type build belongs under project `+` (broadest match), and **a
  node-specific binary outranks any key**, which is what §5 relies on.

### 1.6 Remaining suffix exceptions

**Rule: rename the env, never the suffix.** The suffix is stamped into deployed hardware;
changing it strands every node on that key. The env name exists only in the ini and no device has
ever seen it. Where they disagree, the env is the side that is free to move.

Commit `a134205` fixed most of these, but in **mixed directions**:

*Safe — env renamed, key unchanged:*

- `esp32c3_oled_72x40` -> `c3_oled_72x40` (sht, remotedisplay)
- agri `c3_pico` -> `c3_pico_2`

*Key-changing — suffix edited, so deployed nodes are stranded until aliased (§4):*

| old key | new key | scope |
|---|---|---|
| `<example>_ttgo-t-beam-oled` | `<example>_tbeam_oled` | 16 examples |
| `temp_supermini-4x` | `temp_supermini` | temp |
| `agri_s2_mini_2` | `agri_s2_mini` | agri |

No revert needed - three alias lines cover it, and they self-retire (§4). Whether any node is
actually on those keys is what step 1 reports.

*Still outstanding:*

- **`commonroom` `s2_mini` -> `s2_mini_2`**: untouched by `a134205`. Unlike agri, commonroom has
  no `s2_mini_5`/`_6` siblings, so the `_2` looks inherited rather than meaningful. Renaming the
  env to `s2_mini_2` keeps the key; changing the suffix needs a fourth alias.

*Accepted exception:*

- **`lilygohigrow` -> `prebuilt`**: intentional. The LilyGo HiGrow is a single consumer item, not
  a known board, so the suffix names what it is rather than a board. The harness must therefore
  treat `suffix == env.lower()` as a lint warning, not an assertion.

### 1.7 `extra_configs = *-local.ini` is a glob, in every example

All 20 examples have `extra_configs = *-local.ini`, a `[common] build_flags_local =` stanza and a
`${common.build_flags_local}` reference — uniformly. A file dropped in as
`platformio-secrets-local.ini` is picked up **with no edit to any `platformio.ini`.**

`lib/Frugal-IoT/.gitignore` does **not** ignore `*-local.ini` (the demo project's does). Task T0.

---

## 2. Decisions taken

| # | Decision |
|---|---|
| D1 | Library source selectable per run: `--lib local` (default) / `--lib registry`. A local library **does** override the `lib_deps` registry entry without blanking it - but it must sit in the project's `lib_dir`, and the project must be **staged outside the library tree**. See §7.0; the original "symlink into the example" form is impossible. |
| D2 | **Split**: build script in `lib/Frugal-IoT/scripts/`, upload script in `frugal-iot-demo/scripts/` (org- and credential-specific, must not ship to the registry). |
| D3 | Upload is a **script with form login** — POST `/login` into a cookie jar, then one `POST /ota_update` per key. No server changes. |
| D4 | **Stop at first build failure.** Revisit once the harness is trusted. |
| D5 | **No `--release` flag.** This whole process *is* release. Development-only flags live in `platformio-local.ini`, which is never copied to an example. (§3) |
| D6 | `--lib local` **refuses a dirty library working tree without `--force`**, and records `-dirty` in `build.json` when forced. |
| D7 | Orphaned server-side keys are **reported, not deleted.** No `--prune` in v1. |
| D8 | **One organization per run.** |
| D9 | **Credentials are prompted for at run time**, never passed as a command-line flag - a flag lands in shell history. Username may be a flag (not secret); the password is read with `getpass`. An env var is allowed as an unattended-run escape hatch. The session cookie is cached to a `0600` gitignored jar so `ota_keys.py` and `ota_upload.py` do not each prompt. |

---

## 3. Secrets and debug flags

Two concerns currently sharing `platformio-local.ini`, split because they want opposite
treatment — the secret must reach every example, the debug flags must reach none.

| file | holds | copied to examples? | committed? |
|---|---|---|---|
| `platformio-secrets-local.ini` | `SYSTEM_MQTT_ENROL_SECRET`, which server (`SYSTEM_MQTT_HOST`, `SYSTEM_MQTT_ENROL_URL`, org/project) | **yes** | never |
| `platformio-local.ini` | debug flags, `upload_port`, `monitor_port` | **no** | never |

Master copies live in `frugal-iot-demo/`. One filename everywhere: naming it
`platformio-secrets-local.ini` makes it match the existing `*-local.ini` glob in the demo root
*and* in all 20 examples — zero ini edits, and it is already gitignored in the demo by the rule
that is there.

**Symlink, not copy**, from example → master: one source of truth, and rotating the enrolment
secret needs no re-propagation. Falls back to a copy if that fails (T3).

Release vs development is then simply **whether `platformio-local.ini` is present** — it never is
in an example, so an OTA build is a release build by construction. No flag needed (D5).

### 3.1 The debug flags are currently in the committed example inis

For that to be true, the flags have to actually move. **19 of the 20 examples ship active
`*_DEBUG` flags today**, 21 distinct macros:

| flag | examples |
|---|---|
| `SYSTEM_LORAMESHER_DEBUG` | 18 |
| `SYSTEM_MESSAGE_DEBUG` | 6 |
| `SENSOR_SHT_DEBUG`, `SYSTEM_MDNS_DEBUG`, `SYSTEM_WIFI_DEBUG` | 4 each |
| `SENSOR_DS18B20_DEBUG`, `ACTUATOR_OLED_DEBUG`, `SYSTEM_I2C_DEBUG`, `SYSTEM_MQTT_DEBUG` | 2 each |
| 12 others | 1 each |

Worst offenders: `sht` (8 active), `temp` (9 active), `agri` (5), `ensaht` (5), `sonoff` (4).

`SYSTEM_MESSAGE_DEBUG` logs every message sent, received and looped — not something to push to a
fleet. **Task T5: comment these out in the 19 example inis** (they already sit in a commented
menu in `build_flags_frugaliot`, so this is uncommenting-in-reverse, not restructuring), and keep
the menu, commented, in `platformio-local.ini` for local use.

`build_unflags` is the fallback if you would rather not touch 19 files, but it leaves the inis
misleading for Arduino IDE users, who get no `platformio-local.ini` at all.

### 3.2 Client text needs updating

`admin.js:254` tells people *"Put this in platformio-local.ini, which is not committed:"* under
the enrolment secret. That must become `platformio-secrets-local.ini`. The string is translated
into 4 languages in `core.js` (lines 544, 821, 1098, 1375 — en, fr, hi, id), so it is 5 edits in
`frugal-iot-client`, not 1. Task T6.

---

## 4. Aliases — keys that have been renamed

A node flashed with an old key asks for the old key. The firmware we want it to run is built
under the new one. So the same binary is **published under both**, and an explicit
human-maintained table says which:

`frugal-iot-demo/scripts/ota_aliases.tsv`

```
# published_key_glob   built_from_key       note
sht30_*                sht_*                renamed; retire when no node reports sht30_
lcd_sht_*              lcd_ht_*             renamed 2026-09; retire when no node reports lcd_sht_
*_ttgo-t-beam-oled     *_tbeam_oled         suffix changed in a134205 (16 examples)
temp_supermini-4x      temp_supermini       suffix changed in a134205
agri_s2_mini_2         agri_s2_mini         suffix changed in a134205
```

Three shapes are understood, because a rename is always one of them: a prefix glob
(`sht30_* -> sht_*`), a suffix glob (`*_supermini-4x -> *_supermini`), or one exact key. **First
match wins**, so a specific rule must sit ABOVE any general rule it is an exception to.

The same mechanism moves a node to a *different* example - `temp_supermini-4x -> sht_supermini`
is not a rename, but "build that key, publish it under this one too" is exactly what is wanted,
and it self-retires identically. Use an alias when EVERY node on a key should move; use
`target_node` (§5) when only one should.

Folded into both ends rather than done by hand afterwards:

- **Step 1** resolves `sht30_c3_pico` → builds `sht_c3_pico`, `build=y`, `note` records the alias.
  So it appears in the manifest instead of as an unresolvable row you have to remember about.
- **Step 3** uploads `sht_c3_pico/firmware.bin` to *both* `sht_c3_pico` and `sht30_c3_pico`.

**Aliases are self-retiring.** Once a node takes the update it reports `ota/key = sht_c3_pico`,
because the new binary has the new prefix compiled in. Step 1's output therefore shows the old
key's node count falling to zero, at which point the alias line can be deleted. The report should
say so explicitly — otherwise the table only ever grows.

---

## 5. Per-node firmware

Three cases from review need a binary aimed at one node rather than at a board type:

- **`aht20-bmp280` / `temp_c3_pico`** — wants its own example (AHT20 temperature wired to the
  BMP280), published generically as `aht20bmp280_c3_pico` *and* pointed at that specific node
  until it migrates.
- **BME680** — same shape, same treatment.
- **`esp32-302721` reporting no otakey** — should be `sht_supermini` like `esp32-9ce820`. Most
  likely it has not published a retained `ota/key` (never rebooted since the OTA module landed,
  or the retained message was lost). Targeting it by node id sidesteps the question entirely.

The server already supports this: `['+', node]` is checked **before** `[project, attribs]`
(§1.5), and the admin form's field is labelled "OTA Key or Device ID" for exactly this. So it is
an extra manifest column, not new machinery:

```
build  otakey        target_node    example        env      ...
y      -             esp32-302721   sht            supermini
y      -             esp32-7f11a0   aht20bmp280    c3_pico
```

With `target_node` set, step 3 uploads to `ota/<org>/<project|+>/<nodeid>/firmware.bin`. These
rows do **not** self-retire — a node-specific binary outranks the key forever, so the report must
list them every run as "pinned", or they become invisible state.

---

## 6. Step 1 — the key table

`frugal-iot-demo/scripts/ota_keys.py --org dev [--all-keys] > ota_keys.tsv`

Joins four sources: `/config.json` (live `ota/key` per node), `/nodes_list/:org` (enrolment
state), the parsed example map (§1.2), and `ota_aliases.tsv` (§4). Authenticates per D9, and
**must chdir into each example** before parsing it (§7.2).

TSV — hand-editable, diffable, opens in a spreadsheet. The **full table**, one row per node:

```
build  otakey              target_node  example   env        project     nodeid          name          lastseen          state     note
y      sht_d1_mini         -            sht       d1_mini    developers  esp8266-85ea2b  Greenhouse 1  2026-09-24 08:11  enrolled
y      sht_d1_mini         -            sht       d1_mini    lotus       esp8266-91cc04  Nursery       2026-09-23 19:40  enrolled
y      sht30_c3_pico       -            sht       c3_pico    developers  esp32-1188fe    Old Pico      2026-09-25 06:02  enrolled  alias -> sht_c3_pico
y      -                   esp32-302721 sht       supermini  developers  esp32-302721    Supermini 2   2026-09-25 07:14  enrolled  pinned: node reports no ota/key
n      sht_esp32_old       -            -         -          developers  esp32-aa1201    Retired       2026-02-11 04:33  enrolled  no such key, and no alias
```

- `build` is the column you edit. Defaults to `y` where the key resolves, `n` otherwise with the
  reason in `note`.
- Step 2 dedupes by (example, env), so two `sht_d1_mini` rows cause one build.
- Unresolvable keys are the interesting output — a node on a key no example produces any more is
  exactly what you want to see before a push.
- `--all-keys` appends the 221 buildable keys nobody is running at `build=n`.
- Also reported (D7): server-side keys from `/ota_list/:org` that no node runs and no example
  produces — orphans. Listed, never deleted.

---

## 7. Step 2 — build

`lib/Frugal-IoT/scripts/ota_build.py --manifest <tsv> --stage <dir> [--lib local|registry]`

Plus `--key sht_d1_mini` to build one without a manifest, for iterating.

For each distinct (example, env) with `build=y`:

1. Ensure `platformio-secrets-local.ini` is in place (§3).
2. `--lib local` → ensure `examples/<example>/lib/Frugal-IoT` symlinks to the library root.
   `--lib registry` → ensure it is absent.
3. `pio run -d examples/<example> -e <env>`
4. Copy `<build_dir>/<env>/firmware.bin` → `<stage>/<otakey>/firmware.bin`, matching the server's
   layout so upload is a straight walk.
5. Write `<stage>/<otakey>/build.json` — md5, size, built-at, example, env, lib mode, library git
   sha (`-dirty` per D6), and the `SYSTEM_OTA_PREFIX`/`SUFFIX` actually compiled in. Without it a
   stale binary in the staging tree is indistinguishable from a fresh one.

On failure: print pio's output, exit non-zero, leave the staging tree as-is (D4).

### 7.0 Builds are staged OUTSIDE the library tree - never in place

Five independent findings from T1/T3/T9 force this, and one staging step answers all of them.

**1. A symlink inside the example is a cycle.** The examples live *inside* the library, so
`examples/sht/lib/Frugal-IoT -> <library root>` means
`lib/Frugal-IoT/examples/sht/lib/Frugal-IoT/examples/sht/lib/Frugal-IoT` resolves - forever. LDF
walked into it and blocked for 57 minutes on 1.3 seconds of CPU. Not slow: non-terminating.

**2. `lib_extra_dirs` resolves the library but skips its dependencies.** It satisfies
`Frugal-IoT@^2.0.0` locally, so PlatformIO runs no Library Manager phase at all - libdeps came
back holding only `integrity.dat` and the build died on `ESPAsyncWebServer.h: No such file or
directory`. With the library in `lib_dir` instead, all transitive deps (MQTT, BH1750, Button2,
DHTNEW, HX711, ESPAsyncWebServer, ...) install normally. **The library must be in `lib_dir`.**

**3. The staged library must be a PRUNED view, not the repo root.** Symlinking the whole library
root into `lib_dir` exposes everything else in the repo as candidate libraries - including every
*other* example's leftover `.pio/libdeps/`. An ESP8266 build picked up
`examples/agri/.pio/libdeps/c3_pico/AsyncTCP` (an ESP32 library) and died on
`sdkconfig.h: No such file or directory`. Stage only what a library actually is:
`src/`, `library.json`, `library.properties`, `keywords.txt`. No `examples/`, no `.pio/`, no
`lib/` - which also makes the cycle in (1) structurally impossible.

**4. lib_dir must sit OUTSIDE the staged project.** Every example sets `src_dir = .`, so the
project root IS the source directory - and a `lib/` inside it is walked as project source as well
as being linked as a library, compiling all 64 library files TWICE. On ESP32 that is a hard
duplicate-symbol link failure. On ESP8266 it **links silently**, because `lib_archive` defaults
true there so the second copy sits in an archive the linker never opens. So `lib_dir` is pointed
at a sibling `<example>.lib/` by a staged `platformio-stage-local.ini` (which the existing
`*-local.ini` glob picks up, so still no edit to any example).

**A green ESP8266 build is no evidence for any of this** - it linked happily for two rounds of
testing while silently compiling everything twice. Validate staging changes on ESP32.

**5. Building in place damages the example.** A build leaves `<sketch>.ino.cpp` in the sketch
folder, and per the project CLAUDE.md any `<sketch>.ino*` file there makes arduino-cli - and so
Arduino IDE - drop the example from **File > Examples** entirely. A crashed build leaves it
behind silently.

So each build gets a staging project, outside the library:

```
<stage>/<mode>/<example>/          # <mode> is local | registry - see 7.1
    <symlink per file in examples/<example>/>   # .ino, .cpp, .h, .csv, platform.h
    platformio-secrets-local.ini -> <master>
    platformio-stage-local.ini                  # points lib_dir at the sibling below - see (4)
<stage>/<mode>/<example>.lib/                   # lib_dir, OUTSIDE the project
    Frugal-IoT/                                 # --lib local only; absent for registry
        src              -> <libroot>/src       # PRUNED - see (3). Never the repo root.
        library.json     -> <libroot>/library.json
        library.properties -> <libroot>/library.properties
        keywords.txt     -> <libroot>/keywords.txt
```

This also removes the T3 hazard for free: a staging dir contains no stray `*-local.ini`, so the
`extra_configs` glob cannot pick up a developer's debug flags (§7.2).

### 7.1 Two cache traps to design around

Both measured in T2, against `compute_project_checksum`, which hashes `config.to_json()` - the
*merged* config - plus a walk of `include_dir`/`src_dir`/`lib_dir` for source **paths** (not
contents).

| state | checksum |
|---|---|
| baseline | `2e00f7bb...` |
| + secrets file present | `d685b07c...` |
| + secrets **content** changed | `76968b40...` |
| + `lib/Frugal-IoT` symlink added | `76968b40...` **unchanged** |

**`extra_configs` contents DO feed the checksum.** So the secrets file first appearing wipes all
of that example's build dirs - unavoidable once. After that, **only write it if the content
differs**; an unconditional copy re-wipes every run and turns an incremental rebuild into a full
one.

**A symlinked library does NOT feed the checksum**, because that walk uses `os.walk`'s default
`followlinks=False` and never descends into it. So switching `--lib` mode is invisible to
PlatformIO's cache and it would relink existing objects - a "registry" build that is silently
your working tree. Separate staging trees per mode (§7.0) are **required**, not a precaution.

### 7.2 `extra_configs` globs resolve against the CWD, not the ini

Verified in T3, and it is a trap worth stating plainly. Parsing
`lib/Frugal-IoT/examples/sht/platformio.ini` **from the demo directory** picks up
`frugal-iot-demo/platformio-local.ini` - the developer's debug flags and real enrolment secret -
and applies them to the sht example, silently. Parsing the same file from inside `examples/sht/`
picks up the intended file.

`pio run -d <dir>` is safe: its source does `with fs.cd(project_dir):`. **But step 1's key-map
parser must chdir too**, or it describes an example while reading the demo's local ini.

### 7.3 Cost

A live org is more like 20–40 keys, not 221. Cold ESP32 build is minutes, warm is seconds, and
build dirs are per (example, env) so they stay warm between runs. Budget an hour for a first full
run, minutes after.

---

## 8. Step 3 — upload

`frugal-iot-demo/scripts/ota_upload.py --org dev --stage <dir> [--project +] [--skip-unchanged] [--dry-run]`

1. Log in per D9: `--user <name>` optional, password via `getpass` prompt, or
   `FRUGAL_IOT_PASSWORD` for unattended runs. Reuses the cached cookie jar if still valid.
2. `POST /login` with `username`, `password`, `url=/dashboard`. **Verify the `Location` header**
   (§1.5).
3. Walk the staging tree. For each binary, `POST /ota_update` multipart with fields in this
   order: `url`, `lang`, `organization`, `project`, `otakey`, **then** `file`.
4. Expand aliases (§4) — same binary, extra keys. Expand `target_node` rows (§5) — same binary,
   node id in place of the key.
5. `--project +` by default, so a build applies to every project in the org.
6. `--skip-unchanged` compares md5 against `GET /ota_get/...` first. Optimisation only —
   re-uploading an identical binary is harmless, devices compare md5 and get a 304.
7. `--dry-run` prints what would be sent and exits. Use it before the first real push.

---

## 9. Order of work

Verification first — each is an assumption that would quietly produce a wrong binary.

- **T0 — DONE.** `*-local.ini` added to `lib/Frugal-IoT/.gitignore`, verified with
  `git check-ignore -v`. (§1.7)
- **T1 — DONE, with a redesign.** A local library *does* override `Frugal-IoT@^2.0.0` without
  blanking - but only staged as in §7.0. `sht/d1_mini` built SUCCESS in 90s, 316 objects from the
  staged `lib/Frugal-IoT/src/`, firmware.bin 569,088 bytes, example dir clean afterwards. Three
  other arrangements were ruled out first - see §7.0. (D1)
- **T2 — DONE.** `extra_configs` contents DO feed it; a symlinked library does NOT. Measured
  checksums in §7.1. Write-only-if-different is mandatory; per-mode staging is mandatory.
- **T3 — DONE.** Globbing resolves a symlink, and the staged secret reaches the real compile
  (seen as `SYSTEM_MQTT_ENROL_SECRET="T3_PROOF_CHANGED"` in the build flags). Also found: globs
  resolve against the **CWD**, not the ini's directory - see §7.2.
- **T4 — DONE.** `commonroom` `s2_mini` -> `s2_mini_2` fixed. Only `lilygohigrow`/`prebuilt`
  remains, which is accepted (§1.6), so the suffix rule is a lint warning, not an assertion.
- **T5 — DONE.** 64 lines commented across all 20 examples (`-D` -> `;-D`, comments preserved).
  Verified: zero active DEBUG flags, OTA key set byte-identical (222 envs / 221 keys, no
  collisions). `scripts/prerelease.bash` re-run, so `platform.h`, the `.ino.globals.h` files and
  `keywords.txt` are back in step.
- **T6 — DONE.** `platformio-local.ini` -> `platformio-secrets-local.ini` in `admin.js:254` and
  all 4 language tables in `core.js` (key *and* value, since the English string is the key).
  `node --check` passes; table ordering unaffected.
- **T7** — `ota_keys.py`, read-only, against `dev`. Eyeball the TSV: do the unresolvable rows
  match the ones you already know about (`sht30_`, `lcd_sht_`, `esp32-302721`)?
- **T8** — `ota_build.py --key sht_d1_mini` both ways. Diff the two binaries — **they must
  differ**, or T1/§7.1 is wrong.
- **T9** — `ota_build.py --manifest` over the real edited table.
- **T10** — `ota_upload.py --dry-run`, then one key for real, then the rest.

**T0-T6 are done.** T7 onwards is the tooling itself.

---

## 10. Runbook — start to finish

Written after the first real run of 14 builds / 22 published keys. Every step below has been
exercised except the upload, which has no script yet (§10.6).

Everything happens from the demo directory:

```
cd ~/Documents/Arduino/frugal-iot-demo
```

### 10.0 Once only, per machine

The enrolment secret must live in `platformio-secrets-local.ini`, **not** `platformio-local.ini` —
only the first is given to examples, and the second holds debug flags and serial ports that must
never reach a release build.

Both match the `extra_configs = *-local.ini` glob, so both are loaded for this project and neither
is committed. **Define `build_flags_local` in exactly one of them:** a value is REPLACED, not
merged, so defining it in both means whichever loads last silently wins.

### 10.1 Make sure the library is committed

```
cd lib/Frugal-IoT && git status --short && cd ../..
```

Not housekeeping. `--lib local` refuses a dirty tree without `--force`, because a binary built
from uncommitted work cannot be reproduced later. Building with `--force` is fine while
iterating; the fleet should be built from a clean tree so `build.json` records a real sha.

### 10.2 Refresh the key table

```
./scripts/ota_keys.py --org dev -o ota_keys.tsv
```

Prompts for the password. Reads `/config.json` for each node's live `ota/key`, `/nodes_list/:org` for enrolment
state, and `scripts/ota_aliases.tsv` for renames.

**Re-run this after any change to `ota_aliases.tsv`** — alias resolution happens here, and
`ota_build.py` trusts the `example`/`env` columns it writes.

### 10.3 Edit the table

It is a TSV, meant to be edited. The `build` column is the one to change.

- A key no example builds, that is a **rename**: add a line to `scripts/ota_aliases.tsv` and go
  back to 10.2. First match wins, so a specific rule goes ABOVE a general one.
- A node that should get **its own** firmware (it reports no `ota/key`, or it needs something
  different from everything else on its key): set `build=y`, put its id in `target_node`, and
  fill in `example` and `env` by hand — there is no key to look them up from.
- Anything not being rebuilt this round: `build=n`.

Preview without building anything:

```
./lib/Frugal-IoT/scripts/ota_build.py --manifest ota_keys.tsv --list --stage ./ota-stage
```

Read the list. One build can publish several keys (an alias publishes under old *and* new) and
can also serve a pinned node, so "14 builds -> 22 published names" is normal.

### 10.4 Build

```
rm -rf ota-stage        # only when starting a round cleanly - see the warning below
./lib/Frugal-IoT/scripts/ota_build.py \
    --manifest ota_keys.tsv \
    --secrets platformio-secrets-local.ini \
    --stage ./ota-stage
```

**`--stage` may be relative; it is made absolute internally.** 

**Do not omit `--secrets`.** Without it the firmware carries no enrolment secret and looks
identical to a good binary in the staging tree; `build.json` records `enrol_secret: false` so it
can be caught afterwards, but it is far better not to do it.

Each build prints one line and is otherwise silent for a minute or two — that is the captured pio
output, not a hang. `--verbose` streams it.

It **stops at the first failure**, leaving everything already built in place.

### 10.5 When a build fails

Almost every failure so far has been an example calling library API that only exists under a
`*_DEBUG` flag, because release builds have all of them off. Fix the example or the library, then:

```
./lib/Frugal-IoT/scripts/ota_build.py --manifest ota_keys.tsv \
    --secrets platformio-secrets-local.ini --stage ./ota-stage --skip-built --force
```

`--skip-built` matters: a library edit changes `custom_frugal_lib_fingerprint`, which invalidates
every staged build directory, so without it each fix costs a full rebuild of everything already
done — and that gets worse the further down the list you are.

The cost of `--skip-built` is that the skipped binaries were built against the library as it was
**before** the fix. So when everything finally compiles, run once more **without** it:

```
./lib/Frugal-IoT/scripts/ota_build.py --manifest ota_keys.tsv \
    --secrets platformio-secrets-local.ini --stage ./ota-stage
```

That rebuilds the lot against the final library. Every run reports any staged binary whose
`lib_fingerprint` differs from the library on disk, so this cannot be forgotten.

### 10.6 Check the staging tree before uploading

```
./scripts/ota_upload.py --check --stage ./ota-stage
```

Purely local - no organization, no login, touches nothing. Exits non-zero if the tree is not fit
to upload.

```
22 binary/binaries under .../ota-stage/firmware
   22 x  fingerprint 067593dd7c7f2678  sha 06982bd  local  enrol_secret=True
  21 published by OTA key, 1 pinned to a node
  consistent - one library, all with an enrolment secret, none dirty
```

**One fingerprint line is what you want.** More than one means some binaries predate a fix, which
is what `--skip-built` leaves behind - go back and do the reconciling run in 10.5. The same three
checks run automatically on every upload, so this step is really just a way to see the answer
before committing to it.

### 10.7 Upload

```
./scripts/ota_upload.py --org dev --stage ./ota-stage --dry-run   # always first
./scripts/ota_upload.py --org dev --stage ./ota-stage
```

Prompts for the password unless `ota_keys.py` already logged in this session - the two share a
cookie jar. `--dry-run` sends nothing and does not even log in.

It reads each directory's `build.json` to decide where the binary goes:

| `target` | published to |
|---|---|
| `key` | `<org>/<project>/<otakey>/firmware.bin` |
| `node` | `<org>/<project>/<nodeid>/firmware.bin` |

`--project` defaults to `+`, meaning every project. The server looks for `[project, node]`,
`['+', node]`, `[project, attribs]`, `['+', attribs]` in that order, so `+` is the broadest match
and a node-pinned binary outranks the key its node reports.

`--skip-unchanged` downloads what is already published and skips anything identical. Purely an
optimisation - re-uploading an identical binary is harmless, because devices compare md5 and get
a 304.

**Three things it refuses to upload**, because each looks perfectly healthy in the staging tree
and only goes wrong later:

- a binary with `enrol_secret: false` - built without `--secrets`, works on an enrolled node and
  cannot re-enrol after a factory reset
- a tree with more than one `lib_fingerprint` - some binaries predate a fix, which is exactly
  what `--skip-built` leaves behind
- a binary whose `lib_git_sha` ends `-dirty` - built from uncommitted work, not reproducible

`--force` overrides all three. One flag for all of them on purpose: each is a decision, not
something to wave through out of habit.

### 10.8 Afterwards

- A node that takes an aliased update starts reporting the NEW key, so re-running 10.2 shows the
  old key's node count falling. When it reaches zero, delete that line from `ota_aliases.tsv` —
  aliases are self-retiring.
- Pinned `target_node` rows do **not** retire; they outrank the key forever and are listed every
  run.
- Orphans (a key on the server that no node runs and no example builds) are reported, never
  deleted.
