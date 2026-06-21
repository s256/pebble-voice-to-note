#!/usr/bin/env bash
set -e

# Dev script: build and install on emulator or phone
# Usage:
#   ./scripts/dev.sh              — build + emulator (emery)
#   ./scripts/dev.sh phone        — build + install on phone via Dev Connect
#   ./scripts/dev.sh phone <IP>   — build + install on phone at specific IP
#
# Env vars:
#   PEBBLE_PHONE  — default phone IP (avoids typing it each time)
#   PLATFORM      — emulator platform (default: emery)

PLATFORM="${PLATFORM:-emery}"
PHONE_IP="${PEBBLE_PHONE:-192.168.0.148}"

# Fedora: ensure /usr/local/lib is in library path (sndio, libbz2 compat)
export LD_LIBRARY_PATH="/usr/local/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

echo "Building voice-to-note for $PLATFORM..."
pebble build

if [[ "${1:-}" == "phone" ]]; then
  # Use provided IP or default
  if [[ -n "${2:-}" ]]; then
    PHONE_IP="$2"
  fi
  echo "Installing on phone ($PHONE_IP)..."
  echo ""
  echo "Make sure the Pebble app is open with Dev Connect enabled."
  echo ""
  pebble install --phone "$PHONE_IP" --logs
else
  echo "Installing on emulator ($PLATFORM) with logs..."
  echo ""
  echo "NOTE: On emery, AppMessage JS→C does not work in the emulator"
  echo "      (known pypkjs bug). The app WILL work on a real watch."
  echo ""
  echo "To test dictation: pebble transcribe 'test note' (in another terminal)"
  echo ""
  pebble install --emulator "$PLATFORM" --logs
fi
