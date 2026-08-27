# Project instructions

Working notes for this repository. Facts that were expensive to establish and
are easy to get wrong from memory or from generic documentation.

## What this project is

A **SparkFun LoRa Gateway - 1-Channel (ESP32)**, part number **WRL-15006** — the
original 2019 revision. It arrived already flashed as a single-channel LoRaWAN
gateway and has not been reconfigured since 2019.

It is the second radio in a wider effort tracked in the sibling project
`../../Heltec/ESP32-LoRa-V3`, which covers a Heltec WiFi LoRa 32 V3 at **FTG1**
and the goal of linking it to radios at **SJC** and **SNA**. Read that project's
`CLAUDE.md` for the wide-area context. **Do not copy its hardware facts here** —
see below.

## Hardware facts

Read from the chip, and cross-checked against the SparkFun schematic in
`docs/datasheets/`:

| | |
|---|---|
| MCU | ESP32-D0WDQ6 rev v1.0 (**not** an ESP32-S3), **Xtensa LX6**, dual core, 240 MHz |
| Flash | **4 MB** — the later revision WRL-18074 has 16 MB, which is how this one is dated |
| MAC | `84:0d:8e:0c:5a:80` |
| USB bridge | **CH340C**, `1a86:7523`, kernel driver `ch341` |
| Radio | Hope **RFM95W**, i.e. a **Semtech SX1276** |

**The radio pin map is the single most important fact here**, taken from
SparkFun's own firmware:

```c
const lmic_pinmap lmic_pins = {
  .nss = 16, .rxtx = LMIC_UNUSED_PIN, .rst = 5, .dio = {26, 33, 32},
};
```

NSS 16, RST 5, DIO0 26, DIO1 33, DIO2 32.

**The SPI bus is not VSPI.** This was recorded wrongly as "the ESP32's default
VSPI (SCK 18 / MISO 19 / MOSI 23)" and corrected on 2026-08-21 against the
schematic. The radio sits on the HSPI-side pins:

| Signal | GPIO |
|---|---|
| SCK | **14** |
| MISO | **12** |
| MOSI | **13** |
| NSS | 16 |

Three independent sources agree, and none of them is the sketch above:

- The schematic, `docs/datasheets/sparkfun-lora-gateway-1ch-esp32-schematic.pdf`:
  ESP32 pins `MISO_H/IO12`, `MOSI_H/IO13`, `SCK_H/IO14` carry the nets `MISO`,
  `MOSI`, `SCK`, and the RFM95HW's `MISO`/`MOSI`/`SCK`/`NSS` pins connect to
  `MISO`, `MOSI`, `SCK`, `RFM_CS`. Meanwhile `SCK_V/IO18`, `MISO_V/IO19` and
  `MOSI_V/IO23` carry nets named plainly `IO18`, `IO19`, `IO23` — they go to the
  headers, **not to the radio**.
- SparkFun's Arduino variant, which is why their sketch works without ever
  naming the SPI pins: `Firmware/arduino-esp32/variants/sparkx_esp32_lora/
  pins_arduino.h` sets `SS 16`, `MOSI 13`, `MISO 12`, `SCK 14`.
- The gateway firmware's own "roll your own" comment block,
  `src/loraModem.h:298-301`: `MISO 12 / MOSI 13 / CLK 14 / SS 16`.

This makes the trap below worse than "NSS is 16". A port that fixes NSS and
leaves SPI on the defaults has **four** wrong pins, not one, and still fails
silently.

### Confirmed against the chip, 2026-08-21

`src/tools/probe` reads the SX1276's `RegVersion` (0x42) over each
candidate bus. Run on the board:

```
recorded (schematic)        SCK 14 / MISO 12 / MOSI 13  ->  0x12  SX1276 FOUND
previously assumed (VSPI)   SCK 18 / MISO 19 / MOSI 23  ->  0x00  no answer
```

`0x12` is the SX1276's fixed version byte. `0x00` is a bus with nothing driving
it. **The pin map is no longer document-derived — the radio answered.** And the
wiring this project originally recorded produces exactly the silent failure the
trap below describes.

