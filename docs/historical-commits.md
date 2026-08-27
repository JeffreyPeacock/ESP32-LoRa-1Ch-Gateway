# Historical commit messages

**Why this file exists.** This repository was re-initialised before its first
publication, so the commits below no longer exist as git history — the current
history starts from a single initial commit.

That was a deliberate trade. The old history was clean of secrets in its *file
contents*, but two commit messages still carried the board's stored WiFi
credentials, and re-initialising was the certain fix. What it costs is the
reasoning: on this project the product is largely findings, and a commit message
is where the reasoning behind a finding was written down. **Most of it has been
folded into `CLAUDE.md` and the documents under `docs/`. This file preserves the
rest verbatim, in order, so nothing is lost.**

Read it when you want to know *why* something is the way it is and the current
documents only tell you *that* it is. Several entries record a belief that was
later shown wrong, and the measurement that overturned it — which is more useful
than the conclusion alone.

**Credentials are masked.** Two messages named the stored SSID and password;
those are replaced with `<redacted-ssid>` and `<redacted>`. The values are not
secret in any meaningful sense — the owner's assessment is that they are
production-line test values — but the SSID names a real third-party makerspace
and this repository is public. They remain readable locally in the gitignored
`.claude_artifacts/mask-secrets.txt`, and off the board itself.

**These are historical. Where a message and a current document disagree, the
document wins** — several claims here were corrected by later work, and a few
were corrected only after this file's contents were written.

The dates and short SHAs are from the pre-reset history and refer to nothing you
can `git show`.

---

## 2026-08-21 — Initial commit: SparkFun LoRa Gateway 1-Channel (ESP32)

<!-- was 9b409fe -->

Splits this board out of the Heltec ESP32-LoRa-V3 project, where it had been
recorded as "the second radio". It is a distinct board with a distinct radio
family and deserves its own notes rather than a footnote in another project's.

Identified from the chip rather than the label: ESP32-D0WDQ6, 4 MB flash,
CH340C bridge, RFM95W radio. The 4 MB dates it to the original 2019 revision
WRL-15006; the later WRL-18074 carries 16 MB. It was first mistaken for the
SparkFun Pro RF LoRa SAMD21, which esptool would not have talked to at all.

The fact that governs everything downstream is the radio pin map -- NSS 16,
RST 5, DIO0 26 -- which matches no stock firmware target. Meshtastic has no
variant for this board and no RNode board uses pin_cs 16, while every common
ESP32 + SX127x target expects NSS 18. Either stack needs a custom variant, and
flashing a near-miss target would give a radio that initialises cleanly and
hears nothing.

Also recorded: esptool's stub flasher fails over this CH340 and needs
--no-stub for bulk transfers; the stock gateway sketch prints its stored WiFi
credentials on every boot; and TTN v3 refuses single-channel gateways, so any
LoRaWAN work here needs a private network server.

Datasheets and firmware sources are fetched rather than tracked -- large
binaries, and not ours to redistribute. URLs are in README.

.claude/ is copied verbatim from the Heltec project and its commands still
reference that project's repository and board IDs. CLAUDE.md and README both
say so; they need adapting before use.


---

## 2026-08-21 — Remove the CLion skeleton and adapt the inherited slash commands

<!-- was 3f0561a -->

The directory has been renamed to ESP32-LoRa-1Ch-Gateway.

CMakeLists.txt and main.cpp were CLion's default hello-world, not code anyone
wrote, and cmake-build-debug/ was its output. All removed. The CLion project
files that name the old directory -- .name, the .iml, modules.xml and the CMake
settings in misc.xml -- are now untracked and gitignored, since CLion
regenerates them and the directory has already been renamed once.

Six slash commands are deleted rather than adapted: backlog-audit, fix-ready,
fix-ticket, merge-pr, new-issue and priority-review. Every one is bound to a
GitHub repository, project board and field IDs that do not exist here, and
leaving them in place meant /new-issue would have filed against the Heltec
tracker. Restoring them from the sibling project is a copy and an ID swap if a
board is ever created.

The five that remain are adapted rather than copied. board-check is rewritten
around this board: how to tell a CH340C from the Heltec's CP2102N when both are
attached, why esptool needs --no-stub, what the stock banner looks like, and a
warning not to paste it anywhere because it contains WiFi credentials.
prep-compaction loses its tracker step and its README-table step and gains the
credential-leak check. trim-claude-md's worked examples were still the Heltec's
ADC_CTRL and SX1262 findings; they are replaced with this board's.

scripts/export-transcript.sh is ported so the masking is available locally,
which matters more here than in the sibling project because this firmware emits
credentials on every boot. It brings only the output helpers it needs, in
scripts/lib/common.sh -- the rest of the Heltec common library is CP2102N and
udev handling that does not apply to a CH340C.


---

## 2026-08-21 — Correct the project name in README after the directory rename

<!-- was 4091b26 -->



---

## 2026-08-21 — Docs: add a handoff guide and record the pyenv setup

<!-- was 8e5c565 -->

docs/handoff.md is written for someone arriving cold. It separates what was
verified by reading the hardware from what was inferred, because the two are
easy to confuse here and the inferences are load-bearing: the board revision
rests only on a 4 MB flash reading, the radio part number comes from the
schematic rather than the module, and the decision not to back up the flash
followed three failed backup attempts, so it is partly rationalisation.

It also records what was deliberately not done, which is otherwise invisible:
no reflashing, no backup, six slash commands deleted rather than adapted, and
no git remote.

README gains the pyenv setup for the LoRa virtualenv the project is pinned to,
which now holds esptool 5.3.1 and pyserial 3.5 -- verified against the board.
It notes that pyenv is a preference rather than a requirement, and that
PlatformIO deliberately lives outside any virtualenv.


---

## 2026-08-21 — Docs: correct why the CH340 datasheet was not mirrored

<!-- was 239842c -->

It was not a refused or blocked request. Both WCH URLs return HTTP 200 with
content-type text/html: the page is a ~4 KB JavaScript shell that builds its
download list client-side, so curl retrieves the shell rather than the PDF.
Saying the endpoint 'refused a scripted fetch' implied an access restriction
that does not exist and would send the next person looking for a workaround
that is not needed.


---

## 2026-08-21 — Mask every SSID print site, not just the config dump

<!-- was 75f94bc -->

The banner-masking only matched the two line-anchored ".SSID=" / ".PASS="
config-dump lines. The ESP-1ch-Gateway sketch prints the SSID five more
times at runtime with text ahead of the key, so the anchored patterns
missed all of them and the SSID went into a session log in the clear for
the second time.

Enumerated the print sites from the mirrored firmware source rather than
patching the one shape that leaked:

  src/_WiFi.ino:254      ". WiFi connect SSID=<ssid>"
  src/_WiFi.ino:257      ", pass=<password>"  -- only when debug >= 2
  src/_WiFi.ino:37       "WlanStatus:: CONNECTED ssid=<ssid>"
  src/ESP-sc-gway.ino:420    "setup:: WlanConnect=<ssid>"
  src/_udpSemtech.ino:603    "sendUdp: WlanConnect connected to=<ssid>"
  src/_wwwServer.ino:479     "Set Variables:: ssid=<ssid>"

The board stores debug=1, so the password has only ever reached serial on
the .PASS= line; debug=2 would put it on the runtime line too.

export-transcript.sh gains unanchored key patterns for these. A bare SSID
with no key in front of it stays out of reach of any pattern and needs a
literal in mask-secrets.txt instead. Verified end to end: the export of
the session that leaked now reports no residual and no occurrences.

Also recorded, from running the board-check: RTS resets this board and
DTR alone does not, so step 3 no longer needs an interactive miniterm.


---

## 2026-08-21 — Correct the radio SPI bus: HSPI pins, not the VSPI defaults

<!-- was 3d10ca7 -->

CLAUDE.md called the pin map "the single most important fact here" and
then got half of it wrong. It recorded the radio as sitting on the
ESP32's default VSPI bus, SCK 18 / MISO 19 / MOSI 23. The schematic in
docs/datasheets says otherwise:

  MISO_H/IO12 -> net MISO      RFM95HW MISO
  MOSI_H/IO13 -> net MOSI      RFM95HW MOSI
  SCK_H/IO14  -> net SCK       RFM95HW SCK
  IO16        -> net RFM_CS    RFM95HW NSS

while SCK_V/IO18, MISO_V/IO19 and MOSI_V/IO23 carry nets named plainly
IO18, IO19 and IO23 -- they go to the headers, not to the radio.

Two sources corroborate, neither of them the SparkFun sketch the old
note was taken from: SparkFun's Arduino variant
Firmware/arduino-esp32/variants/sparkx_esp32_lora/pins_arduino.h
(SS 16, MOSI 13, MISO 12, SCK 14), which is why their sketch never names
the SPI pins and still works, and the gateway firmware's own roll-your-own
comment block at src/loraModem.h:298-301.

