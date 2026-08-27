# shellcheck shell=bash
#
# Shared output helpers and paths.
#
# Sourced, never executed. The caller is responsible for `set -euo pipefail`.
# This is the small subset of the Heltec project's lib/heltec-common.sh that is
# not specific to that board -- deliberately not a copy, because the rest of
# that file is CP2102N and udev handling that does not apply to a CH340C.

[[ -n ${_COMMON_LOADED:-} ]] && return 0
_COMMON_LOADED=1

_LIB_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SCRIPTS_DIR="$(dirname -- "${_LIB_DIR}")"
# shellcheck disable=SC2034  # consumed by the sourcing script
readonly SCRIPTS_DIR
PROJECT_DIR="$(dirname -- "${SCRIPTS_DIR}")"
# shellcheck disable=SC2034  # consumed by the sourcing script
readonly PROJECT_DIR

FAILURES=0

ok()      { printf '  [ ok ] %s\n' "$*"; }
info()    { printf '  [ -- ] %s\n' "$*"; }
warn()    { printf '  [warn] %s\n' "$*" >&2; }
fail()    { printf '  [FAIL] %s\n' "$*" >&2; FAILURES=$((FAILURES + 1)); }
section() { printf '\n== %s ==\n' "$*"; }
die()     { printf 'error: %s\n' "$*" >&2; exit 1; }

# Echoes the path of this board's serial port, or dies explaining why not.
#
# The bridge is a CH340C (1a86:7523). The Heltec board in the sibling project is
# a CP2102N (10c4:ea60), so with both attached the vendor/model pair separates
# them. CH340s carry no serial number, so two of THEM cannot be told apart --
# say so rather than guessing, and let the caller pass an explicit port.
find_ch340_port() {
    local dev props
    local -a found=()
    for dev in /dev/ttyUSB*; do
        [[ -e ${dev} ]] || continue
        props="$(udevadm info -q property -n "${dev}" 2>/dev/null || true)"
        if grep -q '^ID_VENDOR_ID=1a86$' <<<"${props}" \
        && grep -q '^ID_MODEL_ID=7523$' <<<"${props}"; then
            found+=("${dev}")
        fi
    done
    case ${#found[@]} in
        0) die 'no CH340 (1a86:7523) found. Is the board plugged in with a DATA cable?' ;;
        1) printf '%s\n' "${found[0]}" ;;
        *) die "found ${#found[@]} CH340s (${found[*]}). They carry no serial number, so pick one explicitly or use /dev/serial/by-path/." ;;
    esac
}

# Takes an exclusive lock on a serial port, or dies rather than corrupting a
# transfer already in progress.
#
# Nothing in Linux stops two processes opening the same tty at once. When that
# happens to a long esptool read the symptom is a mid-transfer abort --
# "device reports readiness to read but returned no data (device disconnected
# or multiple access on port?)". A 16%-complete flash dump died that way on
# 2026-08-21 because a banner read was started alongside it, and it is a
# credible explanation for the earlier failed backup attempts noted in
# CLAUDE.md.
#
# The lock is advisory and only binds scripts in this project that call this,
# so it does not protect against a stray miniterm. It does stop us tripping
# over ourselves, which is what actually happened.
lock_port() {
    local port="$1"
    local lockfile="/tmp/.esp32-lora-1ch-gw$(printf '%s' "${port}" | tr '/' '-').lock"
    # fd 9 stays open for the lifetime of the script, so the lock is held until
    # it exits, however it exits.
    exec 9>"${lockfile}" || die "cannot create lock file ${lockfile}"
    if ! flock -n 9; then
        die "${port} is locked by another script in this project (see ${lockfile}).
A flash read or banner capture is probably already running. Wait for it --
opening the port twice aborts the transfer that is already in flight."
    fi
}

# Prints the pass/fail tail of a check run and returns the right exit status.
report_result() {
    section 'result'
    if [[ ${FAILURES} -eq 0 ]]; then
        ok 'all checks passed'
        return 0
    fi
    fail "${FAILURES} check(s) failed"
    return 1
}