### RST: neither published pin drives the radio

SparkFun publishes two different reset pins, and the probe says **both are
wrong**:

| Source | Claims | Probe result |
|---|---|---|
| `ESP-1CH-TTN-Device-ABP.ino:66` | `.rst = 5` | pulsing GPIO 5 does not reset the radio |
| Gateway tutorial's `_PIN_OUT 6` block | `rst = 27`, "Reset not used" | pulsing GPIO 27 does not reset the radio |

Method: write a recognisable value to `RegOpMode`, pulse the candidate pin low,
read it back. A real reset restores the register to its default; neither pin
changed it.

So the tutorial's throwaway comment — **"Reset not used"** — is the only
accurate statement anyone published about this pin. Treat both 5 and 27 as
placeholders. If a port needs a hardware reset line, read it off the PCB rather
than trusting either number.

Everything else is confirmed four ways over and now by the chip itself: NSS 16,
SCK 14, MISO 12, MOSI 13, DIO0 26, DIO1 33, DIO2 32.

**No stock firmware target matches that mapping.** Meshtastic has no variant
matching `spark`, `1ch` or `gateway`, and no board in RNode's `Boards.h` uses
`pin_cs = 16`. The common ESP32+SX127x targets (`tlora-v2`, `heltec-v2.1`) all
use NSS 18. Flashing one of those produces a board that initialises cleanly and
hears nothing.

### Nothing from the Heltec project's hardware notes applies

Different radio family, different bridge, different MCU. In particular the
SX1262 facts recorded there are **wrong for this board**: there is no 1.8 V
TCXO setting, DIO2 is not an RF switch control, and the interrupt is DIO0 on
GPIO26 rather than DIO1 on GPIO14. The `DIO0` name that is an error on the
Heltec V3 is correct here.

## The core is Xtensa, and that decides what can run on it

**Xtensa LX6, not RISC-V.** The newer ESP32-C3/C6/H2 are RISC-V and are what
upstream LLVM and upstream Rust support directly. Xtensa is the older
architecture with much weaker toolchain support, so in 2026 a project
advertising "ESP32 support" usually means the RISC-V parts.

Check the architecture before believing any such claim. Surveyed 2026-08-25 in
`docs/alternative-languages.md`: **uLisp** (MIT, current, extensible without
forking) and **MicroPython** run here; **Rust** runs but needs Espressif's LLVM
fork via `espup`; **Zig has no Xtensa target at all**.

That document also records the one trap common to all of them — uLisp's
`with-spi`, and any stock driver, defaults to VSPI 18/19/23 and would hit the
same silent failure described above.

## esptool needs `--no-stub` on this board

The stub flasher fails over this CH340 at 921600 and at 460800:

```
A fatal error occurred: Unable to verify flash chip connection
```

`chip-id` and `flash-id` work normally.

**The failure is rate-specific, not stub-specific.** At 115200 the stub is fine:
PlatformIO uploads through it at about 162 kbit/s. So `--no-stub` is the remedy
for a *bulk read* over this bridge, not a blanket rule about the stub.

Expect a `--no-stub` read to be slow, but not as slow as this file used to say.
Measured: the 1.3 MB app partition read in **243 s, 43.4 kbit/s**, which puts a
full 4 MB at about **12 minutes** rather than the 15–20 previously recorded. Run
it in the background rather than waiting on it.

### A long read dies if anything else opens the port

Nothing in Linux stops two processes opening the same tty. Do it during a bulk
`read-flash` and esptool aborts mid-transfer with:

```
A serial exception error occurred: device reports readiness to read but
returned no data (device disconnected or multiple access on port?)
```

That happened on 2026-08-21 at 16% of an `app0` dump, because a banner capture
was started alongside it. Given the reads here run for several minutes and the
board is the only one on this port, **this is a credible explanation for the
three failed backup attempts** recorded below — more credible than the flash
itself being at fault.

`scripts/lib/common.sh` now has `lock_port`, and `dump-flash.sh` and
`read-banner.sh` both take an advisory lock before touching the port. The lock
only binds scripts in this project, so a stray `miniterm` can still break a
read. If a long read dies for no clear reason, check what else has the port:

```bash
fuser -v /dev/ttyUSB1
```

## The stock firmware prints credentials on every boot

The board runs the **ESP-1ch-Gateway** sketch, which dumps its stored
configuration to serial at 115200 on each boot:

```
.SSID=	<network name>
.PASS=	<password in the clear>
```

Reading the banner therefore captures whatever WiFi credentials were last
stored. **Be careful pasting a boot capture anywhere.**

### Read the right repository: `ESP-1ch-Gateway-v5.0`, not `ESP-1ch-Gateway`

`things4u` publishes **two** repositories with near-identical names, and they
are different code:

| Repo | Last pushed | Is it what this board runs? |
|---|---|---|
| `things4u/ESP-1ch-Gateway-v5.0--OLD` | Sept 2019 | **Yes.** SparkFun links it as `…-v5.0`; GitHub redirects, because the repository was renamed to add `--OLD`. |
| `things4u/ESP-1ch-Gateway` | Oct 2022 | No — a later rewrite. |

Cloning either name gets the same code — the redirect is transparent — but the
canonical name is the `--OLD` one, and that is what the archive root says.

The tell is the boot line. v5.0 prints `. WiFi connect SSID=`; the 2022 tree
prints `. WlanConnect SSID=`. **This board prints `WiFi connect`.** Both are
mirrored in `etc/firmware/`; read the v5.0 one.

This matters beyond pedantry: the first credential-masking pass here was written
against the 2022 tree and missed print sites that only exist in v5.0.

**The config dump is not the only place it prints credentials.** Print sites in
v5.0, the build actually running:

| Output | Source | Maskable by a key pattern? |
|---|---|---|
| `.SSID=` / `.PASS=` config dump | `readConfig` | yes |
| `0:1:2. WiFi connect SSID=<ssid>` | `_WiFi.ino:273` | yes |
| `, pass=<password>` on that same line | `_WiFi.ino:276` — guarded at `debug>=1`, and the stored `.DEBUG=0` is the only thing suppressing it | yes |
| `M WlanWriteWpa:: ssid=<ssid>, pass=<password>` | `_WiFi.ino:178` | yes — and its guard is `debug>=0`, so it prints whenever the config is written |
| `WlanReadWpa: <ssid>, <password>` | `_WiFi.ino:155` | **no — the password follows nothing but `", "`** |
| `WlanReadWpa: <<ssid>>, <<password>>` | `_WiFi.ino:160` | **no** |
| `A WlanStatus:: CONNECTED to <ssid>` | `_WiFi.ino:39` | **no — `to`, not `to=`** |
| ` WiFi Connected to <ssid> on IP=` | `ESP-sc-gway.ino:1112` | **no** |

The last four have no key in front of the value. `scripts/lib/mask.py` handles
them by matching the distinctive line prefix and masking the rest of the line —
which is the only handle that exists. If you add a print site, that is the
technique to copy.

`scripts/export-transcript.sh` now covers all of these. What its patterns
**cannot** cover is the SSID written bare, with no `ssid=` key in front of it —
in prose, or as a string literal in a script. For that, add the value to
`.claude_artifacts/mask-secrets.txt`, which is where literals go; both the SSID
and the password of the stored 2019 network are in it.

Current stored state: `freq=903900000` (US915 uplink **channel 8** — the
first channel of sub-band 2; earlier notes here said channel 0, which is
902.3 MHz and wrong), `SF=7`,
`BOOTS=26`, and an NTP timestamp from March 2019.

## The boot noise is two baud rates, and you cannot read the first one

Resetting the board produces a screenful of mojibake before the banner. That is
not a sync problem and not line noise. The ESP32 speaks at **two rates in
sequence**: the ROM bootloader at **74880** baud (derived from the 40 MHz
crystal), then the sketch at **115200** once `Serial.begin()` runs.

The obvious fix — read the preamble at the rate it is actually sent — **does not
work on this bridge**:

```
$ stty -F /dev/ttyUSB1 74880
stty: invalid argument '74880'
```