This makes trap 3 worse than recorded. A Meshtastic or RNode port that
sets NSS to 16 and leaves SPI on the defaults has four wrong pins rather
than one, and still gets a radio that initialises cleanly and hears
nothing.

RST 5 is left as firmware-stated and explicitly not schematic-verified:
the RFM_RST net extracts only at the module end.

Also from the schematic: the module is labelled RFM95HW, the +20 dBm
build, not plain RFM95W. Same SX1276 silicon, so ports are unaffected,
but transmit power and duty-cycle arithmetic are not. And hardware.md's
bare "#14" pointed at a tracker this repository does not have; it is the
sibling Heltec project's issue, now named as such.


---

## 2026-08-21 — Add read-banner.sh; audit SparkFun's docs against the actual board

<!-- was 56d8222 -->

read-banner.sh resets the gateway over RTS, reads the boot banner and
masks the WiFi credentials before printing them. It autodetects the CH340
by vendor/model, refuses to guess when two are attached (they carry no
serial number), reports which firmware is running, and runs python from
PROJECT_DIR so pyenv's .python-version applies wherever it is invoked.

The masking patterns moved to scripts/lib/mask.py, imported by both this
and export-transcript.sh. Copying them would have been the second copy of
a rule set that has already leaked the SSID twice, once because one copy
was less complete than the other.

Documentation audit, prompted by the observation that the prior notes were
partly built from SparkFun's product page:

- Every SparkFun link, products/15006 included, now serves the WRL-18074
  page: 16 MB flash, ESP32-WROOM-32E datasheet. This board is 4 MB and
  reports D0WDQ6, so that is the wrong revision and the wrong module.
  Recorded so the next person does not follow those links in good faith.
- The schematic is the exception and is trustworthy: dated 14 Nov 2018,
  byte-identical to what SparkFun still serves. It also means there is
  only one schematic, so it cannot distinguish the two revisions.
- The schematic names the module ESP-WROOM-32NARROW and the bridge CH340C.
- RFM95HW (schematic) vs RFM95W (product page) is a real conflict and is
  now recorded as unresolved rather than settled either way. Same SX1276
  silicon; transmit power and duty-cycle arithmetic differ.

Datasheets added: Semtech SX1276/77/78/79 rev V7, the register reference a
firmware port needs, and ESP32-WROOM-32 v3.7, the module actually fitted.
CH432DS1.pdf is noted as not this board's -- the CH432 is a dual UART and
appears nowhere in the schematic.


---

## 2026-08-21 — Mirror every linked asset; read the firmware repo the board actually runs

<!-- was ac695d0 -->

Downloaded everything the SparkFun product page links that was missing:
the Eagle files, the WROOM-32E datasheet (labelled as the wrong revision),
the hookup guide, mcci-catena/arduino-lmic, and things4u/ESP-1ch-Gateway-v5.0.
SparkFun's RFM95 PDF is byte-identical to the copy already held.

The v5.0 download exposed a wrong assumption. things4u publishes TWO
repositories with near-identical names:

  things4u/ESP-1ch-Gateway-v5.0   Sept 2019   what SparkFun links, and what
                                              this board runs
  things4u/ESP-1ch-Gateway        Oct 2022    a later rewrite

Every firmware fact recorded here so far came from the 2022 tree. The tell
was visible in the boot capture and went unread: the board prints
". WiFi connect SSID=", the 2022 tree prints ". WlanConnect SSID=".

Re-deriving the credential print sites from v5.0 found four that the
masking could not reach, because the value has no key in front of it:

  _WiFi.ino:155   "WlanReadWpa: <ssid>, <password>"   password after ", "
  _WiFi.ino:160   "WlanReadWpa: <<ssid>>, <<password>>"
  _WiFi.ino:39    "A WlanStatus:: CONNECTED to <ssid>"    "to", not "to="
  ESP-sc-gway.ino:1112  " WiFi Connected to <ssid> on IP="

lib/mask.py now matches the line prefix and masks the rest of the line,
which is the only handle these shapes offer. Also fixed a guard that sat
after a greedy \s* and so let an already-masked WlanReadWpa line match
twice -- residual() reported a leak that was not there, and read-banner.sh
treats any residual as fatal. Verified against the board and idempotent
under re-masking.

Also noted: v5.0 guards WlanWriteWpa's ssid=/pass= print at debug>=0, not
debug>=2, so it prints whenever the config is written.


---

## 2026-08-21 — Ship the provenance, not the payload: asset manifest with hashes

<!-- was e114497 -->

This board is 2019 hardware retired from SparkFun's catalogue, and vendor
download pages for parts that old get reorganised, moved behind JavaScript
and eventually deleted. Mirroring the files locally guards against that;
it does not help anyone else, and the files are third-party documents that
are mostly not licensed for redistribution.

So the repository carries the provenance instead:

  docs/assets.md    every document -- source URL, what it is for, and the
                    traps around it
  docs/SHA256SUMS   sha256sum -c manifest, paths relative to project root

Fourteen assets recorded and verified. The binaries stay gitignored;
docs/sparkfun-hookup-guide.html is now ignored too, since it was a mirrored
vendor page that the existing rules did not cover and would have been
committed.

Two things the manifest records that a bare hash list would not:

- GitHub archive zips are not byte-stable, so for the four source archives
  the commit SHA is given as the durable identifier and the zip hash is
  marked as "what we downloaded on this date".
- The two things4u repositories are distinguished explicitly, with the
  boot-line tell that tells them apart, because taking the wrong one gives
  source that does not match this board.

Corroboration worth keeping: ESP-1ch-Gateway-v5.0's head commit is dated
2019-03-29 and this board's stored NTP timestamp is 2019-03-25.

Renamed things4u-ESP-1ch-Gateway-main.zip to -master.zip; its archive root
was always -master and the default branch is master, so the old name was
simply wrong.


---

## 2026-08-21 — Verify firmware against the flash; find credentials compiled into the image

<!-- was 365cba1 -->

Re-verified the code/board match from both ends.

From source: matched the strings the board prints against both candidate
trees. v5.0 has 13 of 14; the 2022 tree has 8, and the one marker v5.0
lacks is the string that identifies the 2022 tree. Also resolved the repo
name -- SparkFun links ESP-1ch-Gateway-v5.0, which REDIRECTS to
ESP-1ch-Gateway-v5.0--OLD. The code mirrored here was already right; only
the canonical name was wrong, now corrected in three files.

From the hardware: added dump-flash.sh and identify-firmware.sh. The first
reads the real partition table rather than assuming a layout, then dumps a
named partition. The second scores a flash image against each mirrored
source tree using literals distinctive to exactly one tree, so shared
boilerplate cannot inflate a match. On the app0 image:

  things4u-ESP-1ch-Gateway-v5.0-master.zip   116/175   66.3%
  things4u-ESP-1ch-Gateway-master.zip         10/335    3.0%
  mcci-catena-arduino-lmic-master.zip          6/254    2.4%

66.3% rather than 100% because feature-guarded blocks were compiled out.

Three corrections this turned up, all to claims made earlier in this work:

- dump-flash.sh asserted the app partition holds no credentials. It does.
  ESP-sc-gway.h:261 declares a compile-time wpa[] table, and this image
  carries the previous owner's SSID and a password at flash 0x3f478 --
  upstream's placeholders are fire/water and ape/beer, so these are real.
  There are TWO different passwords on this board: one in SPIFFS that the
  banner prints, and this one. Every region is sensitive; the script and
  CLAUDE.md now say so.

- mask.py labelled masked literals with their first six characters, which
  printed most of a ten-digit password into the report that claimed to have
  masked it. Labels are now a truncated SHA-256 plus a length.

- A dump died at 16% because two scripts held the port at once. Both now
  take an advisory flock. This is a likelier cause of the three failed
  backup attempts in the notes than a hardware fault.

Measured timings replace the guesses: app partition 1.3 MB in 243 s
(43.4 kbit/s), so the full 4 MB is about 12 minutes, not 15-20.


---

## 2026-08-21 — Reframe the wpa[] pair as test values, per the repository owner

<!-- was 162d4bd -->

The compile-time credentials found at flash 0x3f478 are not live: they are
test or production-line values that shipped with the firmware, and the
password <redacted> supports that. The notes said "the previous owner's
real SSID and a real password" and treated every dump as sensitive, which
overstated it.

Kept, because it is the part that generalises: dump-flash.sh had claimed
the app partition was credential-free on the grounds that config lives in
SPIFFS and NVS. A compile-time wpa[] table defeats that reasoning whatever
the values happen to be, so the warning now fires for every region and
explains the mechanism instead of asserting a specific secret.


---

## 2026-08-21 — read-banner.sh: add --no-reset to avoid the ROM bootloader noise

