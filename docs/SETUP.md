# Setup Guide — Voice to Note for Pebble Time 2

Complete walkthrough from zero to running app.

## Prerequisites

- Linux (this guide covers Fedora; Ubuntu instructions noted where different)
- A Pebble Time 2 watch (optional for emulator testing, required for real dictation)
- An Android phone with the Pebble app (for watch deployment)
- Internet connection (for SDK download only; the app itself works offline)

## Step 1: Install System Dependencies

### Fedora

```bash
sudo dnf install nodejs SDL2 glib2 pixman zlib
```

### Ubuntu / Debian

```bash
sudo apt install nodejs npm libsdl2-2.0-0 libglib2.0-0 libpixman-1-0 zlib1g libsndio7.0
```

These are needed by the Pebble emulator (QEMU-based).

**Fedora also requires sndio** (not in repos, build from source):

```bash
sudo dnf install alsa-lib-devel   # build dependency
cd /tmp
curl -LO https://sndio.org/sndio-1.10.0.tar.gz
tar xzf sndio-1.10.0.tar.gz
cd sndio-1.10.0
./configure && make
sudo make install

# Add /usr/local/lib to linker path
echo "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/local.conf
sudo ldconfig

# Also fix bz2 version mismatch (QEMU wants libbz2.so.1.0)
sudo ln -sf /usr/lib64/libbz2.so.1.0.8 /usr/local/lib/libbz2.so.1.0
sudo ldconfig
```

## Step 2: Install uv (Python Package Manager)

```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
```

Or on Fedora:

```bash
sudo dnf install uv
```

Verify:

```bash
uv --version
```

## Step 3: Install pebble-tool

```bash
uv tool install pebble-tool
```

This installs the `pebble` CLI globally. Requires Python 3.10+.

Verify:

```bash
pebble --version
```

Expected output: something like `pebble-tool 5.0.x`.

## Step 4: Install the Pebble SDK

```bash
pebble sdk install latest
```

This downloads the ARM cross-compiler and QEMU emulator (~500MB). Takes a few minutes.

Verify:

```bash
pebble sdk list
```

You should see the installed SDK version.

## Step 5: Build the App

From this project's root directory:

```bash
pebble build
```

Expected output:
- Compilation of `src/c/main.c` for the `emery` platform
- Bundling of `src/pkjs/index.js`
- Output: `build/voice-to-note.pbw`

If it fails, check:
- Are you in the project root (where `package.json` is)?
- Is the SDK installed? (`pebble sdk list`)
- Missing system deps? (re-run Step 1)

## Step 6: Test in the Emulator

```bash
pebble install --emulator emery
```

This launches the Pebble Time 2 emulator and installs the app.

### Emulator Controls

| Button | Key |
|--------|-----|
| Back | Q or Left Arrow |
| Up | W or Up Arrow |
| Select | S or Right Arrow |
| Down | X or Down Arrow |

### Test Dictation (Emulator)

The emulator has no microphone. Simulate dictation with:

```bash
pebble transcribe "Buy milk and eggs"
```

Run this BEFORE pressing SELECT in the app (or immediately after — the transcribe command sets up a fake transcription server).

**Expected flow:**
1. Launch app in emulator → see "Press SELECT to dictate"
2. In another terminal: `pebble transcribe "test note"`
3. Press SELECT (S key) in emulator
4. App shows "test note" and status changes to "Sending..."
5. If no webhook configured: status shows "HTTP error: 0" (expected)

### View Logs

```bash
pebble logs --emulator emery
```

Shows both C app logs (`APP_LOG`) and PebbleKit JS console output.

## Step 7: Phone Setup (for Real Watch)

### Install the Official Pebble App