The `ch341` driver rejects the rate outright. It is not an accuracy limit: at
the CH340's 12 MHz base, 74880 needs divisor 160.26 and 115200 needs 104.17 —
**the same 0.16% error**. The driver simply does not expose non-standard rates
on this path.

A two-phase read (74880, then switch to 115200) was tried and measured **worse**
than doing nothing: the preamble stayed garbled *and* the switch swallowed the
`ESP32 defined, freq=` line. Do not re-attempt it without first confirming the
driver will set the rate.

So the preamble is unreadable on this board, full stop. `read-banner.sh
--no-reset` gives completely clean output.

**How `--no-reset` actually works, which is not what its name suggests.**
Opening the port resets the board regardless — the `ch341` driver asserts
DTR/RTS on open, and RTS is wired to EN. Setting both lines before `open()`
reduces but does not eliminate this. What the flag really does is skip the
deliberate RTS pulse and then flush the input buffer after 100 ms, by which
time the unreadable ROM preamble has arrived and is discarded. The banner that
follows is clean. You get a boot banner on every `--no-reset` run for exactly
this reason.

**The board does not reboot on its own.** An earlier note here claimed it
restarted every ~20 s because WiFi fails. Measured over 75 s of continuous
listening: **one** banner, then WiFi retries forever without a restart —

```
WlanConnect:: Not connected after all
WLAN retry=1 , stat=6
Error Wifi network connect
```

That is also why `.BOOTS` sits at 26 and never climbs.

## Lab mode desensitises the receiver by 43 dB, measured

`src/apps/listen` toggles the SX1276's own front-end attenuator at runtime —
`l` for lab, `n` for normal, `?` for status. Measured 2026-08-21:

| Mode | Reported floor |
|---|---|
| normal (LNA max, AGC on) | -120 dBm |
| lab (LNA -48 dB, AGC off) | **-77 dBm** |

**The number goes up, and that is the correct direction.** Turning the LNA down
means mixer and ADC noise dominate instead of amplified antenna noise, so the
floor referred to the input rises by nearly the attenuation. The receiver is 43
dB less sensitive — which is the point when the other radio is on the same
bench.

Two traps: figures taken in lab mode are **not comparable** with normal-mode
figures, and lab mode will miss anything weak. The heartbeat prints the active
mode for exactly this reason.

**AGC must be off or none of it applies.** With `AgcAutoOn` set the radio picks
the LNA gain itself and silently ignores `RegLna`. `SX1276::setLnaGain()` clears
AGC as part of its contract.

### Removing the antenna does not move the noise floor

Measured 2026-08-21, whip attached versus pigtail only, same board and firmware,
minutes apart:

| Mode | Whip on | Whip off |
|---|---|---|
| normal | -121 dBm | -121 dBm |
| lab | -78 dBm | -78 dBm |

No change in either. "Remove the antenna to reduce noise" is a natural
assumption and it is wrong about which quantity moves: the idle floor here is
set by the receiver's own mixer and ADC noise, and internal noise does not care
what is on the connector.

What removing the whip *does* change is received **signal**, by 20–40 dB. That
appears in the RSSI and SNR of an actual packet and never in the idle floor.
**So the floor is the wrong instrument for checking whether an antenna change
worked.**

## A receiver will invent packets if you let it

Two false packets were reported on 2026-08-21, 159 and 61 bytes, and they were
noise. Three things gave them away, and any one of them is enough:

| | |
|---|---|
| **SNR −14.2 and −13.8 dB at SF7** | SF7 demodulates to about **−7.5 dB**. Six dB below the floor is not a weak signal, it is not a signal. |
| Payload uniformly random | No header, no repeated field, no readable text |
| Lengths arbitrary | 159 and 61, from a header that was itself noise |

**Rough demodulation floors**, worth keeping to hand: SF7 −7.5 dB, and roughly
2.5 dB more per spreading factor, reaching about −20 dB at SF12. An SNR below
the floor for the spreading factor in use means the "packet" is not one.

### Why the CRC did not catch it

Because there was no CRC. In **explicit-header** mode the CRC-present flag comes
from the *received* header — it is the transmitter's choice. `RxPayloadCrcOn` in
`RegModemConfig2` only governs what we send. When noise decodes into a header
with CRC off, `PayloadCrcError` can never assert, and a corrupt packet looks
perfect.

