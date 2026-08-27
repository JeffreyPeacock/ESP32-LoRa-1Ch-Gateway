# Board and repository configuration

**Single source for the identifiers the ticket commands need.** The sibling
Heltec project repeats these in all six command files; changing a field there
means six edits and one of them gets missed. Here they live once and the
commands point at this file.

**This file is deliberately not in `.claude/commands/`.** Anything in that
directory becomes a slash command, and this is data.

## Status: NOT YET CREATED

Neither the GitHub repository nor the Project exists as of 2026-08-27. Every
value below is a placeholder. **The commands that read this file must stop and
say so rather than guessing** if they find a `<...>` still in place.

| | |
|---|---|
| Owner | `JeffreyPeacock` — a **user**, not an org. GraphQL uses `user(login:)`, `gh project` needs `--owner JeffreyPeacock` |
| Repo | `<repo>` — likely `JeffreyPeacock/ESP32-LoRa-1Ch-Gateway` |
| Project number | `<N>` |
| Project ID | `<PVT_...>` |
| Status field ID | `<PVTSSF_...>` |
| Priority field ID | `<PVTSSF_...>` |

Status options: Backlog `<id>` · Prioritized `<id>` · Ready `<id>` ·
In Progress `<id>` · Completed `<id>`

Priority options: p1 `<id>` · p2 `<id>` · p3 `<id>` · p4 `<id>` · p5 `<id>`

## Filling this in

Run once, after creating the Project, and paste the results above.

```bash
OWNER=JeffreyPeacock
NUM=<project-number>

# Project ID
gh project view "$NUM" --owner "$OWNER" --format json --jq '.id'

# Field IDs and every single-select option ID, in one call
gh project field-list "$NUM" --owner "$OWNER" --format json \
  --jq '.fields[] | select(.options) | {field: .name, id: .id,
        options: [.options[] | {name, id}]}'
```

## Conventions this project uses

**Priority is a board field, not a label.** Do not create `priority:pN` labels.
A ticket with no priority is a triage failure, not an implied p3.

### Labels

| Label | Use for |
|---|---|
| `hardware` | The board itself: pins, flashing, power, antenna, the GPIO12 strap |
| `radio` | SX1276 driver and register work, modulation settings, RSSI/SNR |
| `firmware` | Applications under `src/`, PlatformIO environments |
| `tooling` | `scripts/`, `Makefile`, port detection, credential masking |
| `docs` | The written findings, which are most of this project's value |
| `lorawan` | Gateway role, ChirpStack, band plan |
| `interop` | The SAMD21 Pro RF link, or anything needing the other developer |
| `decision` | An open choice not yet settled |

More than one is fine. If nothing fits, propose a new label rather than forcing
a bad one.

### Milestones

| Milestone | Criterion |
|---|---|
| `Solo bench` | Completable **and verifiable** with this board alone |
| `Two-radio link` | Needs the SAMD21 Pro RF, or its developer, or both |

The test is **verifiability, not topic**. Writing up a measurement method is
solo; taking a reading that requires the other radio transmitting is not.

### Priority rubric

| Pri | Meaning |
|---|---|
| **p1** | The next step cannot happen until it is done |
| **p2** | Needed for the current milestone, no acceptable workaround |
| **p3** | Real work, schedulable, a workaround or deferral exists |
| **p4** | Minor, cosmetic, or a convenience |
| **p5** | Speculative, or waiting on something far off |

Do not inflate p1.

## Two constraints specific to this board

**One board, one application.** `build_src_filter` compiles exactly one
`main()`, so `listen`, `gateway`, `link`, `probe`, `scan` and `sweep` are
mutually exclusive. Tickets needing different applications cannot be
interleaved — group them and do all of one before reflashing.

**Flashing sometimes needs a physical power cycle**, and no command can do it.
The radio's MISO is GPIO12, the flash-voltage strap, so a flash or a boot fails
intermittently until the USB is unplugged and replugged. See `CLAUDE.md`. Treat
"needs a power cycle" as a distinct blocked state, not a failure.

## The repository is public

The stock firmware printed WiFi credentials on every boot, and this board has
leaked them into a session log twice. **Mask any serial output before putting it
in an issue, a PR, or a commit.** `scripts/lib/mask.py` and
`scripts/export-transcript.sh` do it; a hand-written `sed` for `.SSID=` alone is
exactly how it leaked.
