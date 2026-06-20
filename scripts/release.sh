#!/usr/bin/env bash
set -e

echo "Building release .pbw..."
pebble build

PBW="build/voice-to-note.pbw"
if [ -f "$PBW" ]; then
  echo "Release built: $PBW"
  echo "Sideload this file via the Pebble companion app."
else
  echo "ERROR: Build output not found at $PBW"
  exit 1
fi
