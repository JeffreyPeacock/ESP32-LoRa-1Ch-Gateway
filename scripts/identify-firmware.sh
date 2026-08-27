#!/usr/bin/env bash
#
# Say which source tree a flash image was built from.
#
# Compiled firmware keeps its string literals verbatim in the binary, so the
# question "is this image built from tree X?" reduces to "how many of tree X's
# distinctive literals appear in the image?". That is a far stronger answer
# than comparing a boot banner by eye, because it tests hundreds of strings
# instead of one.
#
# Distinctive means: the literal appears in exactly ONE of the candidate trees.
# Strings common to all of them (Arduino boilerplate, "OK", format specifiers)
# carry no information about which tree was used and are excluded.
#
# Pair it with dump-flash.sh:
#   ./scripts/dump-flash.sh --region app
#   ./scripts/identify-firmware.sh

set -euo pipefail

# shellcheck source-path=SCRIPTDIR
# shellcheck source=lib/common.sh
source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)/lib/common.sh"

IMAGE=''
FW_DIR="${PROJECT_DIR}/etc/firmware"
MIN_LEN=8

usage() {
    cat <<USAGE
Usage: $(basename -- "$0") [options] [IMAGE]

Identify which mirrored source tree a flash image was built from.

  IMAGE            the .bin to identify (default: newest in
                   .claude_artifacts/flash)
  --firmware DIR   where the candidate source zips live (default: etc/firmware)
  --min-len N      shortest literal to consider (default: ${MIN_LEN})
  -h, --help       this message

Reports, for each candidate tree, how many of its DISTINCTIVE string literals
appear in the image. Distinctive means the literal occurs in that tree and in
no other candidate, so a high score cannot be earned by shared boilerplate.
USAGE
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --firmware) FW_DIR="${2:?--firmware needs a value}"; shift 2 ;;
        --min-len)  MIN_LEN="${2:?--min-len needs a value}"; shift 2 ;;
        -h|--help)  usage; exit 0 ;;
        -*)         die "unknown option: $1 (try --help)" ;;
        *)          IMAGE="$1"; shift ;;
    esac
done

if [[ -z ${IMAGE} ]]; then
    IMAGE="$(find "${PROJECT_DIR}/.claude_artifacts/flash" -name '*.bin' \
             ! -name 'partition-table.bin' -printf '%T@ %p\n' 2>/dev/null \
             | sort -rn | head -1 | cut -d' ' -f2-)"
    [[ -n ${IMAGE} ]] || die 'no image found. Run ./scripts/dump-flash.sh first, or pass a path.'
fi
[[ -f ${IMAGE} ]] || die "no such image: ${IMAGE}"

section 'image'
info "$(basename -- "${IMAGE}") — $(numfmt --to=iec "$(stat -c%s "${IMAGE}")")"
info "sha256: $(sha256sum "${IMAGE}" | cut -d' ' -f1)"

section 'candidates'
export IMAGE FW_DIR MIN_LEN
python3 - <<'PYTHON'
import os, re, sys, zipfile

IMAGE   = os.environ['IMAGE']
FW_DIR  = os.environ['FW_DIR']
MIN_LEN = int(os.environ['MIN_LEN'])

blob = open(IMAGE, 'rb').read()

# Pull string literals out of the Arduino sources. F("...") is the PROGMEM
# wrapper this sketch uses everywhere; plain "..." covers the rest.
LITERAL = re.compile(r'"((?:[^"\\\n]|\\.){%d,})"' % MIN_LEN)

def literals(zip_path):
    out = set()
    try:
        z = zipfile.ZipFile(zip_path)
    except zipfile.BadZipFile:
        return out
    for n in z.namelist():
        if not re.search(r'\.(ino|h|cpp|c)$', n) or '/lib' in n.lower():
            continue
        try:
            text = z.read(n).decode('utf-8', 'replace')
        except Exception:
            continue
        for m in LITERAL.findall(text):
            # Skip format-only strings and anything with escapes we would have
            # to interpret to match the compiled bytes.
            if '\\' in m or '%' in m:
                continue
            out.add(m)
    return out

cands = {}
for fn in sorted(os.listdir(FW_DIR)):
    if fn.endswith('.zip'):
        lits = literals(os.path.join(FW_DIR, fn))
        if lits:
            cands[fn] = lits

if not cands:
    sys.exit(f"error: no candidate source zips in {FW_DIR}")

# Distinctive = present in exactly one candidate.
counts = {}
for lits in cands.values():
    for s in lits:
        counts[s] = counts.get(s, 0) + 1

rows = []
for name, lits in cands.items():
    uniq = {s for s in lits if counts[s] == 1}
    if not uniq:
        rows.append((name, 0, 0, 0.0))
        continue
    found = sum(1 for s in uniq if s.encode() in blob)
    rows.append((name, found, len(uniq), 100.0 * found / len(uniq)))

rows.sort(key=lambda r: r[3], reverse=True)

print(f"  {'candidate source tree':<46} {'matched':>16}   {'share'}")
print("  " + "-" * 76)
for name, found, total, pct in rows:
    print(f"  {name:<46} {found:>7} / {total:<6} {pct:>7.1f}%")
print("  " + "-" * 76)

best = rows[0]
if best[2] == 0 or best[3] < 5:
    print("\n  VERDICT: no candidate matches. This image was not built from any\n"
          "           source tree in " + FW_DIR)
else:
    runner = rows[1][3] if len(rows) > 1 else 0.0
    print(f"\n  VERDICT: {best[0]}")
    print(f"           {best[3]:.1f}% of its distinctive literals are in the image, "
          f"against {runner:.1f}% for the next best.")
PYTHON

section 'result'
ok 'identification complete'
