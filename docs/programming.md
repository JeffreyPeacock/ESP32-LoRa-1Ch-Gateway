# Programming this board

How the gateway was configured in 2019, and how to do it again. Reconstructed
2026-08-21 from the firmware source, the flash image, and SparkFun's tutorial
for **this** SKU — not the hookup guide, which is for the later board.

## Short answer: yes, just upload a new sketch

Nothing on the board prevents it. From `espefuse summary`:

| Fuse | Value | Meaning |
|---|---|---|
| `FLASH_CRYPT_CNT` | 0 | no flash encryption |
| `ABS_DONE_0` / secure boot | not set | no secure boot |
| `WR_DIS` | `0x0000` | nothing write-protected |
| `RD_DIS` | `0x0` | nothing read-protected |

It is an ordinary ESP32 with a working auto-reset circuit. Upload over USB with
the Arduino IDE, PlatformIO, or `esptool write-flash`.

## How WiFi was configured originally

**By recompiling.** There is no runtime configuration path on this build:

- The sketch **never reads serial** — no `Serial.available()` or `Serial.read()`
  anywhere in the tree. Serial is output-only.
- `WIFIMANAGER 0`, so there is no access-point config portal. Confirmed against
  the image: `WiFiManager`, `AutoConnect`, `captive` and `configPortal` all
  absent.
- The web UI can change settings, but reaching it requires WiFi already working.

Credentials live in a compile-time table, `ESP-sc-gway.h:261`:

```c
struct wpas { char login[32]; char passw[64]; };
wpas wpa[] = {
    { "", "" },                              // reserved, leave blank
    { "my_wifi_network", "my_wifi_password" }
};
```

**SPIFFS is a cache of that, not a second source of truth.** After a successful
connect, `_WiFi.ino:298` calls `writeGwayCfg()`, which does
(`_loraFiles.ino:221`):

```c
gwayConfig.ssid = WiFi.SSID();
gwayConfig.pass = WiFi.psk();
```

So it records whatever the WiFi stack actually used and writes it to the SPIFFS
config file. That is why both stores hold identical values here, and why
**erasing SPIFFS does not stop the board connecting** — the compiled table
survives and gets used again on the next boot.

### How many slots, and which one is in use

`WlanConnect` walks the whole array (`_WiFi.ino:261`). The board tells you the
state of that walk on every attempt — the digits before `. WiFi connect` are
not a timestamp:

```
0:1:2. WiFi connect SSID=...
│ │ └── sizeof(wpa)/sizeof(wpa[0])  — total slots
│ └──── j — slot being tried
└────── i — retry counter
```

So this build has **two slots** and is connecting on **slot 1**; slot 0 is the
blank reserved entry. Add more networks by adding entries and re-uploading.

## The exact source revision on the board

The image reports `VERSION "V.5.3.3.H; 180825a"`. In the cloned repo that string
appears in exactly one commit:

| | |
|---|---|
| Repo | `things4u/ESP-1ch-Gateway-v5.0--OLD` (SparkFun links `…-v5.0`, which redirects) |
| Commit | **`88113e8`**, 2018-08-26, "Version 5.3.3" |
| Distance from HEAD | 9 commits behind |

Arduino core in the image: ESP-IDF `v3.3-beta1-179-ge931fe9f5-dirty`.

To rebuild what is on the board rather than something newer:

```bash
git checkout 88113e8
```

## The build recipe

From SparkFun's tutorial for this SKU. All of it is mirrored — see
`docs/assets.md`.

1. **Install the ESP32 Arduino core** manually from
   `github.com/espressif/arduino-esp32`.
2. **Add the SparkX variant.** Unzip `sparkx_esp32_lora/` into
   `{sketchbook}/hardware/espressif/esp32/variants/`, then append the board
   stanza the tutorial gives to `boards.txt`. This is what supplies
   `SS 16 / MOSI 13 / MISO 12 / SCK 14` — the sketch never names the SPI pins
   itself, which is exactly why a stock board definition produces a radio that
   initialises and hears nothing.
3. **Edit `ESP-sc-gway.h`:** `_LFREQ 915`, `_SPREADING SF7`, `OLED 0` (no
   display on this board), `_PIN_OUT 6`, ensure `CFG_sx1276_radio` and not
   `CFG_sx1272_radio`, set `_TTNSERVER`, `_DESCRIPTION`, `_LAT`/`_LON`, and add
   your networks to `wpa[]`.
