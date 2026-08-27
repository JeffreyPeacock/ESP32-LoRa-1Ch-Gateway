# Trim CLAUDE.md

Bring `CLAUDE.md` back under the 40k character performance limit without losing
anything expensive to rediscover.

## Step 1 — Measure

```bash
wc -c CLAUDE.md
```

Under 30k: report the size and stop. Nothing needs trimming, and cutting a file
that is not too large only loses information.

This project grew `CLAUDE.md` from 4k to 21k in a single session, so measure
rather than assume; it climbs faster than it feels like it should.

## Step 2 — Triage before touching the file

Assign every candidate passage to a destination **before** editing:

| Bucket | Criteria | Action |
|---|---|---|
| **Delete** | One-time setup notes for work already done; narrative of how something was discovered once the finding itself is stated; anything derivable from the code in front of you | Remove |
| **Condense** | A finding that matters but is explained at three times the length it needs | Rewrite to a sentence or two, keeping the *fact* and the *consequence* |
| **Move to README** | Orientation, procedure, or rationale a human arriving cold would want | Add to `README.md` first, then remove |
| **Move to docs/** | Reference too long for README but too valuable to lose | New `docs/<subject>.md`, linked from README |
| **Save to memory** | Cross-project lessons about how to work, not about this repo | Write the memory file first, then remove |

**Never delete without assigning a destination.** Record the disposition in the
Step 5 report.

### Never cut these

This file is mostly hard-won findings. Each of these cost real time and none is
recoverable by reading the code:

- The **radio pin map** — NSS 16 / RST 5 / DIO0 26 on SPI **SCK 14 / MISO 12 /
  MOSI 13**, which matches no stock Meshtastic or RNode target and is the reason
  neither runs here. The SPI half was recorded wrongly as VSPI once already;
  keep the schematic citation that corrects it
- **esptool's stub loader fails over this CH340** — `--no-stub` for bulk transfers
- **The stock firmware prints WiFi credentials on every boot**
- **4 MB of flash is what dates the board** to WRL-15006 rather than WRL-18074
- **TTN v3 refuses single-channel gateways**, so LoRaWAN work needs a private server
- The **`grep -q` under `pipefail`** trap, and the `head()` shadowing trap
- The **no-attribution rule**
- That none of the sibling Heltec project's SX1262 facts apply to this board

Prefer condensing prose around a finding over deleting the finding.

## Step 3 — Preserve displaced content first

Write to README, `docs/`, or memory **before** removing anything from
`CLAUDE.md`. If the process is interrupted between the two, the version that
loses nothing is the one where the copy already exists.

**Treat the repository as publishable.** Moving content to README exposes it. Check for
coordinates, credentials, and channel keys before doing so.

## Step 4 — Edit, then verify

Make targeted edits only. Re-measure:

```bash
wc -c CLAUDE.md
```

Target 30–35k, leaving headroom. If still over 40k, return to Step 2 rather than
cutting deeper into findings.

Sanity-check that the command table still matches the files on disk:

```bash
diff <(grep -oE '^\| `/[a-z-]+' CLAUDE.md | tr -d '|` /' | sort) \
     <(ls -1 .claude/commands/ | sed 's/\.md$//' | sort)
```

## Step 5 — Report and commit

List every section touched and its disposition — deleted, condensed, moved to
README, moved to `docs/`, or saved to memory — with before and after sizes.

Commit on `main` with a `Docs:` prefix, staging only the files this command
changed. No AI attribution.
