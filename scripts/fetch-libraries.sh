#!/usr/bin/env bash
#
# Fetch the gateway sketch's libraries from GitHub into a local, ignored folder.
#
# This repository is public and these libraries are other people's code, so they
# are not checked in. What is checked in is this script and the refs it pins --
# the provenance, not the payload, the same rule docs/assets.md follows.
#
# Two sources:
#
#   1. Eleven libraries ship INSIDE the gateway repo, under libraries/. They are
#      pinned to commit 88113e8, which is the exact revision the board's own
#      firmware was built from (VERSION "V.5.3.3.H; 180825a") -- so what you
#      compile against matches what is on the board rather than something newer.
#
#   2. Two come from the Arduino Library Manager. The upstream README names
#      exact versions, and they are old enough that "latest" is wrong:
#      ArduinoJson 5.13.1 (the 6.x API is incompatible) and WiFiManager 0.12.
#
# WiFiManager is only needed if you set WIFIMANAGER=1. The include is guarded
# (ESP-sc-gway.ino:66), and this board's firmware was built with it off, so it
# is skipped unless you ask for it with --with-wifimanager.

set -euo pipefail

# shellcheck source-path=SCRIPTDIR
# shellcheck source=lib/common.sh
source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/lib/common.sh"

DEST="${PROJECT_DIR}/vendor"
FORCE=0
WITH_WIFIMANAGER=0

# repo|ref|what-to-extract|destination-name
GATEWAY_REPO='things4u/ESP-1ch-Gateway-v5.0--OLD'
GATEWAY_REF='88113e8'

usage() {
    cat <<USAGE
Usage: $(basename -- "$0") [options]

Fetch the ESP-1ch-Gateway libraries from GitHub into a local folder.

  --dest DIR            where to put them (default: vendor/)
  --force               re-fetch even if already present
  --with-wifimanager    also fetch WiFiManager 0.12 (only needed if you set
                        WIFIMANAGER=1; this board's build has it off)
  -h, --help            this message

Nothing here is committed -- 'vendor/' is gitignored. Re-run after a fresh
clone. Note this is NOT 'lib/': that holds this project's own shared libraries
and IS committed.

Using them:
  Arduino IDE   point your sketchbook at a folder whose 'libraries/' is this one
  PlatformIO    already wired up -- platformio.ini has lib_extra_dirs = vendor
USAGE
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --dest)             DEST="${2:?--dest needs a value}"; shift 2 ;;
        --force)            FORCE=1; shift ;;
        --with-wifimanager) WITH_WIFIMANAGER=1; shift ;;
        -h|--help)          usage; exit 0 ;;
        *)                  die "unknown option: $1 (try --help)" ;;
    esac
done

command -v curl >/dev/null 2>&1 || die 'curl is required'
command -v tar  >/dev/null 2>&1 || die 'tar is required'

mkdir -p -- "${DEST}"
TMP="$(mktemp -d)"
trap 'rm -rf -- "${TMP}"' EXIT

# Fetch a repo tarball at a pinned ref and extract part of it.
#   $1 owner/repo   $2 ref   $3 tar wildcard   $4 strip depth   $5 dest subdir
fetch() {
    local repo="$1" ref="$2" pattern="$3" strip="$4" into="$5"
    local url="https://codeload.github.com/${repo}/tar.gz/${ref}"
    # Declared before assignment: `local x="$(...)"` would swallow the command
    # substitution's exit status, since local itself always succeeds.
    local slug tarball
    slug="$(printf '%s' "${repo}" | tr '/' '-')"
    tarball="${TMP}/${slug}-${ref}.tar.gz"

    curl -fsSL -o "${tarball}" "${url}" \
        || die "could not fetch ${repo}@${ref} from ${url}"

    mkdir -p -- "${into}"
    # --wildcards so we need not know the archive's top-level directory name,
    # which embeds the ref and so changes whenever the pin changes.
    tar -xzf "${tarball}" --wildcards --strip-components="${strip}" \
        -C "${into}" "${pattern}" \
        || die "could not extract '${pattern}' from ${repo}@${ref}"
}