<!-- was 6041a6d -->

The mojibake before the banner is not a sync problem. The ESP32 talks at
two rates in sequence: the ROM bootloader at 74880 baud, then the sketch at
115200 once Serial.begin() runs. At 115200 the preamble is unreadable.

Reading the preamble at the rate it is actually sent does not work here:

  $ stty -F /dev/ttyUSB1 74880
  stty: invalid argument '74880'

The ch341 driver rejects it. That is not an accuracy limit -- at the CH340's
12 MHz base, 74880 needs divisor 160.26 and 115200 needs 104.17, the same
0.16% error. The driver just does not expose non-standard rates on this path.

A two-phase read (74880, then switch) was implemented and measured WORSE
than doing nothing: the preamble stayed garbled and the switch swallowed the
"ESP32 defined, freq=" line. Reverted, and the finding recorded so nobody
tries it again without first checking the driver accepts the rate.

--no-reset is what actually works. It never triggers the ROM phase, so the
output is completely clean. It also sets DTR/RTS before open() rather than
after, because opening a port normally asserts both and RTS is wired to EN
on this board -- so the mere act of opening would reset the chip.

Little is lost by not resetting: the board reboots itself roughly every 20
seconds, since the WiFi network in its stored config has not existed since
2019 and the sketch resets on connection failure. A --no-reset capture still
picked up a full banner, cleanly.

Also fixed a bug introduced with the help text: usage() uses an unquoted
heredoc, so backticks in it were executed. The new text ran
'stty -F /dev/ttyUSB1 74880' on every --help. Backticks removed; the other
three scripts were checked and are clean.

The firmware verdict now distinguishes "no banner because --no-reset" from
"no banner because the board was reflashed", which would otherwise have been
a false alarm on every --no-reset run.


---

## 2026-08-21 — Correct two wrong claims; add the SparkX repo and this board's tutorial

<!-- was 76bbfd8 -->

Two things recorded earlier in this work were wrong, both now fixed with
the measurement that settles them.

"Two different passwords on this board." There is one pair. SPIFFS and the
compiled wpa[] table both hold <redacted-ssid> / <redacted>. The error came
from assuming an unrelated literal already in mask-secrets.txt was this
board's SPIFFS password; it belongs to another project and appears nowhere
in this flash. The independent-stores point still stands and is why erasing
SPIFFS would not stop the board connecting -- the compiled table survives it.

"The board reboots itself every ~20 seconds because WiFi fails." It does
not. Measured over 75 s of continuous listening: one banner, then WiFi
retries forever without a restart ("WlanConnect:: Not connected after all",
"WLAN retry=1"). That is also why .BOOTS sits at 26 and never climbs.

Which forced a correction to how --no-reset works, since its name had been
taken at face value: opening the port resets the board anyway, because the
ch341 driver asserts DTR/RTS on open and RTS is wired to EN. The flag skips
the deliberate RTS pulse and flushes the buffer after 100 ms, discarding the
unreadable ROM preamble. That is why every --no-reset run still yields a
banner.

New assets, found via the hookup guide's own pointer to the older SKU:

- sparkfunX/ESP32_LoRa_1CH_Gateway -- SparkX, "1CH", the original
  SPX-14893/WRL-15006 board rather than the productised 18074 line. Carries
  Production/ send and receive sketches, useful for a link test. Its
  sparkx_esp32_lora variant is a third independent confirmation of
  SS 16 / MOSI 13 / MISO 12 / SCK 14.
- The tutorial for THIS board, "ESP32 LoRa 1-CH Gateway, LoRaWAN, and the
  Things Network", which the mirrored hookup guide names for SPX-14893.

Probed from the flash while there: firmware VERSION "V.5.3.3.H; 180825a"
and Arduino core IDF v3.3-beta1-179-ge931fe9f5-dirty. The sketch has no
serial command interface at all -- nothing reads Serial -- so the version is
only obtainable from the image or the web UI.

Manifest and SHA256SUMS updated; 16 assets verify. The new tutorial page was
not covered by the ignore rules and would have been committed.


---

## 2026-08-21 — Answer "how was this configured, and how do we do it again"

<!-- was 70d28ea -->

docs/programming.md reconstructs it from the source, the flash image and
SparkFun's tutorial for THIS SKU.

The answer is: by recompiling. There is no runtime configuration path on
this build. The sketch never reads serial -- no Serial.available() or
Serial.read() anywhere. WIFIMANAGER is 0, and WiFiManager/AutoConnect/
captive/configPortal are all absent from the image, so there is no AP
portal. The web UI can change settings but needs WiFi to already work.

Credentials come from the compile-time wpa[] table in ESP-sc-gway.h:261.
SPIFFS is a CACHE of that, not a second source: after a successful connect
_WiFi.ino:298 calls writeGwayCfg(), which stores WiFi.SSID() and WiFi.psk()
(_loraFiles.ino:221). Hence both stores holding identical values, and hence
erasing SPIFFS would not stop the board connecting.

The digits on the connect line turn out to be the slot walk, not a
timestamp: "0:1:2." is retry 0, slot 1, of 2 total slots. So this build has
two networks compiled in and uses the second; slot 0 is the blank reserved
entry.

Pinned the exact source revision. The image reports V.5.3.3.H; 180825a, and
that string appears in exactly one commit: 88113e8, 2018-08-26, nine commits
behind HEAD. Arduino core was IDF v3.3-beta1-179-ge931fe9f5-dirty.

Recorded the full build recipe, including the part that is easy to get
wrong: upstream loraModem.h has _PIN_OUT 1-5 and no 6, so the tutorial has
you delete the #error line and paste a block in its place. The SparkX
Arduino variant is what supplies SS 16 / MOSI 13 / MISO 12 / SCK 14 -- the
sketch never names the SPI pins, which is precisely why a stock board
definition yields a radio that initialises and hears nothing.

That block also says rst = 27, commented "Reset not used", where SparkFun's
own LMIC sketch says .rst = 5. Two published values, one annotated unused,
and the schematic's RFM_RST net never resolved at the ESP32 end. Recorded as
an open contradiction rather than picking one.

Corrected a note that had the runtime password print guarded at debug>=2.
In this version it is debug>=1; the compile-time default IS 1, and
_WiFi.ino:137 then overwrites it from the stored config, where .DEBUG=0.
That single stored zero is all that keeps the password off the connect line.

Three more assets mirrored, linked only from the tutorial body and easy to
miss: the SparkX Arduino variant, the LMIC device sketch, and
esp32_lora_gateway-x01.zip -- which is Eagle .brd/.sch, not a sketch,
despite the name. 19 assets now verify against SHA256SUMS.


---

## 2026-08-21 — Probe the radio: pin map confirmed by the chip, RST resolved

<!-- was 7276afa -->

firmware/radio-probe/ is a PlatformIO sketch that reads the SX1276's
RegVersion (0x42) over each candidate SPI wiring. RegVersion is the right
probe because it is a pure read of a constant: nothing about it depends on
the radio being configured. On the board:

  recorded (schematic)       SCK 14 / MISO 12 / MOSI 13  ->  0x12  FOUND
  previously assumed (VSPI)  SCK 18 / MISO 19 / MOSI 23  ->  0x00  no answer

0x12 is the SX1276's fixed version byte; 0x00 is a bus nobody is driving.
The pin map is no longer document-derived, and the wiring this project
originally recorded is shown producing exactly the silent failure the notes
warn about.

It also settles the RST contradiction. SparkFun publishes .rst = 5 in the
LMIC sketch and rst = 27 in the gateway tutorial, the latter commented
"Reset not used". The probe writes a recognisable value to RegOpMode, pulses
each pin low and reads back; a real reset restores the default. NEITHER pin
changed it. That throwaway comment is the only accurate published statement
about this pin -- both numbers are placeholders.

Supporting work:

- scripts/restore-firmware.sh, written BEFORE flashing anything. The board
  no longer runs the 2019 gateway, so the undo had to exist first. It writes
  back the bootloader, partition table and app0 dumped beforehand, and
  refuses a short app image rather than bricking the board with one. SPIFFS
  is deliberately excluded: an app upload never touches it, so the stored
  gateway config survives and a restore brings back a configured gateway.

- scripts/fetch-libraries.sh, for the public repo. Ten libraries ship inside
  the gateway repo and are pinned to 88113e8 -- the same commit the board's
  own firmware was built from -- so you compile against what is on the board
  rather than something newer. ArduinoJson must be 5.13.1 because the 6.x
  API is incompatible; the bundled copy already is, and the script checks
  the VERSION rather than the directory's existence, since accepting a wrong
  bundled version silently is the exact failure the pinning exists to stop.
  WiFiManager is skipped by default -- its include is guarded and this build
  has WIFIMANAGER 0. lib/ is gitignored; the repo ships the refs, not the code.

