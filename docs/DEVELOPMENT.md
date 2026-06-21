# Development Workflow

## Build

```bash
pebble build
```

Output: `build/voice-to-note.pbw`

The build compiles `src/c/main.c` with the ARM cross-compiler for the `emery`
platform and bundles `src/pkjs/index.js` into the `.pbw` package.

## Run in Emulator

```bash
pebble install --emulator emery
```

Or use the dev script:

```bash
./scripts/dev.sh
```

## Emulator Key Bindings

| Watch Button | Keyboard |
|---|---|
| Back | Q / Left Arrow |
| Up | W / Up Arrow |
| Select | S / Right Arrow |
| Down | X / Down Arrow |

## Test Dictation in Emulator

The emulator has no microphone. Use `pebble transcribe` to simulate:

```bash
# Successful transcription
pebble transcribe "Buy milk and eggs"

# Then press SELECT in the emulator to trigger dictation
# The fake transcription server responds with "Buy milk and eggs"
```

To simulate dictation errors:

```bash
pebble transcribe --error connectivity
pebble transcribe --error disabled
pebble transcribe --error no-speech-detected
```

**Important:** Run `pebble transcribe` BEFORE triggering dictation in the app.
The command starts a local transcription server that the emulator connects to.

## View Logs

### All logs (C + JS):

```bash
pebble logs --emulator emery
```

### On a real watch:

```bash
pebble logs --phone 192.168.1.42
```

### What to look for:

- `[INFO]` lines from the C app (via `APP_LOG`)
- `[JS]` lines from PebbleKit JS (via `console.log`)
- AppMessage send/receive confirmations
- HTTP request/response status codes

## Test Configuration Page

### In emulator (local HTML file):

```bash
pebble emu-app-config --file config/index.html
```

This opens the config page directly without needing GitHub Pages.

### On phone:

The config page opens when you tap the settings gear for the app in the Pebble
companion app. It loads from the GitHub Pages URL configured in `src/pkjs/index.js`.

## Debug AppMessage Communication

1. Start logs: `pebble logs --emulator emery`
2. Watch for these events:
   - `PebbleKit JS ready` — JS layer loaded successfully
   - `Status 1 sent to watch` — JS→C "ready" handshake
   - `Received message: {...}` — C→JS transcription received
   - `HTTP 200: ...` — webhook responded successfully

### Common Issues:

| Symptom | Cause | Fix |
|---|---|---|
| "Connecting..." stays forever | JS not loading | Check `pebble logs` for JS syntax errors |
| "Phone not ready" | JS ready signal not received | Ensure `sendStatus(1)` fires in `ready` event |
| "Send failed" | AppMessage outbox full | Wait for previous message to complete |
| "HTTP error: 0" | No URL configured / network error | Configure webhook URL in settings |

## Install on Real Watch

```bash
# Via phone IP (Developer Connection method)
pebble install --phone 192.168.1.42

# Via cloudpebble (GitHub auth method)
pebble login
pebble install --cloudpebble

# With logs attached
pebble install --logs --phone 192.168.1.42
```

## Build a Release

```bash
./scripts/release.sh
```

Produces `build/voice-to-note.pbw` which can be sideloaded via the Pebble app.

## Clean Build

```bash
pebble clean
pebble build
```

## GDB Debugging (Emulator Only)

```bash
pebble gdb
```

Useful commands:
- `b main` — break at main()
- `b dictation_session_callback` — break when transcription arrives
- `c` — continue execution
- `bt` — backtrace
- `p s_transcription_buffer` — print buffer contents

## Project Files You'll Edit Most

| File | What | When |
|---|---|---|
| `src/c/main.c` | Watch UI + dictation | Changing watch behavior |
| `src/pkjs/index.js` | HTTP + config logic | Changing webhook behavior |
| `config/index.html` | Settings page | Adding config options |
| `package.json` | App metadata + message keys | Adding new AppMessage keys |
