# ESP32-LoRa-1Ch-Gateway

A **SparkFun LoRa Gateway - 1-Channel (ESP32)**, WRL-15006 — the original 2019
revision, retired from SparkFun's catalogue.

This is the second radio in a wider effort to link LoRa nodes across long
distances; the first is a Heltec WiFi LoRa 32 V3 in the sibling project
`../../Heltec/ESP32-LoRa-V3`. The two boards are **not interchangeable** — see
[`docs/hardware.md`](docs/hardware.md).

## What it is

| | |
|---|---|
| MCU | ESP32-D0WDQ6 rev v1.0, dual core, 240 MHz |
| Flash | 4 MB |
| Radio | Hope RFM95HW per schematic / RFM95W per SparkFun (Semtech **SX1276**) |
| USB bridge | CH340C (`1a86:7523`) |
| MAC | `84:0d:8e:0c:5a:80` |
| Antenna | U.FL |
| Extras | Qwiic connector, no display |

## Current state

**Reflashed.** The board no longer runs the 2019 firmware. As of 2026-08-23 it
runs `src/apps/listen` — receive-only, **915.0 MHz / SF7 / BW 125 kHz / sync
0x12** — and has a working link with the SAMD21 Pro RF in the sibling project.

The stock ESP-1ch-Gateway sketch is gone from `app0`. It is fully recoverable:
the bootloader, partition table and original `app0` were read off the chip
first, and `./scripts/restore-firmware.sh` writes all three back in about five
minutes.

**SPIFFS was never touched**, so the 2019 gateway config still sits there —
`freq=903900000`, `SF=7`, and the WiFi credentials described below. Restoring
the firmware therefore brings back a *configured* gateway, not a blank one.

**The stock firmware printed the stored WiFi SSID and password in the clear on
every boot.** That applies to a restored gateway, not to the current firmware.
Mask any boot capture from it; `./scripts/read-banner.sh` does so by default.

## Python environment

Tooling comes from a pyenv virtualenv named **`LoRa`**, pinned to this directory
by `.python-version`. To reproduce it:

```bash
pyenv virtualenv 3.12.12 LoRa          # skip if it already exists
pyenv local LoRa                       # writes .python-version
pip install --upgrade pip
pip install esptool pyserial
```

That is everything needed to identify the board and read its serial output.
Verify with `esptool version` (expect 5.3.1) and `esptool --port /dev/ttyUSB1
--no-stub chip-id`.

**pyenv is not a requirement** — a plain `python3 -m venv .venv` works equally
well; nothing in this project resolves interpreters by path.

**PlatformIO is deliberately outside any virtualenv.** The sibling project
installs it to `~/.platformio` and symlinks `pio` into `~/.local/bin`, shared
across every embedded project on this machine. If firmware gets built here, use
that rather than a second copy.

## Talking to it

```bash
esptool --port /dev/ttyUSB1 --no-stub chip-id     # identify
./scripts/read-banner.sh                          # reset and read the banner, masked
```

`read-banner.sh` finds the CH340 itself, resets the board over RTS (no RST tap),
reads the banner and **masks the WiFi credentials before printing them**. Use it
rather than a raw terminal: `python -m serial.tools.miniterm /dev/ttyUSB1 115200`
works but prints the stored SSID and password in the clear.

Pass `--no-mask` when you need to read the stored configuration exactly as it
is — that prints the WiFi password in the clear, so do not paste the result
anywhere.

Pass `--no-reset` to skip the mojibake. Resetting makes the ROM bootloader talk
first at 74880 baud, which is unreadable at 115200 — and unreadable at 74880
too, because the `ch341` driver rejects that rate. Not resetting avoids the ROM
phase entirely and gives clean output; you usually still get a banner, because
this board reboots itself every ~20 s when its stored WiFi network fails to
appear.

To identify the firmware from the flash rather than from the banner:

```bash
./scripts/dump-flash.sh --region list     # the real partition table
./scripts/dump-flash.sh --region app      # ~6 min at 115200, --no-stub
./scripts/identify-firmware.sh            # match it against the mirrored sources
```

**Only one of these can hold the port at a time.** They take an advisory lock
and refuse rather than corrupting a transfer in flight; opening the port twice
is what aborts a long read.

**`--no-stub` matters.** The stub flasher fails over this board's CH340 at both
921600 and 460800 baud. Bulk reads work without it but are slow — budget 15–20
minutes for a full 4 MB dump.

## Building

One PlatformIO project at the root. Fetch the libraries first — they are not
committed:

```bash
./scripts/fetch-libraries.sh
pio run                       # all three environments
pio run -e probe -t upload    # is the radio where we think it is?
pio run -e rx -t upload       # listen, report RSSI/SNR/loss
pio run -e scan -t upload     # sweep LoRaWAN/Meshtastic/link-test settings
pio run -e tx -t upload       # beacon  -- NEEDS AN ANTENNA ATTACHED
```

`esp32dev` is used only for the MCU, flash size and upload protocol. **Every
radio pin comes from `src/board.h`.** Do not swap in a stock ESP32 + SX127x
board definition: they assume VSPI with NSS 18, and the mismatch fails silently.

`rx` is safe with no antenna. **`tx` is not** — transmitting into an open U.FL
connector can damage the PA on an RFM95HW.

To put the original 2019 gateway firmware back: `./scripts/restore-firmware.sh`.

## What it can and cannot run

