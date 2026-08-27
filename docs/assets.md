# Reference assets — where they came from, and how to check what you got

This board is **2019 hardware, retired from SparkFun's catalogue**. Vendor
download pages for parts that old are reorganised, moved behind JavaScript, and
eventually deleted. By the time someone else needs these files, some of the
links below may be dead.

So this file is offered as a courtesy to anyone working on the same dated
hardware: a checked list of every document and source archive this project uses,
the exact URL each came from, and a SHA-256 for each, so that a file recovered
from a mirror, an archive site, or a colleague can be verified as the same one
rather than merely something with a plausible filename.

**What is committed, and what is not**, differs by kind of document. The
licensing is not uniform, and an earlier version of this file said flatly that
all of it "is not licensed for redistribution", which was true of some and wrong
about the rest.

| Kind | Committed? | Licence |
|---|---|---|
| **Component datasheets**, `docs/datasheets/` | **Yes** | All-rights-reserved, mirrored as fair use. See `docs/datasheets/README.md` |
| **Source archives**, `etc/firmware/` | No | Other people's code. `scripts/fetch-libraries.sh` retrieves them; the table below has every URL and commit |
| **Mirrored web pages**, `docs/*.html` | No | CC BY-SA 4.0 — permitted, but they carry Google Tag Manager and social beacons, and share-alike sits awkwardly in an MIT repository |
| **The one vendor sketch**, `etc/reference/` | Yes | Beerware, explicitly redistributable |

**The datasheets are the deliberate exception.** They are all-rights-reserved
and are mirrored anyway, because this is 2019 hardware whose vendor pages are
already decaying — the CH340 datasheet needed a browser to fetch, and every
SparkFun URL for this SKU now serves a different board. `docs/datasheets/README.md`
states the fair-use position in full and **offers removal on request to any
copyright holder**. Read it before adding to that directory.

For everything else, what is committed is this manifest — the provenance, not
the payload.

If a link below is dead, the SHA-256 is the useful part: search for the filename,
then verify what you find against the hash.

## Fetch and verify

```bash
sha256sum -c docs/SHA256SUMS          # from the project root, after fetching
```

`docs/SHA256SUMS` is the machine-readable half of this file, in the format
`sha256sum -c` expects. Paths in it are relative to the project root.

## Datasheets — `docs/datasheets/`

| File | Source | Notes |
|---|---|---|
| `sparkfun-lora-gateway-1ch-esp32-schematic.pdf` | <https://cdn.sparkfun.com/assets/5/3/1/9/c/ESP32_LoRa_1_Channel_Gateway.pdf> | **The authoritative document for this board.** Dated 14 Nov 2018. Released under CC BY-SA 4.0 per its own title block. SparkFun serves this same file from both the WRL-15006 and WRL-18074 pages, so it cannot distinguish the revisions. |
| `semtech-sx1276-77-78-79-datasheet.pdf` | <https://cdn.sparkfun.com/assets/7/7/3/2/2/SX1276_Datasheet.pdf> | Rev V7, 132 pp. The register-level reference for the radio. Adafruit mirrors the same document at <https://cdn-shop.adafruit.com/product-files/3179/sx1276_77_78_79.pdf>. |
| `hoperf-rfm95w-datasheet.pdf` | <https://cdn.sparkfun.com/assets/a/e/7/e/b/RFM95_96_97_98W.pdf> | The `RFM95/96/97/98(W)` family document. It does **not** distinguish `RFM95W` from `RFM95HW`. |
| `espressif-esp32-wroom-32-datasheet.pdf` | <https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf> | v3.7. The module this board actually carries — the schematic names `ESP-WROOM-32NARROW`. |
| `espressif-esp32-datasheet.pdf` | <https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf> | The ESP32 chip datasheet. Correct for the D0WDQ6 this board reports. |
| `CH340DS1.pdf` | <https://www.wch-ic.com/downloads/CH340DS1_PDF.html> | Covers the CH340C. **That URL is a JavaScript shell** — it builds its download list client-side, so `curl` retrieves a ~4 KB page with no links in it. Fetch it with a browser. |
| `espressif-esp32-wroom-32e-datasheet-WRONG-REVISION.pdf` | <https://cdn.sparkfun.com/assets/4/3/1/7/6/esp32-wroom-32e_esp32-wroom-32ue_datasheet_en_v1-6.pdf> | **Kept only to document a trap.** This is what SparkFun's product page links, and it is the wrong part: the WROOM-32E is built on the ESP32-D0WD-V3, where this board reports D0WDQ6. Do not use it for this board. |
| `CH432DS1.pdf` | WCH | **Not this board's.** The CH432 is a dual UART and appears nowhere in the schematic, which names only `CH340C`. Listed because the file is present, not because it belongs. |

## Source archives — `etc/firmware/`

GitHub archive zips are **not byte-stable**: GitHub regenerates them, and the
same branch can produce a different byte stream and therefore a different
SHA-256 later. The hashes below record what was downloaded on 2026-08-21. For
these, the **commit SHA is the durable identifier**, not the hash of the zip.

