#!/usr/bin/env bash
#
# Put the original 2019 gateway firmware back.
#
# The board shipped running ESP-1ch-Gateway V.5.3.3.H; 180825a and was left
# untouched until 2026-08-21. Before anything was flashed over it, three regions
# were read off the chip: the bootloader, the partition table, and the app. This
# writes those three back, which restores the gateway exactly.
#
# SPIFFS is NOT in that set and does not need to be: an application upload never
# touches it, so the stored gateway config -- WiFi credentials, frequency, SF --
# survives whatever was flashed in between. That is also why restoring brings
# back a *configured* gateway rather than a blank one.
#
# Expects the images written by dump-flash.sh, in .claude_artifacts/flash/.

set -euo pipefail

# shellcheck source-path=SCRIPTDIR
# shellcheck source=lib/common.sh
source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/lib/common.sh"

PORT=''
IMG_DIR="${PROJECT_DIR}/.claude_artifacts/flash"
BAUD=115200
ASSUME_YES=0

usage() {
    cat <<USAGE
Usage: $(basename -- "$0") [options]

Restore the original ESP-1ch-Gateway firmware from the pre-flash dumps.

  --port PATH      serial port (default: autodetect the CH340, 1a86:7523)
  --img-dir DIR    where the dumps live (default: .claude_artifacts/flash)
  --baud N         baud rate (default: ${BAUD})
  --yes            do not prompt before writing
  -h, --help       this message

Writes three regions and leaves SPIFFS alone:

  0x1000    bootloader-0x1000-28K.bin
  0x8000    partition-table.bin
  0x10000   app0-0x10000-1.3M.bin

Takes about 5 minutes: --no-stub is forced, because the stub loader fails over
this board's CH340.
USAGE
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --port)    PORT="${2:?--port needs a value}"; shift 2 ;;
        --img-dir) IMG_DIR="${2:?--img-dir needs a value}"; shift 2 ;;
        --baud)    BAUD="${2:?--baud needs a value}"; shift 2 ;;
        --yes)     ASSUME_YES=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *)         die "unknown option: $1 (try --help)" ;;
    esac
done

command -v esptool >/dev/null 2>&1 || die "esptool not on PATH. Activate the LoRa virtualenv (pyenv local LoRa)."

BOOTLOADER="${IMG_DIR}/bootloader-0x1000-28K.bin"
PARTITIONS="${IMG_DIR}/partition-table.bin"
APPLICATION="${IMG_DIR}/app0-0x10000-1.3M.bin"

section 'images'
for img in "${BOOTLOADER}" "${PARTITIONS}" "${APPLICATION}"; do
    [[ -s ${img} ]] || die "missing or empty: ${img}
Without all three this cannot restore the board. Re-read them with dump-flash.sh
BEFORE flashing anything else, or the originals are gone."
    info "$(printf '%-34s %8s bytes  sha256:%s' \
        "$(basename -- "${img}")" "$(stat -c%s "${img}")" \
        "$(sha256sum "${img}" | cut -c1-16)")"
done

# A truncated app image would flash "successfully" and produce a board that
# boot-loops. The dump is a whole partition, so its size is known exactly.
EXPECTED_APP=1310720
actual=$(stat -c%s "${APPLICATION}")
[[ ${actual} -eq ${EXPECTED_APP} ]] \
    || die "app image is ${actual} bytes, expected ${EXPECTED_APP}. A short dump means an interrupted read -- do not flash it."

section 'port'
if [[ -z ${PORT} ]]; then
    PORT="$(find_ch340_port)"
    info "autodetected ${PORT} (CH340, 1a86:7523)"
else
    [[ -e ${PORT} ]] || die "no such port: ${PORT}"
    info "using ${PORT} (given)"
fi

lock_port "${PORT}"

if [[ ${ASSUME_YES} -ne 1 ]]; then
    warn 'this overwrites whatever is currently flashed'
    read -r -p '  type "restore" to continue: ' reply
    [[ ${reply} == restore ]] || die 'cancelled'
fi

section 'write'
info 'this takes about 5 minutes at 115200 with --no-stub'
cd -- "${PROJECT_DIR}"
esptool --port "${PORT}" --baud "${BAUD}" --no-stub write-flash \
    0x1000  "${BOOTLOADER}" \
    0x8000  "${PARTITIONS}" \
    0x10000 "${APPLICATION}" \
    || die 'write-flash failed. The board may be half-written -- run this again before power-cycling.'

section 'result'
ok 'original gateway firmware restored'
info 'verify with: ./scripts/read-banner.sh --no-reset'
info 'expect the stock banner: ESP32 defined, freq=903900000'
