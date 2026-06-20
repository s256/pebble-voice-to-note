#!/usr/bin/env bash
set -e

# Dev script: build and install on emulator
# Usage: ./scripts/dev.sh [platform]
# Default platform: emery (Pebble Time 2)

PLATFORM="${1:-emery}"

echo "Building voice-to-note for $PLATFORM..."
pebble build

echo "Installing on emulator ($PLATFORM)..."
pebble install --emulator "$PLATFORM"

echo ""
echo "App installed. To test dictation in emulator:"
echo "  pebble transcribe 'Hello this is a test note'"
echo ""
echo "To test with real watch:"
echo "  pebble install --phone <YOUR_PHONE_IP>"
