# Hardware — SparkFun LoRa Gateway 1-Channel (ESP32)

Identified 2026-08-21 from the chip itself, not from the label. It was first
mistaken for the SparkFun Pro RF LoRa SAMD21; that board carries a Microchip
Cortex-M0+ and `esptool` would not have talked to it at all.

## Identification

Read from the chip, not assumed:

| | |
|---|---|
| MCU | ESP32-D0WDQ6 rev v1.0, dual core, 240 MHz |
| Flash | **4 MB**, GigaDevice |
| MAC | `84:0d:8e:0c:5a:80` |
| USB bridge | CH340C (`1a86:7523`, driver `ch341`) |
| Module | **ESP-WROOM-32NARROW** — *not* a WROOM-32E |
| Radio | Hope **RFM95HW** per the schematic, **RFM95W** per SparkFun's page — an **SX1276** either way |

Every one of those matches the SparkFun schematic, which names `CH340C` and
`ESP-WROOM-32NARROW`.

**The radio's exact part is unresolved, and the two sources disagree.** The
schematic — Jim Lindblom's drawing, dated 14 Nov 2018 — labels it **`RFM95HW`**.
SparkFun's product page calls it **`RFM95W`**. The `H` is the +20 dBm high-power
build. Both are SX1276 silicon, so the pin map, the register set and any
firmware port are unaffected; transmit power limits and duty-cycle arithmetic
are not. The schematic is the build document and is the better evidence, but
nobody has read the module, so treat this as open. This project originally
recorded plain `RFM95W`, which is the product page's wording.

The board is the **original 2019 revision, WRL-15006**: SparkFun's
later revision (WRL-18074) is otherwise identical but carries **16 MB** of
flash, and this one reports 4 MB.

Both revisions are **retired** from SparkFun's catalogue.

## References

| | |
|---|---|
| Product page | <https://www.sparkfun.com/sparkfun-lora-gateway-1-channel-esp32.html> |
| Retired listing | <https://www.sparkfun.com/products/15006> — **misleading: this URL serves the WRL-18074 page**, not the WRL-15006 one |
| Hookup guide | <https://learn.sparkfun.com/tutorials/sparkfun-lora-gateway-1-channel-hookup-guide> |
| Hardware repo | <https://github.com/sparkfun/ESP32_LoRa_1Ch_Gateway> |
| Eagle files | <https://cdn.sparkfun.com/assets/2/7/0/0/e/ESP32_LoRa_1_Channel_Gateway.zip> |
| Gateway firmware | <https://github.com/things4u/ESP-1ch-Gateway> |
| LMIC library | <https://github.com/mcci-catena/arduino-lmic> |

## Datasheets, and which ones are wrong

Audited 2026-08-21. **Every SparkFun documentation link now resolves to the
WRL-18074 page** — including `products/15006`, which is supposed to be the
retired original. That page states 16 MB of flash and links the ESP32-WROOM-32E
datasheet. Neither describes this board. Anyone following SparkFun's links in
good faith ends up with documentation for the later revision.

In `datasheets/` (gitignored — fetch them yourself):

| File | Correct for this board? |
|---|---|
| `sparkfun-lora-gateway-1ch-esp32-schematic.pdf` | **Yes.** Dated 14 Nov 2018, before WRL-18074 existed. Byte-identical to the PDF SparkFun still serves, so there is only ever one schematic and it cannot distinguish the revisions. |
| `semtech-sx1276-77-78-79-datasheet.pdf` | **Yes** — rev V7, 132 pp. The register-level reference a firmware port needs. |
| `hoperf-rfm95w-datasheet.pdf` | **Yes**, with a caveat: it is the family document `RFM95/96/97/98(W)` and does not distinguish `RFM95W` from `RFM95HW`. |
| `espressif-esp32-wroom-32-datasheet.pdf` | **Yes** — v3.7. This is the module the schematic names. |
| `espressif-esp32-datasheet.pdf` | **Yes** — the ESP32 chip datasheet, correct for the D0WDQ6. |
| `CH340DS1.pdf` | **Yes** — covers the CH340C. |
| `CH432DS1.pdf` | **No.** The CH432 is a dual UART and is **not on this board**; the schematic names only `CH340C`. Harmless, but it is not this board's documentation. |

