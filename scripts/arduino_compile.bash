#!/bin/bash
# Test-compile one example the way the Arduino IDE would, from the command line.
#
#   cd lib/Frugal-IoT/scripts
#   ./arduino_compile.bash commonroom nodemcu_tambak
#
# Arguments
#   $1  example name - a directory under ../examples  (e.g. commonroom)
#   $2  environment  - an [env:NAME] in that example's platformio.ini (e.g. nodemcu_tambak)
#
# Options
#   --install-deps   install the Arduino core and the libraries from library.properties first.
#                    The ESP32 core is a large download (>1GB), so this is opt-in.
#   --update         refresh the package indexes then upgrade only what has a newer version
#                    available - cores and libraries. Much quicker than --install-deps, which
#                    re-resolves and installs everything from library.properties.
#   --clean          discard the cached build directory first (full rebuild)
#   --keep           accepted but redundant - build dirs are now kept and reused
#   --list           list the examples and environments available, then exit
#
# Why this is not just "pio run"
# ──────────────────────────────
# PlatformIO reads platformio.ini directly. The Arduino IDE cannot, so each example ships a
# platform.h generated from it by generate_platform_h.py, which _settings.h includes when
# PLATFORMIO is not defined. The two toolchains therefore get their flags by different routes,
# and it is entirely possible for one to build while the other does not - which is what this
# script is for.
#
# It uses arduino-cli, which is the Arduino toolchain without the GUI - same cores, same
# libraries, same sketchbook as the IDE. Nothing needs to be opened.
#
# Two things it checks beyond "did the compiler exit 0"
#   1. That the example actually has a platform.h, and regenerates it so it matches the
#      current platformio.ini.
#   2. That the ARDUINO_* macro platform.h guards this env with is the macro the Arduino core
#      really defines for the board. If it is not, platform.h contributes NOTHING and the
#      sketch compiles with library defaults - a green build that proves very little.

set -o pipefail

# Resolve our own absolute path BEFORE the cd below - $0 is relative to the caller's directory,
# so after cd'ing it no longer resolves and --help failed with "sed: ...: No such file or directory"
SELF="$(cd "$(dirname "$0")" && pwd)/$(basename "$0")"

cd "$(dirname "$0")" || exit 1
SCRIPTS_DIR="$(pwd)"
LIB_ROOT="$(cd .. && pwd)"
EXAMPLES_DIR="$LIB_ROOT/examples"

INSTALL_DEPS=0
UPDATE_DEPS=0
DID_MAINTENANCE=0
KEEP_BUILD=0
CLEAN_BUILD=0
DO_LIST=0
EXAMPLE=""
ENVNAME=""

for arg in "$@"; do
  case "$arg" in
    --install-deps) INSTALL_DEPS=1 ;;
    --update)       UPDATE_DEPS=1 ;;
    --keep)         KEEP_BUILD=1 ;;   # kept anyway now, accepted for compatibility
    --clean)        CLEAN_BUILD=1 ;;
    --list)         DO_LIST=1 ;;
    -h|--help)      awk 'NR>1 && /^#/ {sub(/^# ?/,""); print; next} NR>1 {exit}' "$SELF"; exit 0 ;;
    -*)             echo "Unknown option: $arg" >&2; exit 2 ;;
    *)  if [ -z "$EXAMPLE" ]; then EXAMPLE="$arg"; elif [ -z "$ENVNAME" ]; then ENVNAME="$arg";
        else echo "Unexpected argument: $arg" >&2; exit 2; fi ;;
  esac
done

