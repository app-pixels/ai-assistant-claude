> Part of [**app-pixels.com**](https://www.app-pixels.com) — browse + flash this app at [`/apps/ai-assistant-claude`](https://www.app-pixels.com/apps/ai-assistant-claude).

# ai-assistant-claude

**AI Assistant (Claude)** · v1.1.0

Voice-to-text AI assistant — Anthropic Claude for the reply, Groq Whisper for STT. Claude can call device + web tools.

**Hardware:** Waveshare ESP32-S3 1.8" AMOLED Touch

**Tags:** `#ai` `#voice`

Voice-to-text assistant that uses **Anthropic Claude (Haiku 4.5)** for the reply and **Groq Whisper** for speech-to-text. Hold **BOOT** to speak; release to get an answer.

Sibling to [**AI Chat (free — groq)**](/apps/ai-chat) — same UI, same controls. The Claude variant uses a more capable model and can call **device + web tools** to give grounded answers (battery, time, weather, notes, web search) instead of guessing.

**Cost.** About **$0.001 per question** for plain Q&A. Tool round-trips roughly double the cost; **web search** is the expensive one — a single search call adds ~$0.01. Anthropic lets you cap monthly spend (see step 3 below).

## Tools the assistant can call

- **Device state** — get time, battery, uptime, WiFi signal, orientation
- **Device control** — set brightness, play beep, restart, power off
- **Weather** — current conditions for `LOCATION_1` (no API key needed)
- **Notes** — save / list notes on SD card (`/notes/YYYY-MM-DD.txt`)
- **Web search** — Anthropic-hosted; toggled with PWR on the splash

## `setup.txt` keys

**Mandatory**
- `SSID` / `PASSWORD` — WiFi
- `CLAUDE_KEY` — Anthropic API key (starts with `sk-ant-api…`) — chat reply
- `GROQ_KEY` — Groq API key (free at [groq.com](https://groq.com)) — speech-to-text only

**Optional**
- `LANGUAGE` — ISO-639-1 hint for Whisper (`de`, `en`, …). Omit for auto-detect.
- `TIMEZONE` — POSIX TZ string for the time tool (e.g. `CET-1CEST,M3.5.0,M10.5.0/3`). Defaults to UTC.
- `LOCATION_1` — city name for the `get_weather` tool (geocoded on first use).
- `CLAUDE_WEBSEARCH` — `0`/`off`/`no`/`false` disables web search by default. Anything else (incl. missing) leaves it on.

## How to get a Claude API key (2 minutes)

1. Go to [**console.anthropic.com**](https://console.anthropic.com).
2. Sign in (or sign up — any email works).
3. **Billing → Add payment method**, then set a **monthly spend cap of $5**. More than enough for casual home use with Haiku.
4. **Settings → API Keys → Create Key**. Name it something like `esp32-app-pixels` so you can rotate it later. Copy the key — it starts with `sk-ant-api…`.
5. Paste it into `/setup/setup.txt` on the SD card as:
   ```
   CLAUDE_KEY = sk-ant-api03-xxxxxxxx...
   ```

> **Note.** A Claude Pro / Max subscription does **not** give you API access — they're billed separately. The API account is its own thing at the console URL above.

## Controls

- **BOOT** (hold) — talk; release to send.
- **BOOT** (tap) — scroll long answers half a page; tap past the bottom returns to the top.
- **PWR** (short) — on the splash: toggle web search on/off. After the first reply: start a new conversation.

## Editing `setup.txt`

The device reads `/setup/setup.txt` from the SD card on boot. [Download a working sample](https://sosbxffigpteqilpgxwn.supabase.co/storage/v1/object/public/app-assets/setup/setup.txt) — covers every app — and edit the keys you need.

Don't want to eject the card? Use the [**USB Stick**](/apps/usb-stick) app (mounts the SD card as a USB drive over USB-C) or the [**Filehub**](/apps/filehub) app (edit over WiFi).

## Build

1. Install [arduino-cli](https://arduino.github.io/arduino-cli/) or Arduino IDE 2.x.
2. Add the ESP32 board package (≥ 3.1.0):

   ```
   arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
   arduino-cli core install esp32:esp32 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```

3. Install the required Arduino libraries:

   - ArduinoJson (bblanchon)
   - GFX Library for Arduino (moononournation)
   - SensorLib (lewishe)
   - XPowersLib (lewishe)

4. Compile and upload:

   ```
   FQBN='esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,PSRAM=opi,FlashSize=16M,FlashMode=qio,PartitionScheme=app3M_fat9M_16MB,UploadSpeed=921600,LoopCore=1,EventsCore=1'
   arduino-cli compile -b "$FQBN" --build-path /tmp/ai-assistant-claude_build .
   arduino-cli upload  -b "$FQBN" --input-dir /tmp/ai-assistant-claude_build -p /dev/ttyACM0 .
   ```

   For browser flashing without a build environment, use the [pre-built binary](https://www.app-pixels.com/apps/ai-assistant-claude).

## License

MIT — see [LICENSE](LICENSE). Do whatever you want with it.

---

Part of the [app-pixels.com](https://www.app-pixels.com) catalogue · live listing: https://www.app-pixels.com/apps/ai-assistant-claude