section 'gateway libraries'
if [[ -d ${DEST}/Streaming && ${FORCE} -eq 0 ]]; then
    info "already present (use --force to re-fetch)"
else
    info "${GATEWAY_REPO}@${GATEWAY_REF} -> ${DEST}"
    rm -rf -- "${TMP}/gw"
    fetch "${GATEWAY_REPO}" "${GATEWAY_REF}" '*/libraries/*' 2 "${DEST}"
    ok "$(find "${DEST}" -maxdepth 1 -mindepth 1 -type d | wc -l) libraries"
fi

section 'library manager dependencies'
# ArduinoJson 5.x. The sketch uses the v5 API; v6 renamed everything and will
# not compile. The gateway repo BUNDLES a copy, so this must check the version
# rather than merely whether the directory exists -- otherwise a wrong bundled
# version would be accepted silently, which is exactly the failure this pinning
# is meant to prevent.
JSON_WANT='5.13.1'
JSON_HAVE=''
[[ -f ${DEST}/ArduinoJson/library.properties ]] \
    && JSON_HAVE="$(sed -n 's/^version=//p' "${DEST}/ArduinoJson/library.properties" | tr -d '\r')"

if [[ ${JSON_HAVE} == "${JSON_WANT}" && ${FORCE} -eq 0 ]]; then
    info "ArduinoJson ${JSON_HAVE} — the copy bundled in the gateway repo is already the pinned version"
elif [[ -n ${JSON_HAVE} && ${FORCE} -eq 0 ]]; then
    warn "bundled ArduinoJson is ${JSON_HAVE}, want ${JSON_WANT} — replacing it"
    rm -rf -- "${DEST}/ArduinoJson"
    fetch 'bblanchon/ArduinoJson' "v${JSON_WANT}" '*' 1 "${DEST}/ArduinoJson"
    ok "ArduinoJson ${JSON_WANT}"
else
    info "bblanchon/ArduinoJson@v${JSON_WANT} -> ArduinoJson/"
    rm -rf -- "${DEST}/ArduinoJson"
    fetch 'bblanchon/ArduinoJson' "v${JSON_WANT}" '*' 1 "${DEST}/ArduinoJson"
    ok "ArduinoJson ${JSON_WANT}"
fi

if [[ ${WITH_WIFIMANAGER} -eq 1 ]]; then
    if [[ -d ${DEST}/WiFiManager && ${FORCE} -eq 0 ]]; then
        info 'WiFiManager already present'
    else
        info 'tzapu/WiFiManager@0.12 -> WiFiManager/'
        rm -rf -- "${DEST}/WiFiManager"
        fetch 'tzapu/WiFiManager' '0.12' '*' 1 "${DEST}/WiFiManager"
        ok 'WiFiManager 0.12'
    fi
else
    info 'WiFiManager skipped -- only needed with WIFIMANAGER=1 (--with-wifimanager)'
fi

# A stamp so a later reader can tell what these are and where they came from,
# without this folder being mistaken for code that belongs to the project.
cat > "${DEST}/FETCHED.md" <<STAMP
# Fetched libraries — not part of this project

Third-party code pulled by \`scripts/fetch-libraries.sh\`. **Not committed**;
this whole directory is gitignored. Delete it and re-run the script at any time.
This is *not* \`lib/\`, which holds the project's own shared libraries.

| Library | Source | Ref |
|---|---|---|
| the eleven shipped with the sketch | \`${GATEWAY_REPO}\` \`libraries/\` | \`${GATEWAY_REF}\` — the commit this board's firmware was built from |
| ArduinoJson | \`bblanchon/ArduinoJson\` | \`v5.13.1\` — the 6.x API is incompatible |
| WiFiManager | \`tzapu/WiFiManager\` | \`0.12\` — only if \`WIFIMANAGER=1\` |
STAMP

section 'result'
info "destination: ${DEST}"
find "${DEST}" -maxdepth 1 -mindepth 1 -type d -printf '  %f\n' | sort
ok 'libraries fetched'
