#!/usr/bin/env bash
#
# Reset the gateway and print its boot banner, with credentials masked.
#
# The stock ESP-1ch-Gateway sketch dumps its stored configuration to serial on
# every boot, and that dump contains the WiFi SSID and password in the clear --
# in six different line shapes, not the two you would guess. See CLAUDE.md.
# So this masks by default and you have to ask for the clear text.
#
# No RST tap is needed: RTS drives the board's auto-reset circuit. Note that
# DTR does NOT -- toggling DTR alone returns zero bytes on this board.
#
# Resetting costs you a screenful of mojibake first: the ROM bootloader talks at
# 74880 baud and the sketch at 115200, and you cannot split the difference --
# the ch341 driver rejects 74880 outright. Use --no-reset to skip the ROM phase
# and get clean output. You rarely lose the banner by doing so, because this
# board reboots itself every 20 seconds or so: its stored WiFi network has not
# existed since 2019, and the sketch resets when the connection fails.

set -euo pipefail

# shellcheck source-path=SCRIPTDIR
# shellcheck source=lib/common.sh
source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/lib/common.sh"

PORT=''
BAUD=115200
SECONDS_TO_READ=12
MASK=1
RESET=1
OUT_PATH=''
SECRETS_FILE="${PROJECT_DIR}/.claude_artifacts/mask-secrets.txt"

usage() {
    cat <<USAGE
Usage: $(basename -- "$0") [options]

Reset the SparkFun LoRa Gateway 1-Channel (ESP32) and print its boot banner.
Secrets are masked unless you switch masking off deliberately.

  --port PATH      serial port (default: autodetect the CH340, 1a86:7523)
  --baud N         baud rate (default: ${BAUD})
  --seconds N      how long to read after reset (default: ${SECONDS_TO_READ})
  --out PATH       also write the (masked) capture to a file
  --no-mask        print credentials in the CLEAR -- think before using this
  --no-reset       do not reset the board; just listen to what it is already
                   saying. No boot banner, but no ROM preamble either.
  -h, --help       this message

A reset makes the ROM bootloader talk first, at 74880 baud, which at 115200
arrives as a screenful of mojibake before the real banner. You cannot read it
at 74880 instead: the ch341 driver REJECTS that rate outright -- 'stty -F PORT
74880' fails -- so on this bridge the preamble is unreadable at any setting.
--no-reset is the way to avoid it: no reset, no ROM phase, no noise. You usually
still get a banner anyway, because this board reboots itself every 20 seconds or
so when its stored WiFi network fails to appear.

The banner prints the stored WiFi SSID and password. With masking on they are
replaced by '***' and the capture is safe to paste. With --no-mask it is not.

Examples:
  ./scripts/read-banner.sh
  ./scripts/read-banner.sh --port /dev/ttyUSB1 --seconds 20
  ./scripts/read-banner.sh --out /tmp/banner.txt
USAGE
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --port)     PORT="${2:?--port needs a value}"; shift 2 ;;
        --baud)     BAUD="${2:?--baud needs a value}"; shift 2 ;;
        --seconds)  SECONDS_TO_READ="${2:?--seconds needs a value}"; shift 2 ;;
        --out)      OUT_PATH="${2:?--out needs a value}"; shift 2 ;;
        --no-mask)  MASK=0; shift ;;
        --no-reset) RESET=0; shift ;;
        -h|--help)  usage; exit 0 ;;
        *)          die "unknown option: $1 (try --help)" ;;
    esac
done

section 'port'

if [[ -z ${PORT} ]]; then
    PORT="$(find_ch340_port)"
    info "autodetected ${PORT} (CH340, 1a86:7523)"
else
    [[ -e ${PORT} ]] || die "no such port: ${PORT}"
    info "using ${PORT} (given)"
fi

[[ -r ${PORT} && -w ${PORT} ]] || die "cannot read/write ${PORT}. Are you in the 'dialout' group? (id -nG)"

lock_port "${PORT}"

if [[ ${MASK} -eq 1 ]]; then
    info 'masking: on'
else
    warn 'masking: OFF -- the WiFi password will be printed in the clear'
fi

if [[ ${RESET} -eq 1 ]]; then
    section 'reset and read'
    info "resetting via RTS, then reading for ${SECONDS_TO_READ}s at ${BAUD} baud"
else
    section 'read'
    info "listening for ${SECONDS_TO_READ}s at ${BAUD} baud without resetting"
    info 'this board reboots itself when WiFi fails, so a banner usually arrives anyway'
fi

# Run python from PROJECT_DIR so pyenv's .python-version (the LoRa virtualenv,
# which is what has pyserial) applies no matter where this was invoked from.
cd -- "${PROJECT_DIR}"

export PORT BAUD SECONDS_TO_READ MASK RESET OUT_PATH SECRETS_FILE
export LIB_DIR="${SCRIPTS_DIR}/lib"