One incidental correction: PlatformIO's upload used esptool's stub at
115200 and worked fine, reaching 162 kbit/s. The recorded stub failure is
specific to 921600 and 460800, not to the stub as such.


---

## 2026-08-21 — Add src/: SX1276 driver and a link test, one project at the root

<!-- was 8b6daf1 -->

Consolidated firmware/radio-probe into a single PlatformIO project with
three environments -- probe, rx, tx -- sharing one pin definition.

src/board.h is the only place the wiring is written down. That matters
more here than on most boards: every stock ESP32 + SX127x target assumes
VSPI with NSS 18, this board is SCK 14 / MISO 12 / MOSI 13 / NSS 16, and
the mismatch fails silently. It also records that there is no usable reset
pin, since the probe showed neither published candidate resets the radio.

src/sx1276.{h,cpp} is a small polling driver rather than RadioLib or LMIC.
Both are better libraries, and both want a board definition -- supplying
one for hardware that matches no stock target is precisely where this
board's predecessors went wrong. A few hundred lines with the pins passed
in explicitly cannot silently pick the wrong bus.

Things the driver gets right that are easy to get wrong:

- Frf is computed in 64-bit. 915e6 << 19 overflows 32 bits comfortably.
- Low-data-rate optimisation is switched on when a symbol reaches 16 ms,
  which happens at SF11/SF12 on 125 kHz -- exactly the settings picked for
  maximum range, so omitting it costs packets where they matter most.
- RSSI is corrected by SNR when SNR is negative. Below the noise floor the
  packet RSSI reading alone understates the signal, and a range test lives
  in that regime.
- PA_BOOST is forced, because the RFM95HW does not wire RFO.
- CRC-failed packets are counted and dropped, never returned. A corrupt
  payload reported as data is worse than a lost one.

src/link is the actual test: numbered beacons one way, and a receiver that
reports RSSI, SNR and loss. Sequence numbers in the payload are what let
the receiver distinguish loss from silence, and loss is the number a range
test is for.

Verified on the board. The receiver finds the radio, configures 903.9 MHz
SF7/BW125/CR4-5, and idles at a -118 dBm noise floor.

One bug found and fixed by running it: the first noise-floor reading was
taken immediately after entering receive, before the front end settled, so
RegRssiValue returned 0 and the offset turned that into a confident -157
dBm. A plausible-looking number that actually meant "no reading yet". Now
delayed, and the receiver prints a periodic noise-floor heartbeat -- a
silent receiver and a dead one look identical otherwise, which is useless
when the other radio is a mile away.

tx is deliberately not the default and warns loudly: transmitting into an
open U.FL connector can damage the PA.


---

## 2026-08-21 — Add a scanner, and record the antenna

<!-- was cb15033 -->

src/scan sweeps known LoRa configurations rather than listening on one.
That is not a convenience: a LoRa receiver demodulates only what matches
its frequency, bandwidth, spreading factor AND sync word, so the link-test
receiver on sync 0x12 is structurally deaf to LoRaWAN (0x34) and Meshtastic
(0x2b). "Received nothing" from a single-configuration receiver says almost
nothing about what is in the air.

Eight curated presets rather than a cross-product: six SFs by three
bandwidths by three sync words is 54 dwells, minutes per pass, nearly all
of it on combinations nothing uses. A hit reports the exact settings that
caught it, which is also what you need to talk back.

Needed SX1276::reconfigure() -- retune an initialised radio without
re-checking the version byte.

Verified on the board. It retunes across all eight presets and reports a
noise floor for each. Those floors are real measurements, not register
artefacts: 500 kHz reads -111 dBm against -118 for 125 kHz, and
10*log10(500/125) is 6 dB.

CRC errors are surfaced per sweep because they are a positive signal --
something with matching modulation and sync word IS transmitting and
arriving corrupted.

docs/antenna.md records the Pycom kit from the invoice: RP-SMA male whip,
~200 mm, on a 100 mm U.FL/MHF1 pigtail, 868/915 MHz, discontinued. Its gain
is NOT published -- Pycom never released a datasheet and distributors leave
the field blank, so no number goes in a link budget. Comparable half-waves
are 2-3 dBi, recorded as inference, not specification.

For the bench: keep the pigtail and swap the 200 mm whip for a 915 MHz
RP-SMA stubby, or solder SparkFun's 78 mm quarter-wave to the ANT pad --
checking the "Close if using U.FL connector" jumper first, since the board
selects between the two paths. Noted that RP-SMA is not SMA, and that the
usual bench problem is too much signal rather than too little, which one
constant in board.h fixes.

The whip does unscrew, but transmitting on the bare pigtail can damage the
PA on an RFM95HW.


---

## 2026-08-21 — Keep SparkFun's LoRaWAN-device sketch for reference, with its two traps

<!-- was 3a953cd -->

Pulled in from the local Arduino workspace on request. It is SparkFun's
ESP-1CH-TTN-Device-ABP, which turns this board into a LoRaWAN device
instead of a gateway. Beerware licence, so unlike everything in
etc/firmware/ it can be committed.

It was never edited. Byte-identical to the tutorial download already
mirrored here, and identical to both hardware repos' copies once line
endings are normalised -- the only byte-level difference anywhere is one
stray CRLF, all four sharing normalised hash 29cf0c3926591010. My first
comparison called the repo copies "different", which was a byte comparison
and misleading.

More to the point it would not compile: the session keys are the literal
placeholders PASTE_NWKSKEY_KEY_HERE and so on, which are not valid C. So it
was downloaded and opened, never built and never flashed. That fits the
board running the gateway firmware. It also means the file holds no secrets,
which is what makes it safe for a public repo.

Two traps recorded, because both are silent:

- The US branch enables channel 17 = 905.7 MHz, while the line enabling
  channel 8 = 903.9 MHz -- what this board's gateway firmware listens on --
  sits commented out directly above it. US915 uplinks are 902.3 + 0.2n MHz.
  As shipped the device would transmit 1.8 MHz away from its own gateway.

- The region is chosen in the LMIC library's lmic_project_config.h, not in
  the sketch. Left at the default the EU branch compiles and the device
  lands on 868.1 MHz.

Its pin map is right on NSS 16 and DIO 26/33/32 and wrong on .rst = 5,
which the probe has already shown does not reset the radio. It names no SPI
pins at all -- LMIC uses the default SPI object, so the bus comes from the
sparkx_esp32_lora variant. Built against a stock esp32dev definition it
compiles, runs, and never reaches the radio.

Board re-verified after being moved: same MAC 84:0d:8e:0c:5a:80,
ESP32-D0WDQ6 rev v1.0, still on /dev/ttyUSB1.


---

## 2026-08-21 — Restructure for many applications; add WiFi config, HTTP, and a CLI

<!-- was fdf76f9 -->

Laid out for the mix that is coming -- a few real applications and a lot of
small experiments -- with one entry point for the terminal, CLion and any
automation.

  lib/     our shared libraries, committed: board, sx1276, loramath, netconfig
  src/apps small set of things meant to keep running
  src/tools single-purpose utilities you read the output of once
  test/    host-side unit tests
  vendor/  third-party libraries, gitignored

Reclaimed lib/ for our own code. Fetched libraries now land in vendor/,
because PlatformIO treats lib/ as the project's private libraries and the
previous arrangement forced a choice between committing other people's code
to a public repo and gitignoring the directory our shared code lives in.

Board settings moved from [env] to an [esp32] section that on-board
environments extend. [env] applies to EVERY environment including the host
test one, and the native platform rejects board=esp32dev outright -- which
is exactly how the first attempt at `make test` failed.

New: src/apps/gateway. WiFi with an HTTP status page and LoRa receive, so
"is it on my network" and "what is it hearing" are both answerable from a
browser or the serial console.

Credentials are NOT compiled in. lib/netconfig stores them in NVS, typed
over serial with ssid/pass/save. That matters twice over: this repo is
public, and NVS is its own partition, so an application upload never erases
them -- set once, reflash freely. Passwords are never echoed back, because
the failure being avoided is the stock firmware's habit of printing its
credentials to a serial log on every boot.

lib/loramath holds the arithmetic with no Arduino dependency so it can be
tested on the host: bandwidth indices, the 64-bit Frf computation, the
low-data-rate threshold, and the US915 channel plan. `make test` runs in a
third of a second with no board attached.

Writing those tests immediately caught a bad constant -- mine, in the test,
not in the code: I hand-computed Frf for 903.9 MHz as 14809190 when it is
14809497. Which is the point of testing arithmetic nobody can eyeball.

Makefile is the front door: build, flash, monitor, run, test, check, libs,
board, restore, ide. Two bugs fixed while proving it works -- `make check`
reported "shellcheck not installed" whenever shellcheck merely had findings,
conflating a missing tool with a failing check; and fetch-libraries.sh had a
`local x="$(...)"` that swallowed the command substitution's exit status.
shellcheck now passes clean.

