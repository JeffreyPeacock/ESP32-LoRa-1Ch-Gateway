# Prepare this session's work for compaction

Fold what was learned into the documents before the context that produced it is
gone. Follow the steps in order.

The risk this addresses is specific: on this project the valuable output is
almost always a **finding** rather than a code change — a pin map that matches no
stock target, a stub loader that fails over this particular bridge, a banner that
prints credentials. Those live only in the conversation until someone writes them
down.

## Step 1 — What actually happened

```bash
git log --oneline -20
```

**There is no issue tracker for this project.** If one is created, add the
`gh issue list` step here and restore the board commands from the sibling
Heltec project.

Separate three things, because they are documented differently:

- **Findings** — something is true that we did not know. Goes in `CLAUDE.md`.
- **State changes** — the board's firmware or stored config changed. Goes in
  README under "Current state".
- **Narrative** — how we got there. Belongs in commit messages and issue
  comments, not in living documents.

## Step 2 — Update CLAUDE.md with findings only

Targeted edits. Do not rewrite sections that did not change.

**Look hardest for statements that are now wrong.** A stale claim is worse than
no claim, because a later session will act on it. The likeliest here are the pin
map, the flash size that dates the board revision, and any assertion about what
firmware is loaded. Correcting a stale claim outranks adding a new one.

Watch the size — 40k is the limit, and this file grows quickly. Use
`/trim-claude-md` if it is over 30k.

## Step 3 — Update README "Current state"

If the session reflashed the board or changed its stored configuration, that
section is now lying. The device is authoritative — read it rather than
reconstructing from memory:

```bash
esptool --port /dev/ttyUSB1 --no-stub chip-id
python -m serial.tools.miniterm /dev/ttyUSB1 115200   # tap RST, read, Ctrl-]
```

Mask the banner before quoting it; it prints WiFi credentials.

## Step 4 — Regenerate what is generated

Nothing is generated in this project yet. Skip.

## Step 5 — Check nothing sensitive is about to be committed

The repository is public and this project has already had a WiFi PSK and a
channel URL land in a session log.

```bash
git grep -niE 'psk|password|SSID|PASS=|BEGIN .*PRIVATE KEY' -- . | grep -v '<redacted>'
git status --short --untracked-files=all
```

This board **prints WiFi credentials on every boot**, so a pasted banner is the
likely leak here. Confirm no credentials, and that `docs/datasheets/`,
`etc/firmware/`, `etc/secrets/` and `.claude_artifacts/` are still ignored.

## Step 6 — Optionally export the transcript

If the session found things worth keeping verbatim:

```bash
./scripts/export-transcript.sh
```

Masking is on by default and it verifies its own output.

## Step 7 — Commit on main

```bash
git branch --show-current      # must be main
```

There is no remote yet, so commit locally and say so rather than reporting a
push that did not happen.

Stage only what this command changed. `Docs:` prefix. **No AI attribution.**

Write the commit message to explain *why* something is true, not just that it
changed — the commit log is where the narrative belongs, and it is the only place
a future reader can recover the reasoning.

## Step 8 — Report

What was updated, what was deliberately left alone, and — most importantly — any
**stale claim you corrected**, since that is the thing most likely to have misled
someone later.