python3 - <<'PYTHON'
import os, sys, time

sys.path.insert(0, os.environ['LIB_DIR'])
from mask import mask as _mask, residual as _residual, load_literals

try:
    import serial
except ImportError:
    sys.exit("error: pyserial not installed. Activate the LoRa virtualenv:\n"
             "  pyenv local LoRa && pip install pyserial esptool")

PORT    = os.environ['PORT']
BAUD    = int(os.environ['BAUD'])
SECS    = float(os.environ['SECONDS_TO_READ'])
MASK    = os.environ['MASK'] == '1'
OUT     = os.environ['OUT_PATH']
RESET   = os.environ['RESET'] == '1'
SECRETS = os.environ['SECRETS_FILE']

try:
    if RESET:
        port = serial.Serial(PORT, BAUD, timeout=0.5)
    else:
        # Opening a port normally asserts DTR/RTS, and on this board RTS is
        # wired to EN -- so the mere act of opening reboots the chip. Set both
        # lines before open() and pyserial applies them as it configures the
        # port, instead of letting them default high and pulsing the reset.
        port = serial.Serial()
        port.port = PORT
        port.baudrate = BAUD
        port.timeout = 0.5
        port.dtr = False
        port.rts = False
        port.open()
except serial.SerialException as exc:
    sys.exit(f"error: cannot open {PORT}: {exc}\n"
             "Another program may hold it -- close any miniterm or serial monitor.")

with port:
    chunks = []
    if RESET:
        # ESP32 auto-reset: RTS drives EN, DTR drives IO0. DTR must stay HIGH
        # (rts/dtr False == line asserted high here) so the chip boots the
        # sketch rather than the ROM bootloader. Pulsing RTS is what actually
        # resets it.
        port.dtr = False
        port.rts = True
        time.sleep(0.15)
        port.reset_input_buffer()
        port.rts = False
    else:
        # Listening to a board that is already running. Drop whatever was
        # buffered before we arrived: it is almost always a part-line.
        time.sleep(0.1)
        port.reset_input_buffer()

    deadline = time.time() + SECS
    while time.time() < deadline:
        chunks.append(port.read(4096))

raw = b''.join(chunks)
if not raw:
    if RESET:
        sys.exit(f"error: no output in {SECS:g}s. The board reset but said nothing.\n"
                 "Check the baud rate, or that this is really the gateway.")
    sys.exit(f"error: nothing in {SECS:g}s. With --no-reset the board is never\n"
             "prodded, so it only speaks when it has something to say. Give it\n"
             "longer with --seconds, or drop --no-reset to force a boot banner.")

# With a reset, the ROM bootloader's preamble arrives first at 74880 baud, so at
# 115200 it lands as mojibake. Reading it at 74880 instead does NOT help: the
# ch341 driver rejects that rate, and a two-phase read was measured to be worse
# -- it garbled the preamble anyway AND swallowed the start of the banner.
# Keep the garbage: it is how you tell the board really did reboot. --no-reset
# is what avoids it.
text = raw.decode('utf-8', 'replace').replace('\r\n', '\n')
text = ''.join(ch if ch == '\n' or ch.isprintable() else '.' for ch in text)

counts = {}
if MASK:
    text, counts = _mask(text, load_literals(SECRETS))
    left = _residual(text)
    if left:
        sys.exit("error: masking left secret material behind: "
                 + ", ".join(f"{k} x{v}" for k, v in left.items())
                 + "\nNot printing it. Add the literal to " + SECRETS)

print()
print(text.strip())
print()

print(f"  {len(raw):,} bytes read")
if MASK:
    print("  masked  : " + (", ".join(f"{k} x{v}" for k, v in counts.items())
                            if counts else "nothing matched"))
    print("  residual: none — safe to paste")
else:
    print("  masked  : NOTHING — this output contains the WiFi password")

# Step 4 of /board-check: say which firmware is running, and do not infer it.
# With --no-reset there is usually no banner to read, and absence of one says
# nothing about the firmware -- claiming a reflash there would be a false alarm.
if 'ESP32 defined, freq=' in text:
    freq = text.split('ESP32 defined, freq=')[1].split()[0]
    print(f"  firmware: stock ESP-1ch-Gateway, freq={freq}")
elif not RESET:
    print("  firmware: not determined — no banner, which is expected with "
          "--no-reset. Re-run without it to identify the firmware.")
else:
    print("  firmware: NOT the stock banner — the board has been reflashed "
          "since these notes were written")

if OUT:
    with open(OUT, 'w', encoding='utf-8') as fh:
        fh.write(text)
    print(f"  written : {OUT}" + ("" if MASK else "  (UNMASKED)"))
PYTHON

section 'result'
ok 'banner captured'