Verified end to end: five environments build, four tests pass, the gateway
flashes, finds the SX1276 on 903.900 MHz, reports no stored credentials and
offers the command list.


---

## 2026-08-21 — gateway: fix the boot panic, and keep the console alive while connecting

<!-- was a56132d -->

Two bugs, both mine, both only visible by running it.

1. Panic loop. setup() called server.begin() unconditionally, but with no
credentials stored connectWifi() returns BEFORE WiFi.begin(), so lwIP was
never initialised. Creating a listening socket then trips

  assert failed: tcpip_send_msg_wait_sem tcpip.c:455 (Invalid mbox)

which panics and reboots, and does the same thing on the next boot -- so it
never reached a usable state. The HTTP server now starts only after a
successful connection, and only once; the listening socket survives WiFi
dropping and returning. Measured before and after: one boot every ~2 s with
a backtrace each time, versus one boot in 20 s with none.

2. A console that went deaf exactly when needed. connectWifi() blocks for up
to 20 s, and nothing serviced serial during it, so a mistyped SSID could not
be corrected until the attempt timed out -- and the retry I had just added
made that most of the time. The read loop is now pumped inside the connect
wait, and a fresh save/connect abandons the attempt in flight rather than
queueing behind it. Retry interval raised 30 s -> 60 s, since each attempt
blocks for twenty and a board that spends its time connecting also misses
LoRa packets. Measured: the console now answers in 0.3 s mid-connect.

Verified the credential round-trip end to end with a dummy network: staged,
saved to NVS, read back, connect attempt correctly diagnosed as "no such
network in range" (status=1), then erased. Passwords are reported as
"(set, N chars)" and never echoed. Left with NVS empty.

Worth recording: the commands I thought were lost during the first blocked
test were not lost, they were buffered in the UART and ran when the connect
finished. That is why the erase took effect. Serial input queues; it does
not vanish.


---

## 2026-08-21 — Brief on ChirpStack, and a checklist to send another radio's owner

<!-- was c43c12a -->

docs/chirpstack.md. The connection itself is trivial and verified from the
source: loraModem.h:500-505 defines PROTOCOL_VERSION 0x01 with
PKT_PUSH_DATA/PULL_DATA/PULL_RESP/PULL_ACK, which is the Semtech UDP packet
forwarder protocol -- exactly what chirpstack-gateway-bridge listens for on
UDP 1700. Pointing this board at ChirpStack is changing _TTNSERVER and
rebuilding. No translation layer.

The useful half of the document is where single-channel stops fitting,
because LoRaWAN assumes a gateway hearing eight channels across several
spreading factors and this board hears one of each:

- OTAA joins need a downlink, so use ABP -- which is why SparkFun's own
  device sketch does
- a normal device hops its whole sub-band, so ~7 of 8 uplinks land where we
  are not listening; the device must be pinned, and no server setting fixes
  that
- ADR must be off, or the first data-rate change makes the device vanish
- RX2 is 923.3 MHz SF12 and the downlink band is 923.3-927.5, nowhere near
  the 903.9 uplink; RX1 is the only window a parked gateway serves

Uplink-only ABP with ADR off works. Bidirectional OTAA is more work than the
board is worth, and the honest recommendation is recorded: for a handful of
sensors, raw LoRa plus our own forwarding is far less machinery than five
containers and a database, and nothing about it breaks because the gateway
hears one channel.

docs/radio-settings-request.md is the forwardable version -- five fields and
where to read them in RadioLib, arduino-LoRa, Meshtastic, LMIC or raw
register code. It leads with why all five are needed: a mismatch in any one
produces silence with no error at either end, which is the failure this
project keeps meeting. It also says plainly that "915 MHz" is a band and not
a frequency, and that sync word is the field people omit.


---

## 2026-08-21 — Default to 915.0 MHz; record the band plan and fix a channel-number error

<!-- was 01bbe90 -->

board.h now defaults to 915000000. We are doing raw point-to-point LoRa, not
LoRaWAN, so there is no plan to satisfy -- the frequency only has to match
the other radios, which are already on 915.0. One constant here beats
reconfiguring several devices.

Recorded in board.h and docs/band-plan.md that 915.0 is deliberately NOT a
LoRaWAN channel: (915.0 - 902.3)/0.2 = 63.5, so it sits between channels 63
and 64. It is a raw-LoRa convention, nothing more. If this board speaks
LoRaWAN again the right value is 903900000.

Fixed a factual error this project has been carrying in three places. The
notes called 903.9 MHz "US915 uplink channel 0". It is channel 8 -- channel
0 is 902.3 MHz. What makes 903.9 significant is being the FIRST channel of
sub-band 2, which is what US networks use, and which is why single-channel
gateways conventionally sit there. Corrected in CLAUDE.md, docs/hardware.md
and board.h, and the scan tool's presets were relabelled ch0 -> ch8 and its
500 kHz entry named as ch65.

docs/band-plan.md records the whole plan: 125 kHz uplinks 0-63 at
902.3+0.2n, 500 kHz uplinks 64-71, and downlinks at 923.3-927.5 -- a
different part of the band, which is why a parked single-channel gateway
cannot serve RX2. Plus the four parameters that must match (frequency, SF,
bandwidth, sync word) and the one that need not (coding rate, carried in the
explicit header). Plus an FCC 15.247 note: the higher power allowances in
902-928 assume hopping or a >=500 kHz signal, and a fixed 125 kHz link at
17 dBm is neither -- irrelevant on a bench, worth checking before a mast.

Two new tests lock this down. One asserts channel 8 is 903.9 and channel 0
is 902.3, so the error cannot come back. The other walks all 64 channels
asserting none of them is 915.0, so nobody later "corrects" board.h to a
channel number. Five tests pass, ten environments build.

The scan tool's own link-test preset moved to 915.0 to stay in step with
board.h; its LoRaWAN presets stay on 903.9, which is where LoRaWAN is.


---

## 2026-08-21 — Record the GPIO12 trap: the radio's MISO sets the flash voltage

<!-- was b0957a5 -->

Uploads started failing with 'Failed to communicate with the flash chip'
and 'Serial data stream stopped', while chip-id kept working. That
combination is the signature of a flash-rail problem, not a cable one.

Three facts already in these notes are one problem. The radio's MISO is on
GPIO12; GPIO12 is MTDI, the ESP32's VDD_SDIO strapping pin, high at reset
meaning 1.8 V; and the radio has no reset pin. espefuse confirms
XPD_SDIO_FORCE is not burned, so 'Flash voltage (VDD_SDIO) determined by
GPIO12 on reset'. The SX1276's MISO line therefore decides the ESP32's
flash voltage every time it boots, and a 3.3 V flash driven at 1.8 V fails
exactly this way.

Because the radio has no reset line, a warm reset does not clear its state.
Only removing power does -- unplug the USB and plug it back in.

Noted that burning XPD_SDIO_FORCE + XPD_SDIO_TIEH would fix it permanently,
and that eFuses are one-way and nobody has authorised it.


---

## 2026-08-21 — board.h: put the GPIO12 warning with the pin it concerns

<!-- was 5430313 -->



---

## 2026-08-21 — Add src/apps/listen, echo typed input in the monitor, note the boot symptom

<!-- was c09462d -->

src/apps/listen is a listener and nothing else: no WiFi, no HTTP, no
framing, no loss accounting. It prints frequency, SF, bandwidth and sync
word at startup -- all four must match at the far end or nothing arrives --
then every packet as hex AND text, since you rarely know which is readable
until you look. Idle heartbeat with the noise floor every 30 s, because a
silent receiver and a dead one are otherwise indistinguishable. It is now
the Makefile's default APP.

monitor_echo = yes. `pio device monitor` does not echo local input, so
typing into the gateway's serial console produced a blank line and no
feedback -- indistinguishable from a dead port. The board does not echo
either, deliberately: echoing a password back is what this project exists
to stop doing. Local echo is the right layer for it.

Recorded a second face of the GPIO12 trap. The same strap sampled high at
reset leaves the ROM bootloader unable to read flash at all:

  rst:0x10 (RTCWDT_RTC_RESET),boot:0x33 (SPI_FAST_FLASH_BOOT)
  flash read err, 1000

That appears AFTER a successful hash-verified upload, so the instinct is to
blame the firmware just written. It is not the firmware. Also noted why it
is intermittent and unfixable in software: MISO is high-impedance when NSS
is deasserted, so GPIO12 floats at reset rather than being driven, and code
runs long after the strap is sampled.


---

## 2026-08-21 — Stop reporting noise as packets: check the received header's CRC flag

<!-- was bf42b84 -->

A 150 s sample "received" two packets, 159 and 61 bytes. Both were noise,
and I said something was on the air before checking. Three independent
tells, any one sufficient:

