#!/usr/bin/env bash
#
# Print this board's serial port, or fail loudly.
#
# Exists because /dev/ttyUSBn is not stable. The number is assigned in
# enumeration order, so a power cycle -- which this board needs regularly, see
# the GPIO12 note in CLAUDE.md -- can move it. On 2026-08-21 it moved from
# ttyUSB1 to ttyUSB2 mid-session, and `make flash` failed with "busy or doesn't
# exist" while the shell scripts carried on working, because they autodetect
# and the Makefile did not.
#
# The CH340 carries no serial number, so udev cannot build a stable by-id name
# for it. /dev/serial/by-path/ IS stable, but only per physical USB socket --
# move the cable and it changes too. Detecting by vendor/model is the one thing
# that survives both.

set -euo pipefail

# shellcheck source-path=SCRIPTDIR
# shellcheck source=lib/common.sh
source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/lib/common.sh"

find_ch340_port
