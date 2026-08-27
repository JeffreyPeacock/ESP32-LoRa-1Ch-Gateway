# Board and repository configuration

**Single source for the identifiers the ticket commands need.** The sibling
Heltec project repeats these in all six command files; changing a field there
means six edits and one of them gets missed. Here they live once and the
commands point at this file.

**This file is deliberately not in `.claude/commands/`.** Anything in that
directory becomes a slash command, and this is data.

## Identifiers

Created 2026-08-27. All live.

| | |
|---|---|
| Owner | `JeffreyPeacock` — a **user**, not an org. GraphQL uses `user(login:)`, `gh project` needs `--owner JeffreyPeacock` |
| Repo | `JeffreyPeacock/ESP32-LoRa-1Ch-Gateway` |
| Project | **#13**, <https://github.com/users/JeffreyPeacock/projects/13> |
| Project ID | `PVT_kwHOAdChXs4Bhr4A` |
| Status field | `PVTSSF_lAHOAdChXs4Bhr4AzhgmtoE` |
| Priority field | `PVTSSF_lAHOAdChXs4Bhr4Azhgmto0` |
| Area field | `PVTSSF_lAHOAdChXs4Bhr4Azhgmto4` |

**Status** — Backlog `da2e6d4d` · Prioritized `2bf6b731` · Ready `e353a1e7` ·
In Progress `f9411084` · Completed `4f793530`

**Priority** — p1 `ebf134ed` · p2 `7d85e4db` · p3 `c1271b51` · p4 `7d45d2ec` ·
p5 `7d01d1b6`

**Area** — Radio `5ff937ac` · Firmware `2f7a4d49` · Tooling `efcfd9fc` ·
Docs `04ec6e87` · Hardware `d15f5e87` · LoRaWAN `ac07ca52` · Interop `d2977905`

Both the repository and the project are **public**.

## Re-reading these if they ever change

```bash
gh project field-list 13 --owner JeffreyPeacock --format json \
  --jq '.fields[] | select(.options) | {field:.name, id:.id,
        options:[.options[] | {name, id}]}'
```

## Area is a board field, not a label

This project was copied from the sibling **SAMD21-LoRa-ProRF** board (project
#11), which uses a single-select **Area** field rather than topic labels. That
convention is kept. Two of its areas were dropped because this board does not
have the hardware — `Qwiic` (no Qwiic connector) and `Logging` (no OpenLog, no
SD card) — and three were added: `Hardware`, `LoRaWAN`, `Interop`.

**Set Area on every ticket.** Labels are still available for cross-cutting
things, but the topical classification lives in Area.

## Conventions this project uses

**Priority is a board field, not a label.** Do not create `priority:pN` labels.
A ticket with no priority is a triage failure, not an implied p3.

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
