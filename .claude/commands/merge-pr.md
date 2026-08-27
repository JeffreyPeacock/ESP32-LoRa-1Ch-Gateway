# Merge a PR and close out its tickets

Verify, squash-merge, and do the board housekeeping.

## Config

`.claude/board-config.md`. If its identifiers are placeholders, merge normally
and report that the board step was skipped.

## Determine the PR

Use the number in `$ARGUMENTS`, otherwise infer from the current branch:

```bash
gh pr view --json number,title,body,statusCheckRollup,closingIssuesReferences
```

If no PR is found, report and stop.

## Step 1 — Verify, by running the gates yourself

**This repository has no CI.** `statusCheckRollup` will normally be empty, and
an empty rollup is **not** evidence of anything. Never report "all checks
passed" when there were no checks.

Do not assume the authoring session ran them:

```bash
git fetch origin && git checkout <branch> && git pull --ff-only
make check     # no output = clean
make build     # all environments
make test      # host-side unit tests
```

If the PR touches the radio driver, `lib/loramath/`, or anything that computes a
register value, **`make test` is not optional** — that is the class of error
this project has actually shipped.

If the PR changes what runs on the board and the board is attached, flash and
read real serial output. If it is not attached, say the hardware claim is
unverified rather than merging on faith.

If any gate fails, report and stop. Do not merge. If a check *is* present and
pending or failed, report and stop.

## Step 2 — Squash and merge

```bash
gh pr merge <PR> --repo <repo> --squash --delete-branch
```

If the merge fails, report and stop — do not continue to housekeeping.

## Step 3 — Update local main

```bash
git checkout main && git pull --ff-only
```

## Step 4 — Identify the closed issues

Read `closingIssuesReferences` from the PR JSON, and parse title and body for
`closes` / `fixes` / `resolves` followed by `#<number>`. Take the unique set.

## Step 5 — Move each to Completed

```bash
ITEM=$(gh api graphql -f query='
{ repository(owner:"JeffreyPeacock", name:"<repo-name>") {
    issue(number:<N>){ projectItems(first:5){ nodes{ id project{ number } } } } } }' \
  --jq '.data.repository.issue.projectItems.nodes[] | select(.project.number==<N>) | .id')

gh api graphql -f query='mutation {
  updateProjectV2ItemFieldValue(input:{
    projectId:"<project-id>" itemId:"'"$ITEM"'"
    fieldId:"<status-field-id>"
    value:{ singleSelectOptionId:"<completed-id>" } }){ projectV2Item{ id } } }'
```

If an issue is not on the board, note it and continue.

## Step 6 — Fold durable facts back into the documents

Only if the merge changed something a future session would otherwise get wrong:
a pin definition, a radio setting, a default frequency, the toolchain layout, a
shell convention, or a hardware finding.

**A stale claim is worse than no claim**, because the next session acts on it.
If the merge invalidated a sentence in `CLAUDE.md`, `README.md` or `docs/`, fix
that sentence — correcting a wrong statement outranks adding a new one.

If the board's firmware or stored configuration changed, README "Current state"
is now lying. Read the device rather than reconstructing from memory.

Commit any such change separately on `main` with a `Docs:` prefix. No AI
attribution.

## Step 7 — Report

PR merged, issues moved to Completed, branch deleted, which gates you ran and
their **actual** output, and any documentation update made.

State plainly that there was no CI if there was none.