`SX1276::poll()` now reads **`RegHopChannel` bit 6 (`CrcOnPayload`)** and sets
`Packet::crcPresent`. Packets without a CRC are counted separately
(`uncheckedPackets()`) and labelled `*** NO CRC -- UNVERIFIED ***` rather than
dropped, because a transmitter that genuinely sends without CRC is legitimate —
but so is noise, and only a person can tell which.

**Do not treat a CRC-less packet as data.** Nothing verified those bytes.

## The radio's MISO is GPIO12, which is the flash-voltage strapping pin

**This is the worst trap on the board, and it makes flashing fail intermittently
in a way that looks like a bad cable.**

Three facts already recorded here, which turn out to be one problem:

1. The radio's MISO is on **GPIO12** (schematic: `MISO_H/IO12` → net `MISO`).
2. On the ESP32, **GPIO12 is MTDI**, the VDD_SDIO strapping pin. Sampled HIGH at
   reset it configures the flash rail for **1.8 V**; low or floating gives 3.3 V.
3. The radio has **no usable reset pin** — `src/tools/probe` showed neither of
   SparkFun's published candidates does anything.

`espefuse summary` confirms nothing overrides it:

```
XPD_SDIO_FORCE (BLOCK0)  Ignore MTDI pin (GPIO12) for VDD_SDIO on reset = False
Flash voltage (VDD_SDIO) determined by GPIO12 on reset (High for 1.8V, Low/NC for 3.3V)
```

So **the SX1276's MISO line decides the ESP32's flash voltage at every reset.**
When it is high, the 3.3 V flash chip is driven at 1.8 V and esptool reports:

```
Warning: Failed to communicate with the flash chip, read/write operations will
fail. Try checking the chip connections or removing any other hardware
connected to IOs.
A fatal error occurred: Serial data stream stopped: Possible serial noise or corruption.
```

`chip-id` still works, because that never touches flash. That combination —
identification fine, bulk transfer failing — is the signature.

**It has a second face, at boot.** The same strap sampled high when the board
resets leaves the ROM bootloader unable to read flash at all, and it loops:

```
rst:0x10 (RTCWDT_RTC_RESET),boot:0x33 (SPI_FAST_FLASH_BOOT)
flash read err, 1000
ets_main.c 371
```

This happens *after a successful, hash-verified upload*, so the instinct is to
blame the firmware you just wrote. It is not the firmware. Seen 2026-08-21
immediately after flashing `listen` over `gateway`, both images verified.

Either symptom, same cause, same fix.

### What to do

**Power-cycle the board: unplug the USB and plug it back in.** Because the radio
has no reset line, a warm reset does not clear its state; only removing power
does. This is the fix, and it costs five seconds.

Expect to do it more than once. The radio's MISO is high-impedance whenever NSS
is deasserted, so GPIO12 is *floating* at reset rather than driven either way —
which is why this is intermittent rather than constant, and why it cannot be
fixed in firmware. Code runs long after the strap has been sampled.

Do not conclude the cable or the CH340 is bad until you have tried it. Observed
2026-08-21 after several successful flashes in a row, with no cable change.

### The permanent fix, which nobody has authorised

Burning `XPD_SDIO_FORCE` + `XPD_SDIO_TIEH` pins the flash rail at 3.3 V and
makes GPIO12 irrelevant at boot. **eFuses are one-way** — get the value wrong
and the board does not boot again, with no recovery. Do not do it without
explicit instruction from the repository owner.

## Credentials are compiled into the firmware, not just stored in SPIFFS

Verified 2026-08-21 by dumping `app0` and reading it, so this is a fact about
the image rather than an inference from source.

`ESP-sc-gway.h:261` defines a **compile-time** table:

```c
struct wpas { char login[32]; char passw[64]; };
wpas wpa[] = { { "", "" }, { "fire", "water" }, { "ape", "beer" } };
```

