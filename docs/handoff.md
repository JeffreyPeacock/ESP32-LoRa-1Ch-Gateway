# Handoff — read this first

You are taking over a project with **one board and no issue tracker**. Much of
the value is written-down findings rather than code — but as of 2026-08-23 there
is now firmware of its own too: a PlatformIO project with an SX1276 driver, a
listener, a link test and two diagnostic tools. Read `CLAUDE.md` before touching
anything; every line in it cost real time to establish.

## The one-paragraph version

There is a **SparkFun LoRa Gateway 1-Channel (ESP32)**, part WRL-15006. **Do not
hardcode its port** — `/dev/ttyUSBn` moves; use `./scripts/find-port.sh`.

It now runs this project's own firmware and has a **working 915.0 MHz link** to
the SAMD21 Pro RF in `../SAMD21-LoRa-ProRF`. The original question was what else
it could run, and the answer was "nothing stock" — its radio pin map matches no
Meshtastic or RNode target, which is why the driver here was written from
scratch with every pin passed explicitly.

## What is verified, and what is inference

Take this distinction seriously. Do not promote the second list to the first
without checking.

**Verified by reading the hardware:**

- ESP32-D0WDQ6 rev v1.0, 4 MB flash, MAC `84:0d:8e:0c:5a:80` — from `esptool`
- CH340C bridge, `1a86:7523`, driver `ch341` — from `udevadm`
- It *was* running ESP-1ch-Gateway at 903.9 MHz, SF7, last configured 2019 —
  from the boot banner. **Reflashed since**; see "What was deliberately not
  done". The SPIFFS config still holds those values.
- `esptool`'s stub loader fails at 921600 and 460800; `--no-stub` works — tried
- No Meshtastic variant matches `spark`/`1ch`/`gateway`; no RNode board uses
  `pin_cs = 16` — grepped both upstream repos
- Pin map NSS 16 / DIO0 26 / DIO1 33 / DIO2 32, **SPI on SCK 14 / MISO 12 /
  MOSI 13** — **confirmed by the chip itself**: `src/tools/probe` read `RegVersion`
  and got `0x12` on this wiring and `0x00` on the VSPI pins this project used
  to record
- **RST is neither 5 nor 27.** SparkFun publishes both; the probe pulsed each
  against the live radio and neither reset it. The tutorial's own comment,
  "Reset not used", is the accurate one

**Inferred, not proven:**

- That the board is WRL-15006 rather than WRL-18074. This rests on it reporting
  4 MB where the later revision has 16 MB. Nobody has read the silkscreen.
- That the radio is an RFM95W. The schematic says `RFM95` and the firmware only
  supports SX1276, but nobody has looked at the module.
- That a full flash backup is unnecessary. Reasoning is that the firmware is
  stock and the only unique content is a stale WiFi config — **three backup
  attempts failed** before that conclusion was reached, so it is partly
  rationalisation. If you need the flash, budget about 12 minutes for 4 MB with
  `--no-stub` — measured at 43.4 kbit/s, not the 15–20 once guessed here.
  *(Partly resolved 2026-08-21: a dump aborted at 16% because a second script
  opened the port mid-read. That is a likelier cause of the three failures than
  a hardware fault, and the scripts now lock the port. See `CLAUDE.md`.)*

## Three traps

**1. The firmware prints WiFi credentials on every boot.** `.SSID=` and `.PASS=`
in the clear. This has already leaked a third party's credentials into a session
log once. Mask before pasting a banner anywhere; `scripts/export-transcript.sh`
has patterns for it.

And a third half, found by dumping the flash on 2026-08-21: **the firmware image
carries its own WiFi pair, separate from the SPIFFS config.** The compile-time
`wpa[]` table at flash `0x3f478` holds an SSID and the password `<redacted>` —
the same pair the SPIFFS config holds. They are a MakerSpace network the owner
used years ago — **not live credentials**, so do not spend time protecting them. What matters is structural:
`dump-flash.sh` claimed the app partition was credential-free because the config
lives in SPIFFS, and compile-time tables defeat that reasoning.

The trap has a second half that the first pass missed: **the config dump is not
the only place the SSID is printed.** The sketch prints it again at runtime as
`WiFi connect SSID=`, `WlanStatus:: CONNECTED ssid=` and three more shapes, all
with text before the key, so the line-anchored `.SSID=` pattern sailed past
them. Reading the banner on 2026-08-21 put the SSID in the clear into a session
log a second time. All six print sites are now listed in `CLAUDE.md` and covered
by the export script. A bare SSID with no `ssid=` in front of it still needs a
literal in `.claude_artifacts/mask-secrets.txt`.

**2. None of the Heltec project's hardware facts apply here.** That project is
an SX1262 behind a CP2102N; this is an SX1276 behind a CH340C. Its notes about
a 1.8 V TCXO, DIO2 as RF switch, and DIO1 on GPIO14 are all wrong for this
board. Its "the variant header wrongly calls it DIO0" warning is *inverted*
here — DIO0 is correct on this board.

**3. Flashing a near-miss firmware target produces a silent failure.** Every
common ESP32 + SX127x target uses NSS 18; this board uses 16. The radio will
initialise cleanly, report no error, and hear nothing.