- SNR -14.2 and -13.8 dB at SF7, whose demodulation floor is about -7.5 dB.
  Six dB below the limit is not a weak signal, it is not a signal.
- Payloads uniformly random: no header, no repeated field, no text.
- Lengths arbitrary, because the header was itself noise.

The CRC did not catch it because there was no CRC. In explicit-header mode
the CRC-present flag comes from the RECEIVED header -- the transmitter's
choice -- while RxPayloadCrcOn only governs what we send. Noise that decodes
into a header with CRC off can never raise PayloadCrcError, so a corrupt
packet looks perfect. That is a real hole in the driver, not a quirk of this
sample: any CRC-less packet was being handed back as verified data.

poll() now reads RegHopChannel bit 6 (CrcOnPayload) and sets
Packet::crcPresent. Such packets are counted separately via
uncheckedPackets() and labelled "*** NO CRC -- UNVERIFIED ***" rather than
dropped -- a transmitter that genuinely omits CRC is legitimate, but so is
noise, and only a person can tell them apart. The listener also flags the
SNR-below-floor case explicitly.

Verified: 100 s after the fix, 0 received, 0 CRC errors, 0 unverified,
where the same conditions previously produced phantom packets.

Recorded the demodulation floors in CLAUDE.md -- SF7 about -7.5 dB, roughly
2.5 dB per SF up to about -20 dB at SF12 -- since an SNR below the floor is
the quickest way to recognise this class of mistake.


---

## 2026-08-21 — Document measuring SNR on a bench, and round the RSSI correction properly

<!-- was d368b66 -->

The transmitting side cannot measure SNR -- raw LoRa has no round trip, so
every signal-quality figure comes from the receiving radio's registers. What
that side can usefully do is report its transmit power and turn it down.

docs/measuring-snr.md puts numbers on the close-range problem. Free-space
loss at 915 MHz is 31.7 dB at one metre, so 17 dBm with 2 dBi antennas puts
about -10.7 dBm into a receiver whose SF7 sensitivity is around -123 dBm.
That is ~112 dB of excess: two radios a metre apart with full antennas are
not a weak-signal experiment, they are an overload experiment.

Consequences recorded because both mislead: the SNR estimate saturates near
the top of its range, so a column of +9/+10 dB readings is a number that has
stopped carrying information rather than a good link; and a saturated front
end produces CRC failures that look like a bad link but are caused by too
much signal, so errors rising as radios approach means overload.

Levers to shed the necessary ~80 dB, with values: transmit power 17->2 dBm
is 15 dB and free, a coaxial attenuator 20-40 dB, ten metres instead of one
is 20 dB. Warned against the obvious shortcut -- unscrewing the antenna
reflects power into the PA, and this is the +20 dBm RFM95HW; use a dummy
load or attenuator instead.

Demodulator floors quoted from datasheet Table 13 rather than memory: SF6
-5, SF7 -7.5, SF8 -10, SF9 -12.5, SF10 -15, SF11 -17.5, SF12 -20 dB. This
confirms the -7.5 dB figure used to identify the phantom packets.

Reading the datasheet also turned up a real defect. Section 5.5.5 gives
Packet Strength = -157 + PacketRssi + PacketSnr * 0.25 for negative SNR, and
poll() was truncating that correction term to an integer -- losing up to a
dB precisely in the below-the-noise-floor regime the correction exists for.
Now computed in float and rounded.


---

## 2026-08-21 — Add lab mode: -48 dB front-end attenuation, toggled at runtime

<!-- was 8143b7b -->

Turning our own transmit power down does nothing while we are the listener
-- the overload comes from their transmitter. The lever that helps on this
side is the receiver's own attenuator: RegLna bits 7-5 offer up to -48 dB,
which is a coaxial attenuator we already own and were not using.

The catch, and the reason a naive attempt would appear to do nothing: with
AgcAutoOn set the radio chooses the LNA gain itself and silently ignores
RegLna. setLnaGain() therefore clears AGC as part of its contract, and
returning to normal restores it.

Toggled at runtime from the listener -- 'l' lab, 'n' normal, '?' status --
rather than by a build flag, because on this board every reflash risks a
GPIO12 boot-loop needing a physical power cycle. Writing a register beats
rebuilding. The idle heartbeat reports the active mode so a capture is never
ambiguous about which gain produced it.

Lab mode also drops transmit power to 2 dBm for when this board does
transmit, and tx-lab builds the beacon the same way, since transmit power is
fixed at build time there.

board.h carries the two constants with the reasoning: the levers are
independent because they solve different halves of the problem, and this
board is normally the receiving half.

docs/measuring-snr.md gains the key map and the warning that matters --
lab mode will miss weak signals, which is the point, so press 'n' before
concluding the band is quiet.

14 builds pass. Not yet verified on hardware: the upload hash-verified but
the board came up in the GPIO12 flash-read loop again and needs a power
cycle before lab mode can be exercised.


---

## 2026-08-21 — Verify lab mode on hardware: 43 dB of desensitisation, measured

<!-- was 7316d67 -->

Toggled three times on the board:

  normal        -120 dBm
  lab            -77 dBm
  normal again  -121 dBm

43 dB against a nominal -48 dB for the G1->G6 gain step plus the LNA boost
switching off. Close agreement and repeatable, so the mechanism is doing
what the register description says.

Recorded the part that will otherwise be misread: the reported floor goes UP
in lab mode and that is correct. With the LNA turned down, mixer and ADC
noise dominate instead of amplified antenna noise, so the floor referred to
the input rises by nearly the attenuation -- exactly what a coaxial
attenuator does to a receiver. Reading -77 dBm as "noisier, something broke"
is the natural mistake; the receiver is simply 43 dB less sensitive, which
is the entire point.

Two consequences noted in both places: figures taken in lab mode are not
comparable with normal-mode figures, and sensitivity drops from about
-123 dBm to about -80 dBm, so anything across the building disappears. The
idle heartbeat prints the active mode so no capture is ambiguous about which
gain produced it.

Not reflashed for this: the app's wording could call the lab-mode reading
something other than "noise floor", but on this board a reflash risks a
GPIO12 boot-loop needing a physical power cycle, and that is not worth
spending on a string. Documented instead.


---

## 2026-08-21 — Measure the antenna change: the noise floor does not move

<!-- was 80bc4e6 -->

Whip attached vs pigtail only, same board and firmware, minutes apart:

  normal   -121 dBm  ->  -121 dBm
  lab       -78 dBm  ->   -78 dBm

No change in either mode, which is the expected result and worth recording
because "remove the antenna to reduce noise" is a natural assumption that is
wrong about which quantity moves. The idle floor here is set by the
receiver's own mixer and ADC noise, not by noise arriving through the
antenna, and internal noise does not care what is on the connector.

What removing the whip does change is received SIGNAL, by 20-40 dB -- useful
against a transmitter a metre away, but it appears in the RSSI and SNR of an
actual packet and never in the idle floor. So the floor is the wrong
instrument for checking whether an antenna change worked.

Also recorded, from this session's own data: the floor moved 15 dB on its
own. Two 150 s samples earlier read -106/-107 dBm repeatedly; later readings
with identical configuration and the same antenna read -121/-122. Nothing in
the setup changed, so it is environmental -- the band is shared and what is
transmitting nearby varies by the hour.

That has a direct consequence for the measurement procedure: an SNR figure
without the noise floor it was taken against cannot be compared across
sessions. A signal at -100 dBm is 6 dB above one of those floors and 21 dB
above the other.


---

## 2026-08-21 — Move the monitor menu key off Ctrl+T, which terminals swallow

<!-- was d7e1709 -->

pio device monitor advertises "Menu: Ctrl+T" but that key never arrives:
terminal emulators, tmux and IDE terminals commonly bind it, so the banner
promises something that does nothing. The option is settable from
platformio.ini -- verified with `pio project config` -- so
monitor_menu_char = 29 puts it on Ctrl+], which is rarely bound.

Deliberately not 28: that is SIGQUIT.

Documented the menu keys worth having now that it works: Ctrl+] Ctrl+H for
help, and Ctrl+] Ctrl+R to toggle RTS -- which on this board is EN, so it
resets the device. Quit stays Ctrl+C.

Also fixed a plain error in docs/project-layout.md, which claimed Ctrl+] was
the quit key. It is the menu key; Ctrl+C quits. And brought two stale lists
in that file up to date -- the source tree was missing apps/listen, and the
APPS example predated listen and tx-lab.

Worth noting most of the time the menu is unnecessary: listen takes
single-key commands and gateway takes typed lines.


---

## 2026-08-21 — Pass --menu-char on the CLI; there is no such project option

<!-- was 26247e7 -->

My previous fix did nothing. I put monitor_menu_char in platformio.ini,
tested it with `pio project config`, saw it echoed back and concluded it
worked. That only proved the INI parser does not reject unknown keys.