Upstream ships `fire`/`water` and `ape`/`beer` as placeholders. This board's
image holds a different pair at **flash `0x3f478`** (app0 offset `0x2f478`),
inside the ~880 KB of the 1.3 MB partition that is actually used: an SSID and
the ten-digit password `<redacted>`.

**Redacted from this repository.** Git history was rewritten with
`git filter-repo` on 2026-08-24, before the first push, so the literals appear
in no commit. They live in `.claude_artifacts/mask-secrets.txt`, which is
gitignored, and can always be re-read from the board. **Do not paste them back
into a tracked file** — this repo is public.

**These are not live credentials.** The repository owner's assessment is that
they are test or production-line values that came with the flashed firmware,
and the password supports that reading. They are not something to protect.

The structural point is worth keeping, because it is what the next person will
get wrong:

- **A dump of the app partition is not automatically credential-free.**
  `dump-flash.sh` originally asserted it was, on the reasonable-sounding
  grounds that the sketch keeps its config in SPIFFS and the WiFi stack keeps
  its copy in NVS. Compile-time tables defeat that reasoning. Reflash this
  board with real credentials in `wpa[]` and the app image will carry them.
- **The SPIFFS config and the `wpa[]` table are independent stores holding the
  same values here** — both `<redacted-ssid>` / `<redacted>`. An earlier note in
  this file claimed they were *different* passwords; that was wrong. It came
  from assuming an unrelated literal already sitting in `mask-secrets.txt` was
  this board's, when it belongs to another project. Erasing one store does not
  touch the other, which is why erasing SPIFFS does not stop the board trying
  to connect: the compiled table still has the same credentials.

## Nothing on the board needed preserving, and it has now been reflashed

**Past tense: the stock sketch is no longer what runs.** Since 2026-08-23 the
board runs this project's own firmware — see README "Current state".

The original was worth nothing unique: it is the stock sketch, mirrored in
`etc/firmware/`, and its only local content was a SPIFFS config holding 2019
credentials for a network nobody uses. It was backed up anyway before flashing,
because that cost five minutes and made the decision reversible:

| Region | File |
|---|---|
| `0x1000` bootloader | `.claude_artifacts/flash/bootloader-0x1000-28K.bin` |
| `0x8000` partition table | `.claude_artifacts/flash/partition-table.bin` |
| `0x10000` app0 | `.claude_artifacts/flash/app0-0x10000-1.3M.bin` |

`./scripts/restore-firmware.sh` writes those three back. **SPIFFS is
deliberately not in that set and does not need to be** — an application upload
never touches it, which is why a restore brings back a *configured* gateway
rather than a blank one.

## The port number moves; detect the board, do not name it

`/dev/ttyUSBn` is assigned in enumeration order, and this board gets
power-cycled often enough (see GPIO12 above) that the number changes under you.
Observed 2026-08-23: **ttyUSB1 → ttyUSB2 → ttyUSB1 within one evening**, same
physical socket throughout.

When it moved, the shell scripts kept working and `make flash` broke — because
`scripts/lib/common.sh` detects by vendor/model while `platformio.ini` had the
path hardcoded. Both now use `scripts/find-port.sh`, which greps udev for
`1a86:7523`.

**A by-id path is not available here.** The CH340C carries no serial number, so
udev cannot build one — unlike the sibling ESP32 Thing Plus, whose CP2102N does
and which therefore uses `/dev/serial/by-id/...`. `/dev/serial/by-path/` is
stable per physical USB socket but changes if the cable moves. Detection by
vendor/model survives both.

## The link works, and the peer's failure is worth knowing

A 915.0 MHz link to the SAMD21 Pro RF in `../SAMD21-LoRa-ProRF` was brought up
2026-08-23. Both ends agree on **915.0 MHz / SF7 / BW 125 kHz / sync 0x12**, and
all four must match or nothing arrives with no error at either end.

**Nothing on this side was ever wrong.** The silence was the Pro RF transmitting
into `RFO`, a pin the RFM95W does not connect. RadioHead's signature is
`setTxPower(int8_t power, bool useRFO = false)`, SparkFun's hookup guide prints
`setTxPower(14, true)`, and that `true` routes the output nowhere. Nothing
reported an error: the radio entered TX, `send()` returned true, the console
said "sent".

