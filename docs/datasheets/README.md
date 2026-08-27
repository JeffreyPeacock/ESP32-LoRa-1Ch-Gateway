# Component datasheets — why they are committed here

These PDFs are **unmodified vendor documents, redistributed without permission**,
and they are committed deliberately. This file explains the reasoning, states the
position plainly, and offers removal on request.

## Why

This board is a **SparkFun LoRa Gateway 1-Channel (ESP32), WRL-15006** — 2019
hardware, retired from SparkFun's catalogue. Its principal components are end of
life or close to it:

| Part | Status |
|---|---|
| Semtech SX1276 | superseded by the SX126x family |
| Espressif ESP32-D0WDQ6 | superseded by the -V3 die, then by the S- and C-series |
| Hope RFM95W | long-running, but the module page has moved repeatedly |
| WCH CH340C | current, but WCH's download page is a JavaScript shell that `curl` cannot read |

Vendor download pages for parts this old get reorganised, moved behind
JavaScript, and eventually deleted. Two of these were already awkward to obtain
in August 2026: the CH340 datasheet needed a browser because the WCH page builds
its link list client-side, and every SparkFun URL for this SKU now serves a
**different board's** page with a different module's datasheet attached.

Anyone picking up this hardware in 2028 will need exactly these eight files, and
a dead link plus a SHA-256 is not the same as the document.

## The position, stated plainly

The copyright in each document belongs to its vendor — Semtech, Espressif,
Hope Microelectronics, WCH, SparkFun. **No licence is claimed or implied here,
and none is granted to you by this repository.**

The expectation is that mirroring complete, unmodified datasheets for
discontinued components, in a non-commercial repository, for the sole purpose of
keeping obsolete hardware serviceable, is **fair use**. The relevant factors:

- **Purpose** — non-commercial, educational, and preservational. Nothing here
  competes with the vendor.
- **Nature** — technical reference material, factual rather than creative.
- **Amount** — the whole document. This is the factor that cuts against, and
  there is no honest way around it: a partial datasheet is useless.
- **Market effect** — none. These are given away free by their vendors to
  promote the parts, and the parts are discontinued.

**Fair use is a judgment, not a permission.** It is asserted here in good faith
and has not been tested.

## Removal on request

**If you hold the copyright in any document here and would rather it were not
mirrored, open an issue or contact the repository owner and it will be removed
promptly, without argument.** Every entry in `docs/assets.md` carries the
original source URL, so removal costs readers a link rather than the
information.

## What is *not* committed, and why

- **Source archives** — `etc/firmware/` holds vendor repository snapshots and
  tutorial zips. Those are other people's *code*, they remain available from
  their original repositories, and `scripts/fetch-libraries.sh` retrieves them
  on demand. They are gitignored and should stay that way.
- **Mirrored web pages** — the two saved SparkFun tutorial pages are CC BY-SA
  4.0 and could be committed, but they carry Google Tag Manager and
  social-network beacons, and share-alike content sits awkwardly in an MIT
  repository.

## The files

`docs/assets.md` is the authoritative table: source URL, purpose, and which
documents describe **this** board rather than the later revision. `docs/SHA256SUMS`
verifies them:

```bash
sha256sum -c docs/SHA256SUMS
```

Two files here do **not** describe this board and are kept as evidence that the
vendor documentation points at the wrong parts:

| File | Why it is here |
|---|---|
| `espressif-esp32-wroom-32e-datasheet-WRONG-REVISION.pdf` | What SparkFun's current product page links. The -32E is built on the D0WD-**V3** die and reports silicon revision 3; this board reports **D0WDQ6 revision v1.0** |
| `CH432DS1.pdf` | The CH432 is a dual UART and appears nowhere in the schematic, which names only `CH340C`. Retrieved alongside the CH340 datasheet from the same WCH page |

The schematic, `sparkfun-lora-gateway-1ch-esp32-schematic.pdf`, is the one
document that is dated (14 Nov 2018) and unambiguously describes this board. It
is what settled the SPI pin map. Trust it over any vendor product page.