PlatformIO's actual monitor project options are monitor_baud, dir, dtr,
echo, encoding, eol, filters, parity, port, raw, rts and speed. No
menu_char. The parser accepts anything and silently ignores what it does not
know, so an invented option looks accepted and has no effect -- which is
worse than an error, and is exactly how I fooled myself.

The menu key is CLI-only. The Makefile now passes --menu-char, with
MENU_CHAR ?= 29 so it can be overridden: make monitor MENU_CHAR=30.

Verified in PlatformIO's source rather than by another test that proves the
wrong thing: device/monitor/command.py declares --menu-char with default 20,
and device/monitor/terminal.py sets term.menu_character = chr(menu_char) and
prints the banner from key_description(terminal.menu_character). So the flag
determines both the binding and the advertised key.

Removed the inert option from platformio.ini rather than leaving a line that
reads as configuration but is decoration, and left a comment saying why.
Documented the general trap: if a monitor_* setting appears ignored, check it
against the supported list before assuming a bug.


---

## 2026-08-21 — Add src/tools/sweep: find which modulation parameter disagrees

<!-- was 6a20d8a -->

scan sweeps named presets -- LoRaWAN, Meshtastic, our link test. This is the
other question: the frequency is agreed, both ends are running, and nothing
arrives, so one of the remaining parameters differs and the fastest way to
find out which is to try them all.

Grid at 915.0: SF7-12 x BW 125/250 kHz x sync 0x12/0x34. 24 combinations at
5 s each, two minutes per pass. Coding rate is deliberately not swept -- it
travels in the explicit header, so the receiver adapts to whatever arrives.

Reports CRC errors per combination as well as packets, because a CRC error
is nearly as informative: the modulation and sync word matched well enough
to demodulate and only the payload was corrupt, which means those settings
are right and either the signal is bad or the front end is overloaded.

Warns up front that the other end must transmit continuously while it runs.
Each combination is listened to for five seconds, so a beacon every thirty
would be missed by nearly every dwell -- a sweep that finds nothing under
those conditions proves nothing.


---

## 2026-08-22 — Link works: decode the Pro RF frame, step the attenuator, autodetect the port

<!-- was f409b43 -->

The link is up. The Pro RF's RFO bug was the whole cause -- nothing on this
side changed. Receiving seq 340 onward at 2 s intervals, no loss.

Decode their frame, per their docs/radio-settings-reply.md and confirmed
byte-for-byte against capture: 4 bytes RadioHead header, then magic 0x4A,
version, type, nodeId, seq as uint32 LE, then the NUL-terminated ASCII
record from byte 12. The listener prints seq/node/type and the sentence, and
counts sequence gaps for a real loss rate. Unrecognised frames still fall
back to the hex+text dump.

Verify PA_BOOST by reading RegPaConfig back rather than trusting the write,
which is what their reply recommends and what cost them a day: RadioHead's
setTxPower(power, useRFO=true) routed transmit to a pin the RFM95 does not
connect, and nothing reported an error -- send() returned true and the
console said "sent". Ours was already correct; now it is checked and printed
at startup.

Attenuation is stepped 1..5 (-6/-12/-24/-36/-48 dB) instead of on/off. -48
alone put SNR at -6.8 dB, which is 0.7 dB above SF7's -7.5 floor -- the edge
of the range rather than the middle of it. Measured against the live link:

  atten    0 dB   SNR +9.0     -24 dB   SNR +7.2
         -12 dB   SNR +8.8     -36 dB   SNR +2.8
                               -48 dB   SNR -1.8

Note 12 dB of attenuation moved SNR by 0.2 dB. That is the saturation
ceiling made visible: below about +8 the estimator responds, above it does
not, so a "good" SNR near +9 carries no information at all.

Port is autodetected by vendor/model via scripts/find-port.sh, not
hardcoded. /dev/ttyUSBn is assigned in enumeration order and this board gets
power-cycled often, so it moved ttyUSB1 -> ttyUSB2 -> ttyUSB1 tonight. The
shell scripts kept working throughout because they already detected; only
the Makefile and platformio.ini broke, and both are fixed.


---

## 2026-08-22 — Write up the attenuation measurements for the Pro RF side

<!-- was b28976c -->

docs/attenuation-report.md. What we added on the receive side, the numbers,
and the two things they need to know to read them.

The measurements, both ends about 1 m apart at their low-power setting:

  atten    0 dB   RSSI -91  SNR +9.0      -36 dB   RSSI -85  SNR +2.8
         -12 dB   RSSI -93  SNR +8.8      -48 dB   RSSI -79  SNR -1.8
         -24 dB   RSSI -90  SNR +7.2

Zero loss at every step. The headline is the first two rows: 12 dB of
attenuation moved SNR by 0.2 dB, which is the estimator pinned at its
ceiling. An SNR near +9 is not a measurement, and any comparison taken there
shows no difference regardless of what changed.

Also flagged that reported RSSI RISES with attenuation because the
computation does not compensate for manual LNA gain, so SNR is the number to
compare across steps and RSSI only means anything at 0 dB.

Raised the discrepancy rather than leaving it: they predict -30 to -50 dBm
at a metre, we measure -91 at full sensitivity. 40-60 dB unexplained, not
urgent at zero loss, but worth finding before it matters at range. Asked for
their actual dBm and what antenna is on each end -- if the gateway is on a
bare U.FL socket that accounts for most of it.

Asked them not to reduce power yet: we are 48 dB down and 5.7 dB above SF7's
floor, so simultaneous changes would put us through it with no way to
attribute the loss.


---

## 2026-08-23 — Widen the permission allowlist for read-only commands

<!-- was 2064fdd -->

Scanned the 50 most recent session transcripts across all projects and added
the read-only patterns that actually recur and were not already covered:
journalctl, dmesg, systemctl is-active/status, ip neigh/route, gh project
list, gh label list, adb devices, plus three read-only MCP tools.

Deliberately excluded, despite high counts: python3/bash/ssh/adb (arbitrary
execution), curl (can POST), gh api (mutation-capable), docker exec,
pio run (uploads firmware), esptool (writes flash), git push/commit/add.
Also skipped everything Claude Code already auto-allows -- grep, head, tail,
ls, git status/log, gh issue/pr view and list -- which needs no entry.

29 entries -> 41.


---

## 2026-08-24 — Docs: correct four stale claims and record this session's findings

<!-- was 119d46e -->

Prep for compaction. The corrections matter more than the additions: a stale
claim is worse than a missing one, because the next session acts on it.

Corrected, all four now false:

- README "Current state" said the board runs the stock 2019 ESP-1ch-Gateway
  sketch on 903.9 MHz. It has not since 2026-08-23. It runs src/apps/listen
  on 915.0 MHz. The same sentence also still said "US915 uplink channel 0",
  an error corrected elsewhere weeks ago but which survived here -- 903.9 is
  channel 8.
- CLAUDE.md "Nothing on the board needs preserving" asserted in the present
  tense that the firmware IS the stock sketch. Rewritten to past tense, with
  the three backed-up regions listed and the point that SPIFFS is
  deliberately excluded because an app upload never touches it -- which is
  why a restore returns a CONFIGURED gateway rather than a blank one.
- docs/handoff.md opened with "one board, no firmware of its own" and
  "PlatformIO is not set up here". Both were true when written and are now
  the opposite: there is a PlatformIO project with six environments, an
  SX1276 driver, a Makefile and host-side tests.
- firmware/radio-probe/ was cited in CLAUDE.md, docs/hardware.md and
  docs/programming.md. That path stopped existing in the restructure; it is
  src/tools/probe.

Added, as findings rather than narrative:

- The port number moves. ttyUSB1 -> ttyUSB2 -> ttyUSB1 in one evening, same
  socket. When it moved the shell scripts kept working and `make flash`
  broke, because the scripts detect by vendor/model and platformio.ini had
  the path hardcoded. Recorded with the reason a by-id path is unavailable
  here: the CH340C carries no serial number, unlike the sibling board's
  CP2102N.
- The link works, and why it did not. Nothing on this side was ever wrong --
  the Pro RF was transmitting into RFO, a pin the RFM95W does not connect,
  via RadioHead's setTxPower(power, useRFO=true) as printed in SparkFun's own
  hookup guide. Nothing reported an error. Since that idiom is everywhere in
  SparkFun and Adafruit examples, SX1276::begin() now reads RegPaConfig back
  and exposes paBoostConfirmed(). Their frame layout is recorded too.

CLAUDE.md is 23.7 KB, under the 30 KB trim threshold.

The board is currently detached, so README "Current state" was written from
the last verified serial read rather than re-read from the device.


---

## 2026-08-24 — Docs: keep the AP shortcut usable after redaction, and record the scrub