**Do not fetch the `esp32-wroom-32e` datasheet SparkFun links.** The WROOM-32E
is built on the ESP32-D0WD-V3; this board reports D0WDQ6 rev v1.0, which is the
original WROOM-32. Wrong module, wrong silicon revision.

**The CH340 datasheet download needs a browser** — that WCH URL is a JavaScript
shell; see the note in `README.md`.

## Radio pin map

Corrected against the schematic on 2026-08-21; the SPI half had been recorded as
the ESP32's default VSPI bus, which is wrong.

| Signal | GPIO | Source |
|---|---|---|
| NSS | 16 | schematic (`IO16` → net `RFM_CS`), **and the chip answered** |
| SCK | **14** | schematic (`SCK_H/IO14` → net `SCK`) |
| MISO | **12** | schematic (`MISO_H/IO12` → net `MISO`) |
| MOSI | **13** | schematic (`MOSI_H/IO13` → net `MOSI`) |
| DIO0 | 26 | schematic (`IO26` → net `RFM_INT`) |
| DIO1 | 33 | schematic (net `RFM_DIO1/33`) |
| DIO2 | 32 | schematic (net `RFM_DIO2/32`) |
| RST | **none found** | SparkFun publishes 5 (LMIC sketch) and 27 (gateway tutorial, commented "Reset not used"). `src/tools/probe` pulsed both against the live chip and **neither resets the radio**. Treat both as placeholders. |

`SCK_V/IO18`, `MISO_V/IO19` and `MOSI_V/IO23` carry nets named `IO18`, `IO19`,
`IO23`: they are broken out to the headers and are **not** wired to the radio.

Corroborating sources, neither of which is the SparkFun sketch:
`Firmware/arduino-esp32/variants/sparkx_esp32_lora/pins_arduino.h` in SparkFun's
hardware repo (`SS 16`, `MOSI 13`, `MISO 12`, `SCK 14`), and `src/loraModem.h`
lines 298-301 in the gateway firmware (`MISO 12 / MOSI 13 / CLK 14 / SS 16`).

## What it is currently running

The stock **ESP-1ch-Gateway** sketch, i.e. what the hookup guide installs. Its
boot banner prints the stored configuration:

```
ESP32 defined, freq=903900000
.SF= 7   .CH= 0   .BOOTS= 26   .NTPS= 37
```

903.9 MHz is **US915 uplink channel 8** — `902.3 + 0.2 × 8` — and the first
channel of sub-band 2, the one US networks use. (Earlier notes here called it
channel 0; channel 0 is 902.3 MHz.) The stored NTP timestamp is from March
2019, so it has not been reconfigured since.

**The banner prints the stored WiFi SSID and password in the clear on every
boot.** Reading it put a third party's credentials into a session log before
anything masked them; `scripts/export-transcript.sh` now has patterns for the
`.SSID=` / `.PASS=` shape. Be aware before pasting a boot capture anywhere.

## How this differs from the Heltec V3

- **It is SX127x, so it is not interchangeable with FTG1.** Different firmware
  targets throughout: `heltec-v2.1`/`tlora-v2` rather than `heltec-v3` for
  Meshtastic, and RNode menu option 7 rather than 8. The SX1262-specific facts
  in `CLAUDE.md` — 1.8 V TCXO, DIO2 as RF switch, DIO1 on GPIO14 — do **not**
  apply to this board.
- **On this chip Meshtastic's phone queue is 8 packets, not 32.** The 32-packet
  branch is guarded to ESP32-S3 and C3.
- **It is directly relevant to Heltec #14**, the LoRaWAN-gateway-to-Meshtastic
  bridge, because it already is a working LoRaWAN gateway. That issue lives in
  the sibling project's tracker — **this repository has no tracker and no
  remote**, so a bare `#14` here points at nothing.
- **But TTN v3 does not accept single-channel gateways.** SparkFun's own listing
  says so. It can still speak LoRaWAN to a private network server; it cannot
  join The Things Network as a gateway. That constrains Heltec #14.
