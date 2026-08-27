# Verify the gateway end to end

Confirm the host, the serial path and the board are all working, and report what firmware is
running. Run this before believing any negative result.

Requires the board attached over USB micro-B with a **data** cable.

`$ARGUMENTS` may name a port (e.g. `/dev/ttyUSB1`); otherwise find it below.

## Step 1 — Find the port

This board's bridge is a **CH340C**, `1a86:7523`, kernel driver `ch341`. That is a different chip
from the Heltec V3's CP2102N (`10c4:ea60`), so with both attached they are easy to tell apart:

```bash
for d in /dev/ttyUSB*; do
  printf '%s  %s\n' "$d" "$(udevadm info -q property -n "$d" | grep -E '^ID_(VENDOR_ID|MODEL_ID|USB_DRIVER)=' | tr '\n' ' ')"
done
```

`ID_VENDOR_ID=1a86 ID_MODEL_ID=7523 ID_USB_DRIVER=ch341` is this board.

**CH340s carry no serial number**, so with two of them attached udev cannot distinguish them.
Use `/dev/serial/by-path/` in that case.

## Step 2 — Identify the chip

```bash
esptool --port /dev/ttyUSB1 --no-stub chip-id
```

**`--no-stub` is not optional here.** The stub flasher fails over this CH340 at 921600 and at
460800 with `Unable to verify flash chip connection`. Basic commands work without it.

Expect:

```
Chip type: ESP32-D0WDQ6 (revision v1.0)
MAC:       84:0d:8e:0c:5a:80
Detected flash size: 4MB
```

A different MAC means a different board — say so rather than assuming. 16 MB rather than 4 MB
means this is the later revision WRL-18074, and `CLAUDE.md` needs correcting.

## Step 3 — Read the boot banner

```bash
./scripts/read-banner.sh
```

That finds the CH340, resets it, reads the banner, **masks the credentials** and reports which
firmware is running. Prefer it to a raw terminal. The manual equivalent, which does not mask:

```bash
python -m serial.tools.miniterm /dev/ttyUSB1 115200
```

Tap **RST**, read, then `Ctrl-]`.

No RST tap is needed if you drive the reset line yourself, which also makes this step scriptable:

```python
import serial, time
p = serial.Serial('/dev/ttyUSB1', 115200, timeout=0.5)
p.dtr = False          # IO0 high -> normal boot, not bootloader
p.rts = True           # EN low  -> hold in reset
time.sleep(0.15)
p.reset_input_buffer()
p.rts = False          # release; banner follows within ~2 s
buf = b''
end = time.time() + 12
while time.time() < end: buf += p.read(4096)
```

**It is RTS that resets, not DTR.** Toggling DTR alone returns zero bytes on this board.

**Stop and think before pasting the output anywhere.** The stock ESP-1ch-Gateway sketch prints
its stored WiFi SSID and password in the clear on every boot, as `.SSID=` and `.PASS=`. Run it
through `./scripts/export-transcript.sh` masking, or redact by hand.

**Redacting by hand means more than those two lines.** The sketch prints the SSID again further
down as `WiFi connect SSID=`, `WlanStatus:: CONNECTED ssid=` and four other shapes — see the
table in `CLAUDE.md`. If the stored debug level is raised to 2 the password joins them. A
hand-written `sed` for `^\.SSID=` alone will leak; that is exactly how it leaked on 2026-08-21.

The stock banner opens with `ESP32 defined, freq=903900000` and a `readConfig::` dump. Anything
else means the board has been reflashed since these notes were written.

## Step 4 — Report

Port, chip identity, flash size, and **which firmware is running**. Report only what was
observed. If the board was not attached, say the check did not run — never infer a result.
