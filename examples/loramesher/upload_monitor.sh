#!/usr/bin/env bash
# Build, upload, and monitor both gateway_tbeam and node_tbeam in parallel.
# Each device's serial output goes to its own timestamped log file.
#
# Phases:
#   1. pio run  (compile)  — both envs in parallel
#   2. pio run -t upload   — both envs in parallel (each to its own port)
#   3. pio device monitor  — both in parallel, timestamped, per-device logs
#
# Usage:
#   ./upload_monitor.sh [gateway_port] [node_port]
# Defaults:
#   gateway_port=/dev/ttyUSB0  node_port=/dev/ttyUSB1
#
# Env overrides:
#   SKIP_UPLOAD=1   only monitor (skip phases 1 and 2)
#   BAUD=460800     monitor baud rate (must match firmware's startSerial)
#
# Ctrl-C stops everything cleanly.

set -u

# GATEWAY_ENV="gateway_tbeam"
# NODE_ENV="node_tbeam"
GATEWAY_ENV="gateway_heltec"
NODE_ENV="node_heltec"
GATEWAY_PORT="${1:-/dev/ttyUSB0}"
NODE_PORT="${2:-/dev/ttyUSB1}"
BAUD="${BAUD:-460800}"
TS=$(date +%Y%m%d_%H%M%S)

cd "$(dirname "$0")"
LOGS_DIR="logs/${TS}"
mkdir -p "$LOGS_DIR"
GW_LOG="${LOGS_DIR}/loramesher_gw.log"
ND_LOG="${LOGS_DIR}/loramesher_nd.log"
GW_BUILD_LOG="${LOGS_DIR}/build_gw.log"
ND_BUILD_LOG="${LOGS_DIR}/build_nd.log"
GW_UPLOAD_LOG="${LOGS_DIR}/upload_gw.log"
ND_UPLOAD_LOG="${LOGS_DIR}/upload_nd.log"

echo "Session logs: $LOGS_DIR"
echo "Gateway: $GATEWAY_ENV on $GATEWAY_PORT"
echo "Node:    $NODE_ENV on $NODE_PORT"
echo "Baud:    $BAUD"

# kill 0 hits the entire process group - the script and every child pio spawned.
cleanup() {
    trap - INT TERM EXIT
    echo ""
    echo "--- Stopping ---"
    kill 0 2>/dev/null
    exit 0
}
trap cleanup INT TERM

if [ "${SKIP_UPLOAD:-0}" != "1" ]; then
    echo "--- Phase 1: compile both envs in parallel ---"
    pio run -e "$GATEWAY_ENV" > "$GW_BUILD_LOG" 2>&1 & GW_BUILD=$!
    pio run -e "$NODE_ENV"    > "$ND_BUILD_LOG" 2>&1 & ND_BUILD=$!
    wait $GW_BUILD; GW_RC=$?
    wait $ND_BUILD; ND_RC=$?
    if [ $GW_RC -ne 0 ]; then echo "Gateway build failed — see $GW_BUILD_LOG"; exit 1; fi
    if [ $ND_RC -ne 0 ]; then echo "Node build failed — see $ND_BUILD_LOG";    exit 1; fi
    echo "Both builds succeeded."

    echo "--- Phase 2: upload both in parallel ---"
    pio run -e "$GATEWAY_ENV" -t upload --upload-port "$GATEWAY_PORT" > "$GW_UPLOAD_LOG" 2>&1 & GW_UP=$!
    pio run -e "$NODE_ENV"    -t upload --upload-port "$NODE_PORT"    > "$ND_UPLOAD_LOG" 2>&1 & ND_UP=$!
    wait $GW_UP; GW_RC=$?
    wait $ND_UP; ND_RC=$?
    if [ $GW_RC -ne 0 ]; then echo "Gateway upload failed — see $GW_UPLOAD_LOG"; exit 1; fi
    if [ $ND_RC -ne 0 ]; then echo "Node upload failed — see $ND_UPLOAD_LOG";    exit 1; fi
fi

echo "$(date '+[%Y-%m-%d %H:%M:%S.%3N]') # session started on $GATEWAY_PORT @ ${BAUD}" >> "$GW_LOG"
echo "$(date '+[%Y-%m-%d %H:%M:%S.%3N]') # session started on $NODE_PORT @ ${BAUD}"    >> "$ND_LOG"

echo "--- Phase 3: monitoring (Ctrl-C to stop) ---"

# Each pipeline: pio monitor -> timestamp each line -> tee to per-device log -> prefix tag for terminal.
(pio device monitor --port "$GATEWAY_PORT" -b "$BAUD" 2>&1 \
    | while IFS= read -r line; do printf '%s %s\n' "$(date '+[%Y-%m-%d %H:%M:%S.%3N]')" "$line"; done \
    | tee -a "$GW_LOG" \
    | sed -u 's/^/[GW] /') &

(pio device monitor --port "$NODE_PORT" -b "$BAUD" 2>&1 \
    | while IFS= read -r line; do printf '%s %s\n' "$(date '+[%Y-%m-%d %H:%M:%S.%3N]')" "$line"; done \
    | tee -a "$ND_LOG" \
    | sed -u 's/^/[ND] /') &

wait
