# Work a ticket end to end

Work the issue number in the argument (e.g. `/fix-ticket 3`). Follow the steps
in order. If no argument was given, ask which issue and stop.

## Config

`.claude/board-config.md` holds the project and field IDs, the Area values, and
the two constraints specific to this board — one application at a time, and a
flash that sometimes needs a physical power cycle.

## Step 0 — Move the ticket to In Progress

```bash
ITEM=$(gh api graphql -f query='
{ repository(owner:"JeffreyPeacock", name:"ESP32-LoRa-1Ch-Gateway") {
    issue(number: <N>) { projectItems(first:5){ nodes{ id project{ number } } } } } }' \
  --jq '.data.repository.issue.projectItems.nodes[] | select(.project.number==13) | .id')

gh api graphql -f query='mutation {
  updateProjectV2ItemFieldValue(input:{
    projectId:"PVT_kwHOAdChXs4Bhr4A" itemId:"'"$ITEM"'"
    fieldId:"PVTSSF_lAHOAdChXs4Bhr4AzhgmtoE"
    value:{ singleSelectOptionId:"f9411084" } }){ projectV2Item{ id } } }'
```

If the item is not on the board, warn and continue. The board is bookkeeping;
the work is not.

## Step 1 — Read the ticket

```bash
gh issue view <N> --repo JeffreyPeacock/ESP32-LoRa-1Ch-Gateway --json number,title,body,labels,milestone
```

Identify what "done" means before touching anything. Ask if you are not at 95%
confidence.

**If the ticket is labelled `interop`, confirm with the user first.** It depends
on the SAMD21 Pro RF or its developer, and may not be actionable today.

## Step 2 — Decide whether the board is needed

Tickets labelled `hardware`, `radio` or `firmware` usually need it attached.

```bash
./scripts/find-port.sh      # errors if the CH340 is absent
make board                  # banner + chip-id
```

If the board is not attached and the ticket needs it, **stop and say so rather
than guessing at results. Never report a hardware outcome that was not
observed.** That rule is why the findings in `CLAUDE.md` are trustworthy.

## Step 3 — Read before editing

Read the files involved. **`CLAUDE.md` records the facts that are easy to get
wrong from memory** — check it before changing pin definitions, radio settings,
frequencies, or anything in `scripts/`. The pin map in `lib/board/board.h` is
the single definition; do not duplicate it.

## Step 4 — Branch from a current main

```bash
git checkout main && git pull --ff-only
git checkout -b <type>/<short-description>      # fix/ feat/ docs/ chore/
```

Documentation-only tickets may go straight to `main` — say which you chose.

## Step 5 — Make the smallest change that resolves the ticket

No unrelated refactoring, no drive-by cleanups.

## Step 6 — Gate locally

Whichever apply. All must pass.

```bash
make check     # bash -n on every script, then shellcheck. No output = clean
make build     # every PlatformIO environment
make test      # host-side unit tests, no board needed
```

`make test` is the gate the sibling Heltec project does not have. Anything you
can express as pure arithmetic — frequency registers, symbol durations, band
plan channel numbers — **belongs in `test/` rather than being verified by eye on
hardware.** A wrong channel number survived three documents here until a test
pinned it.

Firmware changes that can be verified on the board should be:

```bash
make flash APP=<env> && make monitor
```

Paste **real** output into the PR, masked, never what you expect it to say.

If flashing fails with a flash-communication error, or the board boots to
`flash read err`, that is the GPIO12 strap — ask the user to power-cycle. It is
not your change.

## Step 7 — Commit

```bash
git add <specific files>
git commit -m "<Type>: <description> (#<N>)"
```

**No `Co-Authored-By`, no "Generated with", no AI attribution of any kind.**
Standing rule for this repository.

Write the message to explain *why* something is true, not just that it changed.
The commit log is where the reasoning lives.

## Step 8 — Push and open a PR

```bash
git push -u origin <branch>
gh pr create --repo JeffreyPeacock/ESP32-LoRa-1Ch-Gateway \
  --title "<Type>: <description> (#<N>)" \
  --body "$(cat <<'BODY'
## Summary
- <what changed and why>

## Verification
- [ ] make check clean
- [ ] make build succeeds
- [ ] make test passes
- [ ] <hardware observation, with real masked serial output, or "not run: no board">

Closes #<N>
BODY
)"
```

No generated-with footer.

## Step 9 — Return to main, then report

Issue number, branch, PR number and URL, which gates ran and their **actual**
results, and anything left unverified — particularly anything needing hardware
you did not have. Then hand off to `/merge-pr <PR>`.

> **This repository has no CI.** The Step 6 gates are the only gates. Do not
> claim CI passed.