Download from [rePebble.com/app](https://repebble.com/app) — this is the new
official Core Devices app (not the old Fitbit-era APK).

**Android 14+** (if direct install is blocked):
1. Download the APK from rePebble.com/app
2. Install via ADB: `adb install --bypass-low-target-sdk-block <pebble-app>.apk`
3. Go to Settings > Apps > Pebble > (3 dots) > Restricted Settings > Allow all

### Pair Your Watch

1. Open the Pebble app
2. Follow the pairing flow (Bluetooth LE)
3. Allow all permissions when prompted

### Enable Local Speech Recognition

This is the key feature — the official Pebble app runs the **parakeet-tdt-0.6b-v3** model
locally on your phone for offline speech-to-text.

1. Pebble app → Settings → Speech Recognition
2. Set to **"Local Only"** or **"Local (with Cloud Fallback)"**
3. Download the offline model when prompted (~715MB)
4. Set spoken language (or "Automatic")

**Important:** PebbleOS must be ≥ 4.9.158 for dictation to work reliably.
Earlier firmware has a "Dictation is not available" bug.

### Enable Developer Connection

1. Pebble app → Devices → tap 3 dots on your watch
2. Enable "Dev Connect"
3. Sign into GitHub when prompted
4. Note the IP address shown (for `--phone` installs)

Or use the newer cloudpebble flow:
```bash
pebble login    # signs in via GitHub
pebble install --cloudpebble
```

## Step 8: Deploy to Watch

```bash
pebble install --phone 192.168.1.42
```

Replace with your phone's IP from Step 7.

**Alternative (new rePebble app):**

If using the new Core Devices Pebble app:
1. Devices → tap 3 dots → Enable Dev Connect → Sign into GitHub
2. Then: `pebble login && pebble install --cloudpebble`

## Step 9: Configure the Webhook

1. On your phone, open the Pebble app
2. Go to the Voice to Note app settings (gear icon)
3. The configuration page opens in a webview
4. Enter:
   - **Webhook URL**: your n8n webhook endpoint (e.g., `https://n8n.example.com/webhook/voice-note`)
   - **HTTP Method**: POST
   - **Headers**: `{"Content-Type": "application/json"}`
   - **Body**: `{"note": "{{note}}"}` (default)
5. Tap "Save Settings"

## Step 10: Test End-to-End

1. On the watch: press SELECT
2. Speak your note
3. Watch shows transcribed text → "Sending..." → "Sent!"
4. Check your n8n workflow received the webhook

## Troubleshooting

### Build fails with "No SDK installed"

```bash
pebble sdk install latest
```

### Emulator won't start

Check SDL2 is installed: `dnf list installed SDL2` (Fedora) or `dpkg -l libsdl2*` (Ubuntu).

### "Connecting..." never changes to "Ready"

**In the emulator:** This is a known bug with the `emery` platform emulator. The phone
simulator (pypkjs) shows `QemuInboundPacket.footer` warnings and fails to deliver
AppMessages to the watch. The JS layer runs fine (confirmed in logs), but messages
don't reach the C app. **This only affects the emulator** — on a real watch with a real
phone, AppMessage works correctly.

**On a real watch:** PebbleKit JS isn't loaded. Check:
- `src/pkjs/index.js` exists and has no syntax errors
- Run `pebble logs --phone <IP>` to see JS errors

### Dictation fails on real watch

- Ensure PebbleOS ≥ 4.9.158 (earlier versions have a dictation bug)
- Ensure local model is downloaded (Pebble app → Speech Recognition → download offline model)
- Phone must be connected to watch via Bluetooth
- Check Speech Recognition is set to "Local Only" or "Local (with Cloud Fallback)"
- iOS: dictation support may not work yet on the new app (Android confirmed working)

### HTTP request fails

- Check webhook URL is correct and reachable from your phone
- Verify headers are valid JSON
- Check `pebble logs` for the actual HTTP status code

### Config page doesn't open

- GitHub Pages must be enabled on the repo
- URL in `src/pkjs/index.js` must match your actual GitHub Pages URL
- For local testing: `pebble emu-app-config --file config/index.html`