# ── Which Arduino board to compile for ──────────────────────────────────────────────────────
# Deliberately explicit rather than guessed: a wrong FQBN would compile for the wrong chip and
# still go green. Keep in step with generate_platform_h.py's env_defines/board_defines.
#
# Checked FIRST, because some envs need a different board under Arduino than under PlatformIO,
# and the platformio.ini board name has lost that information by then.
fqbn_for_env() {
  case "$1" in
    # PlatformIO has no lolin_c3_pico board, so [env:c3_pico] uses lolin_c3_mini plus
    # -D ARDUINO_LOLIN_C3_PICO. Arduino has the real board, and it defines that macro itself.
    c3_pico)         echo "esp32:esp32:lolin_c3_pico" ;;
    # "TTGO LoRa32-OLED" is the right Arduino board; the variant is chosen by its Board
    # Revision menu, whose v21new option defines ARDUINO_TTGO_LoRa32_v21new. Without the menu
    # option you silently get the V1 default.
    ttgo-lora32-v21) echo "esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new" ;;
    *)               echo "" ;;
  esac
}

fqbn_for_board() {
  case "$1" in
    lolin_c3_mini)          echo "esp32:esp32:lolin_c3_mini" ;;
    lolin_s2_mini)          echo "esp32:esp32:lolin_s2_mini" ;;
    nodemcu-32s)            echo "esp32:esp32:nodemcu-32s" ;;
    esp32-c3-devkitm-1)     echo "esp32:esp32:esp32c3" ;;
    ttgo-lora32-v21)        echo "esp32:esp32:ttgo-lora32" ;;
    lilygo-t3-s3)           echo "esp32:esp32:lilygo_t3s3" ;; # note: t3s3, not t3_s3
    heltec_wifi_lora_32_V3) echo "esp32:esp32:heltec_wifi_lora_32_V3" ;;
    esp32dev)               echo "esp32:esp32:esp32" ;;              # "ESP32 Dev Module"
    nologo_esp32c3_super_mini) echo "esp32:esp32:nologo_esp32c3_super_mini" ;;
    # sonoff_basic deliberately unmapped: Arduino's only Sonoff board is ITEAD Sonoff, which is
    # the SV variant, not the Basic this code targets. Better to fail loudly than build for the
    # wrong hardware.
    ttgo-t-beam)            echo "esp32:esp32:t-beam" ;;
    # Note there may also be an ancient esp8266com:esp8266 2.3.0 installed; esp8266:esp8266 is
    # the current core and the only one with d1_mini_pro
    d1_mini)                echo "esp8266:esp8266:d1_mini" ;;
    d1_mini_pro)            echo "esp8266:esp8266:d1_mini_pro" ;;
    *)                      echo "" ;;
  esac
}

# Ask generate_platform_h.py itself which macro it guards this env with, rather than keeping a
# duplicate table here that could drift out of step with it.
guard_for_env() { # $1 = board  $2 = env
  python3 - "$SCRIPTS_DIR" "$1" "$2" <<'PYEOF'
import sys
sys.path.insert(0, sys.argv[1])
from generate_platform_h import PlatformIOConverter
print(PlatformIOConverter("").get_board_define(sys.argv[2], sys.argv[3]))
PYEOF
}

list_envs() { # $1 = platformio.ini
  grep -oE '^\[env:[^]]+\]' "$1" | sed 's/^\[env://; s/\]$//'
}

# PlatformIO's board_build.partitions -> Arduino's PartitionScheme menu option. Without this
# the board's default (often only 1.2MB of app space) is used and a big sketch overflows with
# "text section exceeds available space", which does not obviously point at partitions.
scheme_for_partitions() {
  case "$1" in
    min_spiffs.csv)  echo "min_spiffs" ;;
    huge_app.csv)    echo "huge_app" ;;
    no_ota.csv)      echo "no_ota" ;;
    noota_3g.csv)    echo "noota_3g" ;;
    noota_ffat.csv)  echo "noota_ffat" ;;
    default.csv)     echo "default" ;;
    *)               echo "" ;;
  esac
}

