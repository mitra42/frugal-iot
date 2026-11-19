# Frugal-IoT tests

Tests of the library's own behaviour (readings, wiring, controls, persistence) through the
messages a node sends and receives. They run on a real board with **no sensors attached**:
readings come from `Sensor_Fake`, whose value each test sets.

```
cd test
pio test -e c3_pico                        # build, upload, run every suite, report
pio test -e c3_pico -f test_hysteresis     # one suite
pio test -e c3_pico --without-uploading --without-testing   # just check they compile
```

Envs: `c3_pico`, `supermini`, `s2_mini`, `nodemcu-32s`. ESP32 only for now.

**Each suite formats the board's LittleFS before it starts**, WiFi credentials included. Use a
board you don't mind wiping.

## How a suite works

Each `suites/test_*` folder is one firmware, and so one node. The `setup()` builds a
`System_Frugal` with the modules under test, then runs the tests against it in order. Tests share
that node, so each one sets up the state it relies on (with `inject("set/...")`) rather than
depending on the test before it.

No broker is connected, so nothing ever leaves `System_Messages::outgoing`. That queue *is* the
record of what the node published, and it keeps only the latest payload per topic. `setUp()`
clears it before every test, so a test can also check that something was **not** published.

Helpers, from `lib/FrugalTest/FrugalTest.h`:

| | |
|---|---|
| `frugalTestPreSetup()` | Format LittleFS, then `configure_power`, `pre_setup`, `configure_mqtt`. Add modules after it, then `frugal_iot.setup()` |
| `Sensor_Fake` | A `Sensor_Float`: set `next` (NAN = no reading), call `read()` |
| `pump(n)` | Run `frugal_iot.loop()` n times (default 10). A wired chain takes one pass per hop |
| `inject(twig, payload)` | A message from the broker to this node, e.g. `inject("set/sht/temperature/max", "40")` |
| `injectPath(path, payload)` | The same for any topic, e.g. another node's reading |
| `published(twig)` / `latest(twig)` | The latest message on `<node>/<twig>`, as `System_Message*` (or null) / payload (or `"<none>"`) |
| `subscribed(path)` | Was a subscription to that topic sent upstream? Remote topics and `set/` only: a wire to this node's own topics needs none |
| `persisted(module, leaf)` | Is that setting on LittleFS, e.g. `persisted("controlhysteresis", "limit/value")`? |
| `statusText(full)` | The `/status` page as a String |
| `TEST_ASSERT_CONTAINS(needle, haystack)` | Unity has no substring assertion |

## Writing one

Copy `suites/test_hysteresis`. A new module gets a suite when it is written, testing the message
behaviour it is meant to have, including decisions such as "a uint16 output publishes nothing
when invalid".

## PlatformIO quirks met setting this up

- `test_dir = suites`, not `.`: with `.`, PlatformIO also collects Unity's example tests from
  `.pio/libdeps`.
- `FrugalTest`, `ESPAsyncWebServer` and `AsyncTCP` are all in `lib_deps` by hand. See the
  comments in `platformio.ini` for why.
- `SLEEP_HARDWARE_TEST.md` in this directory is a separate, manual procedure, not part of this.
