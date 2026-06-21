#!/usr/bin/env bash
set -e

# Dev script: build and install on emulator
# Usage: ./scripts/dev.sh [platform]
# Default platform: emery (Pebble Time 2)

PLATFORM="${1:-emery}"

# Fedora: ensure /usr/local/lib is in library path (sndio, libbz2 compat)
export LD_LIBRARY_PATH="/usr/local/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

echo "Building voice-to-note for $PLATFORM..."
pebble build

echo "Installing on emulator ($PLATFORM) with logs..."
echo ""
echo "NOTE: On emery, AppMessage JS→C does not work in the emulator"
echo "      (known pypkjs bug). The app WILL work on a real watch."
echo ""
echo "To test dictation: pebble transcribe 'test note' (in another terminal)"
echo "To test on watch:  pebble install --logs --phone <PHONE_IP>"
echo ""
pebble install --emulator "$PLATFORM" --logs