| File | Source | Commit at download | Licence |
|---|---|---|---|
| `things4u-ESP-1ch-Gateway-v5.0-master.zip` | <https://github.com/things4u/ESP-1ch-Gateway-v5.0--OLD> | `0e0717c51ad8` (2019-03-29) | MIT |
| `things4u-ESP-1ch-Gateway-master.zip` | <https://github.com/things4u/ESP-1ch-Gateway> | `3b023527bd23` (2021-10-18) | MIT |
| `sparkfun-ESP32_LoRa_1Ch_Gateway-main.zip` | <https://github.com/sparkfun/ESP32_LoRa_1Ch_Gateway> | `7877033edce1` (2023-02-10) | see repo |
| `mcci-catena-arduino-lmic-master.zip` | <https://github.com/mcci-catena/arduino-lmic> | `25d1670c7452` (2026-04-04) | MIT |
| `sparkfun-eagle-ESP32_LoRa_1_Channel_Gateway.zip` | <https://cdn.sparkfun.com/assets/2/7/0/0/e/ESP32_LoRa_1_Channel_Gateway.zip> | static CDN asset | CC BY-SA 4.0 |
| `sparkfunX-ESP32_LoRa_1CH_Gateway-master.zip` | <https://github.com/sparkfunX/ESP32_LoRa_1CH_Gateway> | pushed 2022-01-31 | see repo |
| `sparkfun-tutorial-sparkx_esp32_lora-variant-v01.zip` | <https://cdn.sparkfun.com/assets/learn_tutorials/8/0/4/sparkx_esp32_lora-v01.zip> | static CDN asset | see repo |
| `sparkfun-tutorial-ESP-1CH-TTN-Device-ABP-v01.zip` | <https://cdn.sparkfun.com/assets/learn_tutorials/8/0/4/ESP-1CH-TTN-Device-ABP-v01.zip> | static CDN asset | see repo |
| `sparkfun-tutorial-esp32_lora_gateway-x01.zip` | <https://cdn.sparkfun.com/assets/learn_tutorials/8/0/4/esp32_lora_gateway-x01.zip> | static CDN asset | CC BY-SA 4.0 |

The three above are linked only from the tutorial body, not the product page, so
they are easy to miss. The variant zip is the Arduino board definition the build
needs; the `esp32_lora_gateway-x01.zip` is Eagle `.brd`/`.sch`, **not** a sketch,
despite the name. The variant is byte-identical to the repo copy apart from line
endings.

### The two firmware repositories are not the same code

`things4u` publishes two repositories with near-identical names, and picking the
wrong one silently gives you source that does not match this board:

| Repo | Last pushed | Is it what the board runs? |
|---|---|---|
| `ESP-1ch-Gateway-v5.0--OLD` | Sept 2019 | **Yes.** SparkFun links it as `…-v5.0`, which redirects — the repository was renamed to add `--OLD`. |
| `ESP-1ch-Gateway` | Oct 2022 | No — a later rewrite. |

The tell is the boot line: v5.0 prints `. WiFi connect SSID=`, the 2022 tree
prints `. WlanConnect SSID=`. This board prints `WiFi connect`. Corroborating
that, v5.0's head commit is dated 2019-03-29 and the board's stored NTP
timestamp is 2019-03-25.

Both are mirrored here because the difference is the point. **Read the v5.0 one.**

### SparkX vs SparkFun: two hardware repositories too

The same trap as the firmware repos. There are two, differing by one letter and
one capital, and the **SparkX** one is the original board's:

| Repo | Notes |
|---|---|
| `sparkfunX/ESP32_LoRa_1CH_Gateway` | SparkX, note `1CH`. The original SPX-14893 / WRL-15006 board — the one here. Also carries `Production/` send and receive test sketches, useful for a link test. |
| `sparkfun/ESP32_LoRa_1Ch_Gateway` | note `1Ch`. The productised WRL-18074 line. |

Both ship the same `sparkx_esp32_lora` Arduino variant with `SS 16 / MOSI 13 /
MISO 12 / SCK 14`, which is a third independent confirmation of the pin map.

## Guides — `docs/`

| File | Source | Notes |
|---|---|---|
| `sparkfun-hookup-guide.html` | <https://learn.sparkfun.com/tutorials/sparkfun-lora-gateway-1-channel-hookup-guide/all> | The guide for **WRL-18074**, the later board. The `/all` suffix returns every page as one document. |
| `sparkfun-tutorial-1ch-gateway-ttn.html` | <https://learn.sparkfun.com/tutorials/esp32-lora-1-ch-gateway-lorawan-and-the-things-network/all> | **The tutorial for THIS board.** The hookup guide above points to it for the older SKU, SPX-14893. Covers the gateway sketch, turning the gateway into a device, and TTN routing. |

**Both pages are CC BY-SA 4.0**, per the "License" line in each page footer, so
quoting or re-hosting them is permitted with attribution and share-alike.

Images are not inlined in either, so they will not render offline. DigiKey also
mirrors the hookup guide as a PDF, which may outlive SparkFun's copy:
<https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/4066/LoRa_Gateway_1-Channel_Hookup_Guide_Web.pdf>

## A caution about SparkFun's own links

Audited 2026-08-21. **Every SparkFun URL for this product, including
`sparkfun.com/products/15006`, now serves the WRL-18074 page** — the later
16 MB revision. That page states 16 MB of flash and links the WROOM-32E
datasheet. Neither describes the original WRL-15006.

The schematic is the exception: it is the 2018 drawing and it does describe this
board. Where SparkFun's page and the schematic disagree — the page says
`RFM95W`, the schematic says `RFM95HW` — the schematic is the build document.
