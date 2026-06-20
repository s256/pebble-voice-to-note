# Voice to Note — Pebble Time 2

Dictate voice notes on your Pebble Time 2 and send them as HTTP requests to n8n (or any webhook).

Uses the Pebble Dictation API with **local offline transcription** (parakeet-tdt-0.6b-v3 model on your phone) — no cloud dependency.

## How it works

```
Watch (C)  ──dictation──►  Phone (local STT)  ──AppMessage──►  PebbleKit JS  ──HTTP──►  n8n/webhook
```

1. Press SELECT on the watch → speak
2. Phone transcribes speech locally using the offline model
3. Transcribed text is sent back to the watch via AppMessage
4. PebbleKit JS sends an HTTP POST (configurable) to your webhook
5. Watch shows success/error status

## Configuration

Open the app settings in the Pebble companion app to configure:

- **Webhook URL** — your n8n webhook endpoint
- **HTTP Method** — POST (default), PUT, PATCH, or GET
- **Headers** — JSON object (default: `{"Content-Type": "application/json"}`)
- **Body Template** — use `{{note}}` as placeholder for the transcription (default: `{"note": "{{note}}"}`)

## Prerequisites

- [Pebble SDK](https://developer.rebble.io/sdk/) (pebble-tool via uv)
- Pebble Time 2 (or emulator with `emery` platform)
- Phone with Pebble app configured for **local dictation** (Settings → Voice Language → download offline model)

## Quick Start

```bash
# Install SDK
uv tool install pebble-tool --python 3.13
pebble sdk install latest

# Build
pebble build

# Run in emulator
./scripts/dev.sh

# Test dictation in emulator
pebble transcribe "Buy milk and eggs"

# Install on real watch
pebble install --phone <YOUR_PHONE_IP>
```

## Project Structure

```
voice-to-note/
├── src/
│   ├── c/
│   │   └── main.c           # Watch app: UI + dictation + AppMessage
│   └── pkjs/
│       └── index.js          # Phone: HTTP sender + config storage
├── config/
│   └── index.html            # Settings page (hosted on GitHub Pages)
├── resources/
│   └── images/
│       └── mic.png           # App icon
├── scripts/
│   ├── dev.sh                # Build + emulator install
│   └── release.sh            # Build release .pbw
├── package.json              # App metadata + message keys
└── wscript                   # Pebble WAF build script
```

## Config Page Hosting

The configuration HTML is hosted on GitHub Pages. After pushing:

1. Go to repo Settings → Pages → Source: Deploy from branch `main`, folder `/config`
2. Your config URL: `https://<username>.github.io/voice-to-note/config/index.html`
3. Update the URL in `src/pkjs/index.js` (`showConfiguration` handler)

## Emulator Notes

The emulator has no microphone. Use `pebble transcribe "your text"` to simulate dictation results. Real speech recognition requires a physical Pebble Time 2 paired with a phone.

## License

MIT