**It is four wrong pins, not one.** This project recorded the radio as sitting
on the ESP32's default VSPI until 2026-08-21, when the schematic showed
otherwise: SPI is **SCK 14 / MISO 12 / MOSI 13**, and IO18/IO19/IO23 go to the
headers instead. A port that changes NSS to 16 and leaves the bus alone hits the
same silent failure, and the corrected note is the only thing standing between
the next person and a day spent on it.

## What was deliberately not done

- ~~**No flash backup.**~~ **Done 2026-08-21.** The bootloader (`0x1000`), the
  partition table (`0x8000`) and `app0` (`0x10000`) are in
  `.claude_artifacts/flash/`. SPIFFS is deliberately not in that set — an
  application upload never touches it, so the stored gateway config survives
  reflashing, which is why those three are enough to restore a *working*
  gateway.
- ~~**No reflashing.**~~ **The board no longer runs the 2019 firmware.** As of
  2026-08-23 it runs `src/apps/listen` on 915.0 MHz, receive-only, with a
  working link to the SAMD21 Pro RF. Put the original back with:

  ```bash
  ./scripts/restore-firmware.sh
  ```

  That writes the three saved regions and takes about five minutes. Verify with
  `./scripts/read-banner.sh --no-reset` and expect `ESP32 defined,
  freq=903900000`.
- **Six slash commands deleted, not adapted** — `backlog-audit`, `fix-ready`,
  `fix-ticket`, `merge-pr`, `new-issue`, `priority-review`. They target a GitHub
  repo and project board that do not exist here, and `/new-issue` would have
  filed against the *Heltec* tracker. Restore them from
  `../../Heltec/ESP32-LoRa-V3/.claude/commands/` and swap the IDs if a board is
  created.
- **No CH340 datasheet mirrored.** Not a block — the WCH page is a JavaScript
  shell that builds its download list client-side, so `curl` retrieves a 4 KB
  page with no links in it. A browser gets the PDF. Link is in README.
  *(Since fetched by hand into `docs/datasheets/CH340DS1.pdf`.)*
- **No git remote.** Commit locally. Do not report a push that did not happen.

## The local mirror is the only copy

Everything SparkFun links from the product page is mirrored into
`docs/datasheets/` and `etc/firmware/` — schematic, Eagle files, both datasheets
they list, the hookup guide, and all three linked repositories. The board is
2019 hardware retired from a catalogue, so those web assets are not guaranteed
to outlast the project.

**Both directories are gitignored, and there is no git remote.** The mirror
therefore protects against SparkFun deleting an asset, and against nothing else
— not disk loss, not accidental deletion. Nothing versions it. If these files
matter, that gap needs closing deliberately; see the note in `README.md`.

## Environment

Python tooling comes from a pyenv virtualenv named `LoRa`, pinned by
`.python-version`. It holds `esptool` 5.3.1 and `pyserial` 3.5 — enough to
identify and talk to the board. See "Python environment" below.

**PlatformIO is set up.** `platformio.ini` at the root, six environments, and a
`Makefile` front door — `make help`. `pio` itself lives outside any virtualenv
at `~/.local/bin/pio`, shared across every embedded project on the machine; do
not install a second copy. `make test` runs host-side unit tests with no board
attached.

## First commands to run

```bash
cat CLAUDE.md                       # the hardware facts, all of them
esptool --port /dev/ttyUSB1 --no-stub chip-id
./scripts/read-banner.sh            # reset, read the banner, credentials masked
```

Expect `ESP32-D0WDQ6`, `84:0d:8e:0c:5a:80`, `4MB`. **A different MAC means a
different board** — stop and say so rather than carrying on.

## SparkFun's own documentation describes the wrong board

Audited 2026-08-21. **Every SparkFun link, including `products/15006`, now
serves the WRL-18074 page** — the later 16 MB revision. That page links the
ESP32-WROOM-32E datasheet, which is a different module on different silicon
(D0WD-V3, where this board reports D0WDQ6).

The previous notes were partly built from that page, and it is the likely source
of two errors already found: the radio recorded as `RFM95W` where the schematic
says `RFM95HW`, and — indirectly — the habit of trusting SparkFun's text over
the schematic, which is what let the wrong SPI bus stand.

The schematic is the exception: it is dated 14 Nov 2018, is byte-identical to
what SparkFun still serves, and does describe this board. `docs/hardware.md`
lists which datasheets are right and which are not.

## Where the work could go

Nothing is committed to. In rough order of effort:

1. **Leave it as a LoRaWAN gateway** and point it at a private network server
   such as ChirpStack. It already works. Note TTN v3 refuses single-channel
   gateways, so The Things Network is not an option.
2. **Port Meshtastic.** A new `variants/` folder with the pin map plus a
   PlatformIO env. SX1276 is well supported upstream, so this is configuration
   rather than driver work.
3. **Port RNode.** Harder — firmware pins *and* `rnodeconf` provisioning, which
   expects a known product/model pair. The homebrew path is less trodden.

If the goal is a two-radio link test for the wider effort, a second Heltec V3
would cost less effort than porting this board, and the results would transfer
to the other sites.