value_for_env() { # $1 = platformio.ini  $2 = env  $3 = key
  awk -v want="[env:$2]" -v key="$3" '
    /^\[/ { hdr = $0; sub(/[[:space:]]*;.*$/, "", hdr); gsub(/[[:space:]]+$/, "", hdr);
             inenv = (hdr == want); next }
    inenv && index($0, key) {
      sub(/^[^=]*=[[:space:]]*/, ""); sub(/[[:space:]]*;.*$/, ""); gsub(/[[:space:]]+$/, "");
      print; exit
    }' "$1"
}

board_for_env() { # $1 = platformio.ini  $2 = env name
  awk -v want="[env:$2]" '
    /^\[/ { hdr = $0; sub(/[[:space:]]*;.*$/, "", hdr); gsub(/[[:space:]]+$/, "", hdr);
             inenv = (hdr == want); next }
    inenv && /^[[:space:]]*board[[:space:]]*=/ {
      sub(/^[[:space:]]*board[[:space:]]*=[[:space:]]*/, "");
      sub(/[[:space:]]*;.*$/, "");
      gsub(/[[:space:]]+$/, "");
      print; exit
    }' "$1"
}

# ── dependency helpers ──────────────────────────────────────────────────────────────────────
require_arduino_cli() {
  if ! command -v arduino-cli >/dev/null 2>&1; then
    echo "arduino-cli is not installed. It is the Arduino toolchain without the GUI." >&2
    echo "  macOS:  brew install arduino-cli" >&2
    echo "  else:   https://arduino.github.io/arduino-cli/latest/installation/" >&2
    exit 1
  fi
}

install_core() { # $1 = packager:arch e.g. esp32:esp32
  echo "== installing core $1 (the slow part - the ESP32 core is >1GB)"
  arduino-cli config init --overwrite >/dev/null 2>&1
  if [ "${1#*:}" = "esp8266" ]; then
    arduino-cli config add board_manager.additional_urls \
      https://arduino.esp8266.com/stable/package_esp8266com_index.json
  fi
  arduino-cli core update-index || exit 1
  arduino-cli core install "$1" || exit 1
}

install_libs() {
  # The libraries the Arduino IDE would install via Library Manager, from library.properties
  echo "== installing libraries from library.properties"
  DEPENDS="$(grep '^depends=' "$LIB_ROOT/library.properties" | cut -d= -f2-)"
  echo "$DEPENDS" | tr ',' '\n' | sed 's/^ *//; s/ *$//' | while read -r libname; do
    [ -n "$libname" ] && arduino-cli lib install "$libname"
  done
}

# ── --list ──────────────────────────────────────────────────────────────────────────────────
if [ "$DO_LIST" = 1 ]; then
  for d in "$EXAMPLES_DIR"/*/; do
    name="$(basename "$d")"
    if [ -f "$d/platformio.ini" ]; then
      printf "%-24s %s\n" "$name" "$(list_envs "$d/platformio.ini" | tr '\n' ' ')"
    fi
  done
  exit 0
fi

# --update and --install-deps are maintenance actions that do not need an example or an env,
# so handle them before the usage check and exit if that is all that was asked for.
if [ "$UPDATE_DEPS" = 1 ]; then
  require_arduino_cli
  # Only downloads what actually changed, unlike --install-deps
  echo "== refreshing package indexes"
  arduino-cli update || exit 1
  echo "== upgrading installed cores and libraries"
  arduino-cli upgrade || exit 1
  DID_MAINTENANCE=1
  echo
fi

if [ "$INSTALL_DEPS" = 1 ] && { [ -z "$EXAMPLE" ] || [ -z "$ENVNAME" ]; }; then
  # No example/env to tell us which core, so install both platforms this library supports
  require_arduino_cli
  echo "== no example/env given, so installing both supported cores"
  install_core esp32:esp32
  install_core esp8266:esp8266
  install_libs
  DID_MAINTENANCE=1
  echo
  INSTALL_DEPS=0
fi

# Exit only if we actually DID some maintenance and there is nothing to compile. Guarding on
# DID_MAINTENANCE rather than on the flags: with no arguments at all both are 0, and an earlier
# version of this test exited 0 silently instead of printing the usage.
if [ "$DID_MAINTENANCE" = 1 ] && [ -z "$EXAMPLE" ] && [ -z "$ENVNAME" ]; then
  exit 0
fi

if [ -z "$EXAMPLE" ] || [ -z "$ENVNAME" ]; then
  echo "usage: ./arduino_compile.bash <example> <env> [--install-deps|--update] [--clean]" >&2
  echo "       ./arduino_compile.bash --update    # refresh indexes, upgrade cores and libraries" >&2
  echo "       ./arduino_compile.bash --install-deps   # install both cores and all libraries" >&2
  echo "       ./arduino_compile.bash --list      # show examples and their environments" >&2
  exit 2
fi

SKETCH_DIR="$EXAMPLES_DIR/$EXAMPLE"
PIO_INI="$SKETCH_DIR/platformio.ini"

if [ ! -d "$SKETCH_DIR" ]; then
  echo "No such example: $EXAMPLE" >&2
  echo "Available: $(ls "$EXAMPLES_DIR" | tr '\n' ' ')" >&2
  exit 1
fi
if [ ! -f "$PIO_INI" ]; then
  echo "$EXAMPLE has no platformio.ini" >&2; exit 1
fi
if [ ! -f "$SKETCH_DIR/$EXAMPLE.ino" ]; then
  echo "$EXAMPLE has no $EXAMPLE.ino - Arduino requires the sketch to match its directory name" >&2
  exit 1
fi

BOARD="$(board_for_env "$PIO_INI" "$ENVNAME")"
if [ -z "$BOARD" ]; then
  echo "No [env:$ENVNAME] with a 'board =' in $PIO_INI" >&2
  echo "Environments in $EXAMPLE: $(list_envs "$PIO_INI" | tr '\n' ' ')" >&2
  exit 1
fi

FQBN="$(fqbn_for_env "$ENVNAME")"
[ -z "$FQBN" ] && FQBN="$(fqbn_for_board "$BOARD")"
if [ -z "$FQBN" ]; then
  echo "No Arduino FQBN known for PlatformIO board '$BOARD'." >&2
  echo "Add it to fqbn_for_board() in $SELF - guessing would risk compiling for the wrong chip." >&2
  exit 1
fi
# First two colon-separated fields; the rest may be a board id plus :Menu=Option pairs
CORE="$(echo "$FQBN" | cut -d: -f1,2)"

# Captured once - each arduino-cli invocation costs real startup time. Taken on the FQBN as
# resolved from env/board (which may already carry :Revision=...) but before any
# :PartitionScheme= suffix is appended; build.board does not depend on PartitionScheme.
BOARD_PROPS="$(arduino-cli board details -b "$FQBN" --show-properties 2>/dev/null)"

# Carry board_build.partitions across. Two routes, because several boards (lolin_c3_pico and
# nodemcu-32s among them) ship NO min_spiffs menu option even though the core ships the CSV:
#   1. the board offers a matching PartitionScheme menu option  -> select it, exactly as an
#      Arduino IDE user would from Tools > Partition Scheme
#   2. it does not -> override build.partitions directly, plus upload.maximum_size taken from
#      the app partition in the core's own CSV, or the size check still uses the board default
PARTITIONS="$(value_for_env "$PIO_INI" "$ENVNAME" "board_build.partitions")"
PART_NOTE=""
BUILD_PROPS=()
if [ "${CORE%%:*}" != "esp32" ]; then
  # ESP8266 has no PartitionScheme concept - its layout comes from the flash-size menu
  [ -n "$PARTITIONS" ] && PART_NOTE="board_build.partitions=$PARTITIONS ignored - not an ESP32 board"
  # No include-path workaround needed: _settings.h guards its platform.h include with
  # __has_include (unreachable here, since this core does not put the sketch dir on the include
  # path), and the defines arrive instead from <sketch>.ino.globals.h, which this core
  # force-includes into every translation unit. Verified building sht30/d1_mini_pro with no
  # --build-property at all.
elif [ -n "$PARTITIONS" ]; then
  SCHEME="${PARTITIONS%.csv}"
  if echo "$BOARD_PROPS" | grep -q "^menu\.PartitionScheme\.$SCHEME="; then
    FQBN="$FQBN:PartitionScheme=$SCHEME"
    PART_NOTE="$PARTITIONS -> menu PartitionScheme=$SCHEME"
  else
    DATA_DIR="$(arduino-cli config get directories.data 2>/dev/null)"
    CSV="$(ls "$DATA_DIR"/packages/esp32/hardware/esp32/*/tools/partitions/"$PARTITIONS" 2>/dev/null | tail -1)"
    if [ -n "$CSV" ]; then
      # Largest app partition in the CSV - that is what upload.maximum_size must reflect.
      # Sizes are hex, and BSD awk has no strtonum(), so convert in the shell.
      APPHEX="$(awk -F, '$2 ~ /app/ { gsub(/[[:space:]]/,"",$5); print $5 }' "$CSV" | sort | tail -1)"
      APPMAX=$(( APPHEX ))
      if [ -n "$APPMAX" ] && [ "$APPMAX" -gt 0 ]; then
        BUILD_PROPS+=(--build-property "build.partitions=$SCHEME")
        BUILD_PROPS+=(--build-property "upload.maximum_size=$APPMAX")
        PART_NOTE="$PARTITIONS -> build.partitions=$SCHEME, max app $APPMAX bytes"
        PART_NOTE="$PART_NOTE
          (this board has no PartitionScheme=$SCHEME menu option, so an Arduino IDE user must
          instead pick a big-enough scheme from Tools > Partition Scheme - see CLAUDE.md)"
      fi
    fi
    if [ ${#BUILD_PROPS[@]} -eq 0 ]; then
      PART_NOTE="could not honour board_build.partitions=$PARTITIONS - using the board default,
          which may be too small. Available: $(echo "$BOARD_PROPS" \
          | sed -n 's/^menu\.PartitionScheme\.\([a-z_0-9]*\)=.*/\1/p' | sort -u | tr '\n' ' ')"
    fi
  fi
fi

echo "example : $EXAMPLE  ($SKETCH_DIR)"
echo "env     : $ENVNAME"
echo "board   : $BOARD  ->  $FQBN"
echo "library : $LIB_ROOT  (the working tree, not ~/Documents/Arduino/libraries)"
[ -n "$PART_NOTE" ] && echo "parts   : $PART_NOTE"
echo

# ── arduino-cli ─────────────────────────────────────────────────────────────────────────────
require_arduino_cli

if [ "$INSTALL_DEPS" = 1 ]; then
  install_core "$CORE"
  install_libs
fi

if ! arduino-cli core list 2>/dev/null | awk 'NR>1 {print $1}' | grep -qx "$CORE"; then
  echo "Arduino core '$CORE' is not installed." >&2
  echo "Run again with --install-deps, or: arduino-cli core install $CORE" >&2
  exit 1
fi

# ── platform.h ──────────────────────────────────────────────────────────────────────────────
# _settings.h does #include "platform.h" when PLATFORMIO is undefined, so an Arduino build
# needs it present and current. Regenerate rather than trust what is checked in.
if [ ! -f "$SCRIPTS_DIR/generate_platform_h.py" ]; then
  echo "generate_platform_h.py missing from $SCRIPTS_DIR" >&2; exit 1
fi
echo "== regenerating platform.h from platformio.ini"
( cd "$SKETCH_DIR" && python3 "$SCRIPTS_DIR/generate_platform_h.py" ) || exit 1

# ── sanity checks that a green build would otherwise hide ───────────────────────────────────
WARNINGS=0

# Extra .cpp in the sketch folder that also defines setup() - Arduino compiles EVERY source
# file in the sketch directory, so this becomes a duplicate-symbol link error.
for extra in "$SKETCH_DIR"/*.cpp; do
  [ -e "$extra" ] || continue
  if grep -q "void setup()" "$extra" 2>/dev/null; then
    echo "WARNING: $(basename "$extra") also defines setup() - Arduino builds every source file"
    echo "         in the sketch folder, so this collides with $EXAMPLE.ino. Delete or rename it."
    WARNINGS=$((WARNINGS + 1))
  fi
done

# Does the core actually define the macro platform.h guards this env with?
GUARD="$(guard_for_env "$BOARD" "$ENVNAME")"
BUILD_BOARD="$(echo "$BOARD_PROPS" | grep '^build\.board=' | head -1 | cut -d= -f2- | tr -d '\r')"
if [ -n "$BUILD_BOARD" ]; then
  CORE_MACRO="ARDUINO_$BUILD_BOARD"
  if [ "$GUARD" = "$CORE_MACRO" ]; then
    echo "== board guard OK: platform.h uses $GUARD, which $FQBN defines"
  elif grep -qE "^[[:space:]]*#[[:space:]]*define[[:space:]]+$GUARD([[:space:]]|\$)" "$LIB_ROOT/src/_settings.h"; then
    # _settings.h's TO_ADD_BOARD block aliases the Arduino macro to PlatformIO's spelling before
    # it includes platform.h, so a guard it defines is fine even though it is not what the core
    # emits. Grepping for it rather than keeping a copy of the table here.
    echo "== board guard OK: platform.h uses $GUARD; core defines $CORE_MACRO, and _settings.h"
    echo "   normalises one to the other (TO_ADD_BOARD block)"
  else
    echo "WARNING: platform.h guards [env:$ENVNAME] with '$GUARD', but the Arduino core defines"
    echo "         '$CORE_MACRO' for $FQBN, and _settings.h does not alias between them. That"
    echo "         #ifdef never fires, so none of this env's flags reach the build and the sketch"
    echo "         compiles with library defaults only. Fix by adding an alias to the"
    echo "         TO_ADD_BOARD block in _settings.h, or by correcting generate_platform_h.py."
    WARNINGS=$((WARNINGS + 1))
  fi
fi
echo

# ── compile ─────────────────────────────────────────────────────────────────────────────────
# A stable path per example+board, so a repeat run is incremental rather than a full rebuild
# (which for an ESP32 sketch this size is minutes). --clean forces a fresh one.
BUILD_KEY="$(echo "${EXAMPLE}_${FQBN}" | tr -c 'A-Za-z0-9_' '_')"
BUILD_DIR="${TMPDIR:-/tmp}/arduino_compile/$BUILD_KEY"
if [ "$CLEAN_BUILD" = 1 ]; then
  rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"
echo "== compiling"
arduino-cli compile \
  --fqbn "$FQBN" \
  --library "$LIB_ROOT" \
  "${BUILD_PROPS[@]}" \
  --build-path "$BUILD_DIR" \
  --warnings default \
  "$SKETCH_DIR"
STATUS=$?

echo
if [ "$STATUS" = 0 ]; then
  echo "RESULT: $EXAMPLE / $ENVNAME compiled for Arduino ($FQBN)"
  if [ "$WARNINGS" -gt 0 ]; then
    echo "        ...but with $WARNINGS warning(s) above - read them, a green build here may not"
    echo "        mean what you think it does."
  fi
else
  echo "RESULT: $EXAMPLE / $ENVNAME FAILED to compile for Arduino ($FQBN)"
fi

# Kept by default so the next run is incremental; --clean above discards it first
echo "build dir: $BUILD_DIR  (reused next time; --clean to discard)"
exit $STATUS