| | |
|---|---|
| ESP-1ch-Gateway (current) | works |
| LoRaWAN to a private server (ChirpStack) | plausible; already a gateway |
| The Things Network v3 | **no** — TTN v3 rejects single-channel gateways |
| Meshtastic | needs a **custom variant**; no stock target matches the pin map |
| Reticulum / RNode | needs custom firmware **and** homebrew provisioning |

The blocker for both mesh stacks is the same: this board wires the radio at
NSS 16 / RST 5 / DIO0 26 **and puts SPI on SCK 14 / MISO 12 / MOSI 13**, where
every stock ESP32 + SX127x target expects NSS 18 on the default VSPI bus.
Meshtastic is the easier of the two to port, because SX1276 is well supported
there and the work is pin definitions rather than driver code.

## Layout

```
Makefile                  CLI front door: build, flash, monitor, test
platformio.ini            one [env:...] per application
lib/                      our shared libraries (committed)
  board/                    the pin map — one definition
  sx1276/                   radio driver
  loramath/                 pure arithmetic, host-testable
  netconfig/                WiFi credentials in NVS, set over serial
src/apps/gateway/         WiFi + HTTP + LoRa receive
src/apps/link/            two-radio range test (rx / tx roles)
src/tools/probe/          does the radio answer, and on which SPI bus?
src/tools/scan/           sweep known LoRa settings, report what is heard
test/                     host-side unit tests (`make test`, no board needed)
vendor/                   third-party libraries — gitignored, see scripts/
docs/project-layout.md    why it is arranged this way, and CLion notes
docs/handoff.md           start here if you are new to this project
docs/historical-commits.md  commit messages from before the repo was re-initialised
docs/hardware.md          identification, pin map, how it differs from the Heltec
docs/programming.md       how it was configured, and how to build and upload
docs/alternative-languages.md  uLisp, Rust, MicroPython: what runs on Xtensa, what does not
docs/antenna.md           the Pycom kit's specs, and smaller options
docs/band-plan.md         902-928 MHz, why we sit on 915.0, what must match
docs/chirpstack.md        self-hosted LoRaWAN server: what it takes, where it stops fitting
docs/radio-settings-request.md  forwardable checklist for another radio's owner
docs/measuring-snr.md     how to measure SNR, and why a bench saturates it
docs/attenuation-report.md  measurements sent to the Pro RF side
docs/assets.md            every reference document: source URL, hash, purpose
docs/SHA256SUMS           `sha256sum -c` manifest for those documents
docs/datasheets/          component datasheets — COMMITTED, see its README for why
etc/firmware/             vendor firmware, Eagle files — gitignored
etc/reference/            SparkFun's LoRaWAN-device sketch, kept for reference
scripts/                  host-side tooling — see `make help`
```

## Reference documents

**[`docs/assets.md`](docs/assets.md) is the full list** — every datasheet,
schematic and source archive this project uses, with the exact URL it came from,
a SHA-256, and a note on what each one is good for. Verify a download with
`sha256sum -c docs/SHA256SUMS`.

The documents themselves are **not committed**: they are third-party vendor
files and most are not licensed for redistribution. What is committed is their
provenance.

This board is 2019 hardware, retired from SparkFun's catalogue, and vendor
download pages for parts that old get reorganised and eventually deleted. The
manifest is offered as a courtesy to anyone else working on the same dated
hardware — if a link has died by the time you read this, the hash is what lets
you confirm that a copy found elsewhere is the right file rather than merely a
plausibly-named one.

**Note that SparkFun's own links now describe the wrong board.** Every URL for
this product, `products/15006` included, serves the WRL-18074 page: 16 MB of
flash and a WROOM-32E datasheet. This board is the 4 MB WRL-15006. The schematic
is the exception and is trustworthy. See `docs/assets.md`.

## Source links

- Product — <https://www.sparkfun.com/sparkfun-lora-gateway-1-channel-esp32.html>
- Retired listing — <https://www.sparkfun.com/products/15006> — **serves the WRL-18074 page**, not this board's
- Hookup guide — <https://learn.sparkfun.com/tutorials/sparkfun-lora-gateway-1-channel-hookup-guide>
- Hardware repo — <https://github.com/sparkfun/ESP32_LoRa_1Ch_Gateway>
- Eagle files — <https://cdn.sparkfun.com/assets/2/7/0/0/e/ESP32_LoRa_1_Channel_Gateway.zip>
- Gateway firmware — <https://github.com/things4u/ESP-1ch-Gateway-v5.0--OLD>
  (**this** is what the board runs. SparkFun links it as `…-v5.0`, which now
  redirects: the repository was renamed to add `--OLD`. `things4u/ESP-1ch-Gateway`
  without the `-v5.0` is a different, later repository.)
- LMIC library — <https://github.com/mcci-catena/arduino-lmic>
- SX1276 datasheet — <https://cdn.sparkfun.com/assets/7/7/3/2/2/SX1276_Datasheet.pdf>
- ESP32-WROOM-32 datasheet — <https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf>
  (**not** the `esp32-wroom-32e` one SparkFun links — wrong module for this board)
- CH340 datasheet — <https://www.wch-ic.com/downloads/CH340DS1_PDF.html> (not
  mirrored: that URL is a JavaScript shell, ~4 KB with no links in the HTML,
  which builds the download list client-side. `curl` gets the shell, not the
  PDF. Fetch it with a browser.)

## License

[MIT](LICENSE) — Copyright (c) 2026 Jeffrey Peacock