**The same idiom is all over SparkFun and Adafruit examples.** `SX1276::begin()`
therefore reads `RegPaConfig` back and exposes `paBoostConfirmed()`; bit 7 must
be 1. Checking a register you just wrote is cheap next to losing a day.

Their frame, decoded by `src/apps/listen`: four bytes of RadioHead header, then
magic `0x4A`, version, type, nodeId, `seq` as uint32 little-endian, then a
NUL-terminated ASCII `$PJPB…` record from byte 12.

## TTN v3 will not accept this as a gateway

SparkFun's own product listing states that The Things Network v3 does not
support single-channel gateways. It can still speak LoRaWAN to a private network
server such as ChirpStack. This constrains any gateway work — plan for a private
server, not TTN.

## Slash commands

`.claude/commands/` came from the Heltec project and has been adapted. All
eleven are present. Six of them — `new-issue`, `fix-ticket`, `fix-ready`,
`merge-pr`, `backlog-audit`, `priority-review` — were deleted in August 2026 as
Heltec-bound and **re-adapted on 2026-08-27** in preparation for a public
repository and Project.

**The long field and option IDs live in one place**,
`.claude/board-config.md`, which is deliberately *not* under `commands/` so it
does not become a slash command. The Heltec originals repeat the same project
and field IDs in all six files, so changing a field there means six edits and
one gets missed. That file carries the `gh project field-list` call that prints
every ID, if they ever need re-reading.

What changed in adaptation, beyond the identifiers:

- **Gates.** Heltec runs `shellcheck` and `pio run`. Here it is `make check`,
  `make build` and **`make test`** — this project has host-side unit tests and
  that project does not. `merge-pr` treats `make test` as mandatory for changes
  touching `lib/loramath/` or any register arithmetic, because a wrong band-plan
  channel number is exactly the error that shipped here.
- **Labels and milestones** are this project's, not Meshtastic and Reticulum:
  `radio`, `firmware`, `tooling`, `interop`; `Solo bench` versus
  `Two-radio link`.
- **A power cycle is a distinct blocked state.** The GPIO12 strap means a flash
  can fail for reasons no command can fix, and `fix-ready` tells the model to
  ask the user to unplug the board rather than treating it as a failure.
- **Masking.** The commands are warned not to paste unmasked serial output into
  an issue or PR, because this board prints WiFi credentials and has leaked them
  into a session log twice.

`scripts/export-transcript.sh` is ported, with only the output helpers it needs
in `scripts/lib/common.sh` rather than the whole of the Heltec common library —
the rest of that file is CP2102N and udev handling that does not apply. Its
masking patterns include the `.SSID=` / `.PASS=` shape this board emits.

## The repository and the project

Both created 2026-08-27, both **public**:

| | |
|---|---|
| Repository | <https://github.com/JeffreyPeacock/ESP32-LoRa-1Ch-Gateway> |
| Project | **#13**, <https://github.com/users/JeffreyPeacock/projects/13> |

The project was copied from the sibling **SAMD21-LoRa-ProRF** board's project
#11 and adapted, so the two share a structure: a `Status` field
(Backlog / Prioritized / Ready / In Progress / Completed), `Priority` p1–p5, and
an **`Area`** single-select in place of topical labels. Two of the Pro RF's areas
were dropped because this board lacks the hardware — `Qwiic` and `Logging` — and
`Hardware`, `LoRaWAN` and `Interop` were added.

**The history here starts at one commit.** The previous 45 were discarded when
the repository was re-initialised before publication: an earlier scrub had used
`git filter-repo --replace-text`, which rewrites blob contents but **not commit
messages**, and every check used `git log -S`, a pickaxe over diffs that cannot
see message text — so two messages still named the stored WiFi credentials and
nothing looked. If you ever verify a scrub, **check messages and contents
separately**. The old messages are preserved, masked, in
`docs/historical-commits.md`.

## Commits

**Do not add Claude attribution to commits** — no `Co-Authored-By` trailer, no
"Generated with" line. Standing instruction from the repository owner.