<!-- was 2a22232 -->

Rewriting history removed the wpa[] credentials from every commit, which
broke one instruction that depended on them: docs/programming.md offered
standing up an access point matching the compiled-in pair as a way to reach
the web UI without reflashing. With the values gone that could not be
followed, so it now says where to recover them locally -- the gitignored
mask-secrets.txt, or read them off the board.

CLAUDE.md records that the scrub happened and why, so nobody restores the
literals to a tracked file without realising the repository is public.

The values themselves were never a secret worth much -- the owner's
assessment is that they are production-line test values -- but the SSID names
a real third-party makerspace, and "fine in a local repo" is a different
judgement from "published on GitHub". Nothing had ever been pushed, so this
was the cheap moment to decide.


---

## 2026-08-25 — Docs: survey what runs on this board besides C/C++

<!-- was 53ad762 -->

The question was whether Rust, MicroPython or a Lisp could run here. The
answer turns almost entirely on one fact that was recorded imprecisely: this
is an Xtensa LX6 core, not RISC-V. The newer C3/C6/H2 parts are RISC-V and are
what upstream LLVM and upstream Rust actually support, so a project claiming
"ESP32 support" in 2026 usually means those. The hardware table said
"not an ESP32-S3", which rules out the wrong thing -- it now names the
architecture, because that is the discriminator someone will need.

uLisp is the find worth writing down. MIT licensed, release 4.9a dated
2026-03-02, so it is current rather than abandoned. More useful than the
licence: it has a documented extension mechanism -- `#define extensions` plus
an Extensions.ino registering C++ functions in a second lookup table -- so
adding primitives costs a file you own rather than a fork of someone else's
several-thousand-line interpreter. With 79 commits and zero pull requests
upstream, a fork would be a hard fork with no way back, which makes that
mechanism the difference between viable and not.

Two traps recorded because both are the shape this project keeps hitting.
There are two uLisp repositories and the obvious name, ulisp-esp32, is the
dead one -- superseded by ulisp-esp, exactly like ESP-1ch-Gateway versus
ESP-1ch-Gateway-v5.0--OLD. And uLisp's with-spi defaults to VSPI 18/19/23,
so driving the radio from Lisp directly walks into the same silent failure
that platformio.ini warns about. Wrapping lib/sx1276 as an extension avoids
it, and is the better design anyway: it puts a REPL over a driver already
proven correct, which is what the register archaeology in this project has
repeatedly wanted and had to do by recompiling instead.

Nothing was run on hardware and no toolchain is installed; the board was
detached throughout, so README "Current state" is untouched.


---

## 2026-08-25 — Untrack SparkFun's mirrored tutorial page, and drop a local path

<!-- was c09136f -->

Two things that only matter because this repository is about to be public.

docs/sparkfun-hookup-guide.html was committed in ac695d0 and has sat in the
tree ever since, even though .gitignore names it explicitly. A gitignore rule
does not apply to a path that is already tracked, so the rule looked like it
was working and was not. That contradicts the policy docs/assets.md states in
its own words -- vendor documents "are not licensed for redistribution ...
What is committed is this manifest, the provenance, not the payload." The file
is SparkFun's copyrighted tutorial, and it was the only payload in the tree.

Nothing is lost by untracking it: docs/assets.md line 103 carries its source
URL and docs/SHA256SUMS line 17 carries its hash, which is the whole point of
shipping provenance. It stays on disk and is now genuinely ignored.

etc/reference/README.md recorded the absolute sketchbook path the file was
copied from. It disclosed a local directory layout and told a reader nothing
they could use, since the same section already establishes the file is
byte-identical to SparkFun's tutorial download.

Note the blob still exists in history; removing it there is a separate
decision, and cheap only while there is still no remote.


---

## 2026-08-26 — Trim the permission allowlist to what this project actually uses

<!-- was c8eb952 -->

The file came from the Heltec project along with the slash commands and was
never pruned. It allowlisted five scripts that do not exist here
(heltec-dev.sh, heltec-setup.sh, install-toolchain.sh), the meshtastic CLI,
adb, a Gmail MCP tool and heltec.org -- none of which this project has ever
called. Publishing that would have described someone's desktop rather than
this repository's needs.

Also dropped five gh read-only entries that Claude Code already permits
without a rule (gh issue list/view, gh pr list/view, gh label list), and
`gh api graphql -f query=*`, which existed to mutate the Heltec project board
and is far broader than anything here needs.

Kept the gh project read-only entries: a GitHub Project is planned for this
repository, and unlike the issue and pr subcommands those are not auto-allowed.

41 entries to 25.


---

## 2026-08-27 — Docs: state the licensing of the mirrored assets accurately

<!-- was 121c9d4 -->

This file said all of these documents "are not licensed for redistribution",
which is true of the datasheets and wrong about SparkFun's tutorials. Their
page footers state CC BY-SA 4.0, so mirroring them is explicitly permitted
with attribution and share-alike.

The blanket claim mattered because it was the stated reason for untracking
docs/sparkfun-hookup-guide.html. Keeping that file out is still right -- the
sibling saved page was already untracked, so committing one of the two was an
inconsistency -- but the justification was not a licence problem. It is that
share-alike content sits awkwardly in an MIT repository, the saved HTML carries
SparkFun's Google Tag Manager and social-network beacons, and this manifest
already lets anyone refetch and verify the page.

Split the paragraph into the three cases that actually apply: all-rights-
reserved datasheets, CC BY-SA tutorials, and source archives with their own
licences in the table's Licence column.


---

## 2026-08-27 — Restore the six ticket commands, adapted rather than copied

<!-- was 7c0dc38 -->

These were deleted in August because they were bound to the Heltec project's
GitHub repository and board -- /new-issue would have filed against the wrong
tracker. A repository and Project are now planned here, so they are back.

They are adapted, not copied. The identifiers are the least of it:

Gates. The Heltec versions run shellcheck and `pio run`. This project has
host-side unit tests, so the gate is `make check`, `make build` and `make test`.
/merge-pr treats `make test` as mandatory for anything touching lib/loramath or
register arithmetic, because that is the class of error this project actually
shipped -- a wrong US915 channel number survived in three documents until a
test pinned it.

Labels and milestones are this project's. Meshtastic, MQTT and Reticulum are
meaningless here; radio, firmware, tooling and interop are not. The milestone
axis is whether the SAMD21 Pro RF and its owner are needed, so `Solo bench` and
`Two-radio link` replace `Solo bring-up` and `Multi-site link`.

A power cycle is now a distinct blocked state. The GPIO12 strap means a flash
can fail for a reason no command can fix, and treating that as a ticket failure
would be wrong -- /fix-ready tells the model to ask the user to unplug the board
and resume, without moving the ticket.

Masking is called out wherever output could reach a public issue or PR. This
board printed WiFi credentials on every boot and leaked them into a session log
twice; a command that pastes a banner into a PR body would do it a third time.

Structural change: the project and field IDs live once, in
.claude/board-config.md, not repeated across all six files as they are upstream.
That file sits outside commands/ so it does not itself become a slash command.
Nothing is filled in yet, and each command stops rather than guessing if it
finds a placeholder.

CLAUDE.md said flatly that six commands were deleted and should be restored from
the Heltec tree. That is now false in both halves, so it describes what is here
and what changed instead.


---

## 2026-08-27 — Commit the component datasheets, with the fair-use position stated

<!-- was 7eb6de4 -->

This board is 2019 hardware, retired, and its vendor documentation is already
decaying: the CH340 datasheet needs a browser because WCH builds its download
list client-side, and every SparkFun URL for WRL-15006 now serves the WRL-18074
page with a different module's datasheet attached. A dead link plus a SHA-256 is
not the same as the document, and someone picking this hardware up in 2028 needs
these eight files, not a hash of them.

So the datasheets are now committed. They are all-rights-reserved vendor
documents redistributed without permission, and docs/datasheets/README.md says
so plainly rather than implying a licence that does not exist. It works through
the four fair-use factors honestly, including the one that cuts against --
these are complete copies, because a partial datasheet is useless -- and states
that fair use is a judgment asserted in good faith, not a permission granted.

It also carries an unconditional takedown offer. Every entry in assets.md keeps
its source URL, so removing a file costs a reader a link rather than the
information.

This is narrower than "mirror everything". Source archives stay out: those are
other people's code, they remain in their original repositories, and
fetch-libraries.sh retrieves them. The two mirrored SparkFun web pages stay out
as well -- they are CC BY-SA 4.0 and could be committed, but they carry Google
Tag Manager and social-network beacons and share-alike sits awkwardly in an MIT
repository. assets.md now states all four cases in a table instead of one
blanket sentence.

The .gitignore entry carries a note saying the omission of docs/datasheets/ is
deliberate, so it does not get "fixed" back later.

All eight verify against docs/SHA256SUMS.


---

