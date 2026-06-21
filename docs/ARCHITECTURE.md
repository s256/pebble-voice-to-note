# Architecture — Voice to Note

## Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           Pebble Time 2                                  │
│                                                                         │
│  ┌───────────────┐     ┌────────────────┐     ┌──────────────────────┐ │
│  │  main.c (C)   │────▶│  Dictation API │────▶│ PebbleOS STT Engine  │ │
│  │               │◀────│  (on-watch UI) │◀────│ (routes to phone)    │ │
│  └───────┬───────┘     └────────────────┘     └──────────────────────┘ │
│          │ AppMessage                                                    │
└──────────┼──────────────────────────────────────────────────────────────┘
           │ Bluetooth
┌──────────┼──────────────────────────────────────────────────────────────┐
│          ▼                            Phone                              │
│  ┌───────────────┐     ┌────────────────────────────────────────────┐  │
│  │ PebbleKit JS  │     │ Pebble App (Core Devices)                   │  │
│  │ (index.js)    │     │                                             │  │
│  │               │     │  Speech Recognition Engine                   │  │
│  │ • recv text   │     │  ├─ Local: parakeet-tdt-0.6b-v3 (offline)  │  │
│  │ • HTTP POST   │     │  └─ Cloud: Wispr Flow (fallback)           │  │
│  │ • config mgmt │     │                                             │  │
│  └───────┬───────┘     └────────────────────────────────────────────┘  │
│          │ XMLHttpRequest                                                │
└──────────┼──────────────────────────────────────────────────────────────┘
           │ HTTPS
           ▼
┌──────────────────┐
│  n8n Webhook     │
│  (or any HTTP)   │
│                  │
│  POST /webhook   │
│  {"note": "..."}│
└──────────────────┘
```

## Data Flow

1. **User presses SELECT** → `select_click_handler()` in main.c
2. **Dictation starts** → `dictation_session_start()` opens PebbleOS dictation UI
3. **User speaks** → audio streams from watch mic to phone via Bluetooth
4. **Phone transcribes** → parakeet-tdt-0.6b-v3 model (local, offline)
5. **Text returns** → `dictation_session_callback()` receives transcription
6. **C sends to JS** → `dict_write_cstring()` via AppMessage to PebbleKit JS
7. **JS sends HTTP** → XMLHttpRequest to configured webhook URL
8. **Result returns** → JS sends STATUS back to C → watch shows "Sent!" or error

## Layers

### Watch Layer (C) — `src/c/main.c`

Responsibilities:
- UI (TextLayer for prompt + status)
- Dictation session lifecycle
- AppMessage send (transcription) and receive (status codes)
- User feedback (vibration, status text)

Key state:
- `s_js_ready` — whether PebbleKit JS has connected
- `s_transcription_buffer` — last transcription (1024 bytes)
- `s_dictation_session` — the active dictation session handle

### Phone Layer (JS) — `src/pkjs/index.js`

Responsibilities:
- Receive transcription from watch via `appmessage` event
- Read webhook config from `localStorage`
- Make HTTP request with configurable method/headers/body
- Send status code back to watch
- Manage configuration page lifecycle

Key data:
- `localStorage['voice_to_note_config']` — persisted settings JSON
- Config fields: `url`, `method`, `headers`, `body`

### Configuration Layer (HTML) — `config/index.html`

Responsibilities:
- Present settings form to user
- Load existing config from URL params
- Validate input (URL required, headers must be valid JSON)
- Return config via `pebblejs://close#` URI scheme

Hosted on GitHub Pages. Opened by the Pebble companion app when user taps
the settings gear for this app.

## Communication Protocol

### Message Keys (defined in `package.json`)

| Index | Key | Direction | Type | Purpose |
|---|---|---|---|---|
| 0 | TRANSCRIPTION | C → JS | string | Transcribed text to send |
| 1 | STATUS | JS → C | int | 1=ready, 200=sent, other=HTTP error |
| 2 | CONFIG_URL | — | — | Reserved for future watch-side config |
| 3 | CONFIG_METHOD | — | — | Reserved |
| 4 | CONFIG_HEADERS | — | — | Reserved |
| 5 | CONFIG_BODY | — | — | Reserved |

### AppMessage Buffer Sizes

| Direction | Size | Rationale |
|---|---|---|
| Inbox (JS → C) | 2048 bytes | Room for config strings if needed |
| Outbox (C → JS) | 1024 bytes | Must fit longest transcription + overhead |

### Status Codes

| Code | Meaning | Watch Response |
|---|---|---|
| 1 | JS ready | "Ready — press SELECT" |
| 200 | HTTP success | "Sent!" + short vibration |
| 0 | No URL / network error | "HTTP error: 0" |
| 4xx/5xx | HTTP error | "HTTP error: {code}" + double vibration |

## Configuration Flow

```
User taps gear icon in Pebble app
         │
         ▼
showConfiguration event fires in PebbleKit JS
         │
         ▼
Pebble.openURL(githubPagesUrl + ?config=encodedJSON)
         │
         ▼
Config page loads in phone webview, pre-fills form
         │
         ▼
User edits settings, taps "Save"
         │
         ▼
Page redirects to: pebblejs://close#encodedJSON
         │
         ▼
webviewclosed event fires in PebbleKit JS
         │
         ▼
JS parses response, saves to localStorage
```

## Speech Recognition

The Dictation API is **transparent** to our app. We call `dictation_session_start()`
and receive text back. The actual STT processing is handled by the Pebble companion
app on the phone, which offers:

| Mode | Engine | Network Required | Accuracy |
|---|---|---|---|
| Local Only | parakeet-tdt-0.6b-v3 (715MB) | No | High |
| Cloud Only | Wispr Flow | Yes | Higher |
| Local + Cloud Fallback | Both | Preferred | Best |

The user configures this in the Pebble app's Speech Recognition settings,
not within our app. Our app works identically regardless of which mode is active.

## Constraints

- **DictationSession timeout**: PebbleOS caps a single recording at ~15 seconds
- **AppMessage size**: max ~2048 bytes per message (platform limit)
- **Transcription length**: limited by DICTATION_BUFFER_SIZE (1024 bytes)
- **No raw audio access**: only transcribed text is available via the Dictation API
- **Watch has no internet**: all HTTP goes through PebbleKit JS on the phone
- **PebbleKit JS is ES5 only**: no arrow functions, const/let, promises, fetch()
- **Config page must use HTTPS**: required by the webview
- **Firmware requirement**: PebbleOS ≥ 4.9.158 (earlier has dictation bug)
