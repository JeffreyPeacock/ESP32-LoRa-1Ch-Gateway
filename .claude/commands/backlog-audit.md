# Audit the backlog — the board is the source of truth

Groom the open issues: check priorities against the rubric, catch tickets
already done, promote what is genuinely next, and flag hygiene problems.

**This command never edits `CLAUDE.md` or `README.md`.** Ticket state does not
live in documents.

## Config

`.claude/board-config.md` — labels, milestones, rubric, field IDs. If its
identifiers are placeholders, stop and say the board does not exist yet.

## Step 1 — Pull the board

One call gets everything.

```bash
gh project item-list <N> --owner JeffreyPeacock --format json \
  --jq '.items[] | {n: .content.number, title: .content.title, status, priority,
                    labels: .labels, milestone: .milestone}'
```

Cross-reference repo state where needed:

```bash
gh issue list --repo <repo> --state open --limit 100 \
  --json number,title,labels,milestone,createdAt
```

## Step 2 — Checks, all read-only

**1. Priority set and defensible.** Every open item has exactly one. Flag
anything unset — that is a triage failure, not an implied p3. Flag p1 inflation
specifically: p1 means "the next step cannot happen until it is done", which on
this project is a very short list.

**2. Milestone correct.** `Solo bench` = verifiable with this board alone.
`Two-radio link` = needs the SAMD21 Pro RF or its developer. The test is
**verifiability, not topic**, and this is the classification most likely to be
wrong because it is tempting to file by subject matter. Writing up how to
measure SNR is solo. Taking a reading that needs the other radio transmitting is
not.

**3. Already done.** This project does a great deal of work interactively, so a
ticket can be genuinely finished with **no commit to prove it** — a measurement
taken, a question settled, a document read. Check both:

```bash
gh pr list --repo <repo> --state merged --limit 20 \
  --json number,title,closingIssuesReferences
git log --oneline -30
```

Then check whether the answer already sits in `CLAUDE.md` or `docs/`. Flag for a
human glance; **never auto-close.**

**4. Labels present.** Every open issue carries at least one topical label. Flag
bare issues.

**5. Blocked work parked correctly.** Anything labelled `interop` sitting in
**Ready** is probably mis-staged — it cannot be worked solo. Flag for demotion to
Prioritized.

**6. Stale assumptions.** This is the check that matters most here, more than
duplicate-detection. The project's established facts have moved repeatedly: the
SPI bus was recorded wrong for months, the band-plan channel number was wrong in
three documents, and the board no longer runs the firmware the early tickets
assume. **Flag any ticket whose body rests on a superseded fact, and say what
changed and where the correct version is.**

## Step 3 — Present findings and confirm, one pause

Report grouped by check, each with issue numbers and the proposed action. **List
the exact board mutations** Step 4 would make. Pause for confirmation before
mutating anything.

Read-only flags needing a human decision — close candidates, priority disputes,
stale assumptions — are reported, never auto-applied.

## Step 4 — Apply, only after confirmation

Item-ID lookup per issue, then mutate, as in `/fix-ticket` Step 0. Labels and
milestones via `gh issue edit`. **Never close an issue automatically.**

## Step 5 — Report

Issues audited, flags by check with numbers, mutations applied, and what is left
for a human decision. The board — not any document — is left as the accurate
picture.