4. **Edit `loraModem.h`.** Upstream has `_PIN_OUT` 1–5 only; there is no 6.
   Delete the `#error "Pin Definitions _PIN_OUT must be 1(HALLARD) or
   2 (COMRESULT)"` line and put this in its place, between the `#else` and the
   `#endif`:

   ```c
   struct pins {
       uint8_t dio0 = 26;
       uint8_t dio1 = 33;
       uint8_t dio2 = 32;
       uint8_t ss   = 16;
       uint8_t rst  = 27;   // Reset not used
   } pins;
   #define SCK  14
   #define MISO 12
   #define MOSI 13
   #define SS   16
   #define DIO0 26
   ```

5. **Upload**, then watch serial at 115200 for the IP address. First boot is
   slow: it formats SPIFFS. Then browse to that IP for the config and log UI.

## SparkFun contradicts itself about RST

Three sources, three answers, and it is unresolved:

| Source | RST |
|---|---|
| `ESP-1CH-TTN-Device-ABP.ino:66` (LMIC device sketch) | `.rst = 5` |
| Gateway tutorial's `_PIN_OUT 6` block | `rst = 27`, commented **"Reset not used"** |
| Schematic | net `RFM_RST` exists; its ESP32 end did not survive text extraction |

**Settled 2026-08-21: neither pin resets the radio.** `src/tools/probe`
wrote a recognisable value to `RegOpMode`, pulsed each candidate low, and read
it back. A real reset restores the register default; neither GPIO 5 nor GPIO 27
changed it. The tutorial's own throwaway comment, "Reset not used", is the only
accurate published statement about this pin. Copy the block as given — it works,
because `rst` is never exercised — but do not believe the number. If a port
needs a hardware reset, read it off the PCB.

Everything else agrees across four independent sources: NSS 16, SCK 14,
MISO 12, MOSI 13, DIO0 26, DIO1 33, DIO2 32.

## Libraries

`scripts/fetch-libraries.sh` pulls them from GitHub into `lib/`, which is
gitignored — this repository is public and they are other people's code, so it
ships the script and the pinned refs rather than the payload.

```bash
./scripts/fetch-libraries.sh                     # ten shipped + ArduinoJson
./scripts/fetch-libraries.sh --with-wifimanager  # only if you set WIFIMANAGER=1
```

The gateway repo bundles ten libraries under `libraries/`, pinned here to the
same commit `88113e8` the board's firmware was built from. ArduinoJson must be
**5.13.1** — the 6.x API is incompatible — and the bundled copy already is, which
the script verifies by version rather than assuming by presence. WiFiManager is
skipped by default: its include is guarded at `ESP-sc-gway.ino:66`, and this
build has `WIFIMANAGER 0`.

Point the Arduino IDE's sketchbook at a folder whose `libraries/` is this one,
or for PlatformIO add `lib_extra_dirs = ../../lib`.

## Once it is on a network

The web UI is at the IP the sketch prints. It can change settings and do OTA
firmware updates (`Gateway Settings`, `Update Firmware`, `Reset` are all present
in the image), so after the first USB upload you need not use the cable again.

**A shortcut for this specific board:** rather than reflashing to change the
network, stand up an access point matching the credentials already compiled into
`wpa[]`. The board joins, prints its IP, and the web UI becomes reachable with
no cable and no rebuild.

Those values are **redacted from this repository**, which is public. Recover them
locally either way:

- `.claude_artifacts/mask-secrets.txt` — gitignored, holds both
- or read them off the board: `./scripts/read-banner.sh --no-mask` after
  restoring the stock firmware, or search the app image at flash `0x3f478`

## Debug level, and the password print

`ESP-sc-gway.ino:112` sets `uint8_t debug = 1` at compile time — that is the
`debug=1` in the boot banner. Then `_WiFi.ino:137` overwrites it with the stored
value: `debug = gwayConfig.debug`. This board's config has `.DEBUG=0`.

That matters because `_WiFi.ino:275` guards the password print at `debug >= 1`:

```c
Serial.print(F(". WiFi connect SSID="));
Serial.print(ssid);
if ( debug >= 1 ) { Serial.print(F(", pass=")); Serial.print(password); }
```

So the only reason the runtime connect line does not print the password is the
stored `DEBUG=0`. Raise debug — the web UI has +/- buttons for it
(`_wwwServer.ino:247`) — and it will.
