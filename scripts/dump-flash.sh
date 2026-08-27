#!/usr/bin/env bash
#
# Read firmware off the board so it can be checksummed or compared.
#
# Reads the real partition table first rather than assuming the standard ESP32
# layout, then dumps the partition you asked for. Default is the running app,
# which is the part you want for identifying firmware and is roughly a third of
# the flash, so roughly a third of the wait.
#
# SPIFFS and NVS dumps contain the stored WiFi password in the clear, which is
# what you would expect. The app partition is NOT automatically clean either:
# ESP-sc-gway.h:261 declares a compile-time wpa[] table, and this image carries
# an SSID/password pair at flash 0x3f478. On this board those are test values
# rather than live credentials, but the mechanism is general -- reflash with
# real ones in wpa[] and the app image carries them.
#
# Output goes to .claude_artifacts/ , which .gitignore excludes wholesale.
#
# This is slow. `--no-stub` is not optional on this board -- the stub loader
# fails over this CH340 at 921600 and at 460800 -- and without it everything
# runs at 115200. Measured: 1.3 MB app partition in 243 s (43.4 kbit/s), so all
# 4 MB takes about 12 minutes. Run it in the background rather than waiting.

set -euo pipefail

# shellcheck source-path=SCRIPTDIR
# shellcheck source=lib/common.sh
source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/lib/common.sh"

PORT=''
REGION='app'
OUT_DIR="${PROJECT_DIR}/.claude_artifacts/flash"
BAUD=115200

usage() {
    cat <<USAGE
Usage: $(basename -- "$0") [options]

Read firmware off the board and checksum it.

  --port PATH      serial port (default: autodetect the CH340, 1a86:7523)
  --region WHAT    'app'  the running application partition (default)
                   'full' the entire 4 MB flash
                   'list' just print the partition table and exit
                   NAME   any partition label from the table
  --out-dir DIR    where to write (default: .claude_artifacts/flash)
  --baud N         baud rate (default: ${BAUD}; --no-stub caps this in practice)
  -h, --help       this message

NOTE: 'full', 'spiffs' and 'nvs' contain the stored WiFi password in the clear.
'app' is not automatically clean either -- see the header on wpa[]. Output lands
in a gitignored directory.

Timing (measured): app 4 min, full ~12 min. Run it in the background.

Examples:
  ./scripts/dump-flash.sh --region list
  ./scripts/dump-flash.sh                       # app partition
  ./scripts/dump-flash.sh --region full &       # everything, backgrounded
USAGE
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --port)    PORT="${2:?--port needs a value}"; shift 2 ;;
        --region)  REGION="${2:?--region needs a value}"; shift 2 ;;
        --out-dir) OUT_DIR="${2:?--out-dir needs a value}"; shift 2 ;;
        --baud)    BAUD="${2:?--baud needs a value}"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *)         die "unknown option: $1 (try --help)" ;;
    esac
done

command -v esptool >/dev/null 2>&1 || die "esptool not on PATH. Activate the LoRa virtualenv (pyenv local LoRa)."

section 'port'
if [[ -z ${PORT} ]]; then
    PORT="$(find_ch340_port)"
    info "autodetected ${PORT} (CH340, 1a86:7523)"
else
    [[ -e ${PORT} ]] || die "no such port: ${PORT}"
    info "using ${PORT} (given)"
fi

mkdir -p -- "${OUT_DIR}"

lock_port "${PORT}"
cd -- "${PROJECT_DIR}"

# esptool's --no-stub is mandatory here; see the header. Keep it in one place.
esp() { esptool --port "${PORT}" --baud "${BAUD}" --no-stub "$@"; }

section 'partition table'
PT="${OUT_DIR}/partition-table.bin"
info 'reading the real table at 0x8000 rather than assuming a layout'
esp read-flash 0x8000 0xc00 "${PT}" >/dev/null 2>&1 \
    || die "could not read the partition table from ${PORT}"

# Parse it: 32-byte entries, magic 0xAA50, then type/subtype/offset/size/label.
PARTS="$(python3 - "${PT}" <<'PYTHON'
import struct, sys
TYPES = {0: 'app', 1: 'data'}
SUBTYPES = {(0,0):'factory', (1,2):'nvs', (1,0):'otadata', (1,0x82):'spiffs'}
raw = open(sys.argv[1],'rb').read()
for off in range(0, len(raw), 32):
    e = raw[off:off+32]
    if len(e) < 32 or e[:2] != b'\xaa\x50':
        break
    ptype, subtype, offset, size = struct.unpack('<BBII', e[2:12])
    label = e[12:28].rstrip(b'\x00').decode('ascii', 'replace')
    kind = TYPES.get(ptype, f'0x{ptype:02x}')
    sub  = SUBTYPES.get((ptype, subtype), f'0x{subtype:02x}')
    if ptype == 0 and 0x10 <= subtype <= 0x1f:
        sub = f'ota_{subtype - 0x10}'
    print(f"{label}\t{kind}\t{sub}\t{offset}\t{size}")
PYTHON
)"

[[ -n ${PARTS} ]] || die 'the partition table did not parse -- no valid entries found'

printf '  %-10s %-6s %-9s %10s %10s\n' LABEL TYPE SUBTYPE OFFSET SIZE
while IFS=$'\t' read -r label kind sub offset size; do
    printf '  %-10s %-6s %-9s %#10x %10s\n' \
        "${label}" "${kind}" "${sub}" "${offset}" "$(numfmt --to=iec "${size}")"
done <<<"${PARTS}"

[[ ${REGION} == list ]] && { section 'result'; ok 'partition table read'; exit 0; }

# Work out what to dump.
if [[ ${REGION} == full ]]; then
    NAME='full-4mb'; OFFSET=0; SIZE=$((4 * 1024 * 1024))
else
    want="${REGION}"
    # 'app' means the first app-type partition, whatever it happens to be called.
    if [[ ${REGION} == app ]]; then
        want="$(awk -F'\t' '$2=="app" {print $1; exit}' <<<"${PARTS}")"
        [[ -n ${want} ]] || die 'no app-type partition in the table'
    fi
    line="$(awk -F'\t' -v w="${want}" '$1==w {print; exit}' <<<"${PARTS}")"
    [[ -n ${line} ]] || die "no partition labelled '${want}'. Try --region list."
    NAME="${want}"
    OFFSET="$(cut -f4 <<<"${line}")"
    SIZE="$(cut -f5 <<<"${line}")"
fi

# Deliberately not a partition allowlist: the app image can carry compiled-in
# credentials too, so warn on every region rather than on a chosen few.
warn "a '${NAME}' dump may contain WiFi credentials in the clear -- see the header"


OUT="${OUT_DIR}/${NAME}-$(printf '0x%x' "${OFFSET}")-$(numfmt --to=iec "${SIZE}").bin"

section 'read'
info "dumping ${NAME}: $(numfmt --to=iec "${SIZE}") from $(printf '0x%x' "${OFFSET}")"
info "this is slow -- --no-stub at ${BAUD} baud. Expect several minutes."
info "output: ${OUT}"

START=${SECONDS}
esp read-flash "${OFFSET}" "${SIZE}" "${OUT}" || die "read-flash failed. See CLAUDE.md: the stub loader fails on this CH340, which is why --no-stub is forced."
ELAPSED=$((SECONDS - START))

section 'result'
[[ -s ${OUT} ]] || die "wrote no data to ${OUT}"
ok "$(numfmt --to=iec "$(stat -c%s "${OUT}")") in ${ELAPSED}s"
info "sha256: $(sha256sum "${OUT}" | cut -d' ' -f1)"
info "compare with: sha256sum ${OUT}"
