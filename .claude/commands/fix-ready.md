# Drain the Ready column

Work through every issue in **Ready**, in order, until the column is empty or a
ticket blocks.

## Config

`.claude/board-config.md`. If its identifiers are placeholders, stop and say the
board does not exist yet.

## Step 1 — Enumerate the Ready queue in priority order

The board is small, so one call is complete. Filter and sort locally:

```bash
gh project item-list <N> --owner JeffreyPeacock --format json \
  --jq '.items[] | select(.status=="Ready")
        | [(.priority // "p9"), (.content.number|tostring), .content.title] | @tsv' \
  | LC_ALL=C sort -t$'\t' -k1,1 -k2,2n
```

Use `.content.title`, never `.title` — see the trap note in `/priority-review`.

If empty, report "Ready queue is empty" and stop.

## Step 2 — Order the work, and say so before starting

The ordering constraint here is **physical, not code-level**:

- **One board, one application.** `build_src_filter` compiles exactly one
  `main()`. A ticket needing `probe` and one needing `listen` cannot be
  interleaved — group by application and do all of one before reflashing.
- **Some tickets need the radio listening for a long time.** A noise-floor or
  packet-capture ticket is not a five-minute task; sequence it so it can run
  while you do desk work.
- **Some need the other radio transmitting**, which means the other developer is
  in the loop and the timing is not yours. Those are `interop`.
- **Some are pure desk work** — docs, decisions, `scripts/`, anything under
  `test/`. No board at all. Good filler, and safe to do while a capture runs.

Present the ordered plan with the reason for the order and **pause once for
confirmation.** After approval, run the whole plan without pausing between
tickets.

## Step 3 — For each ticket, in order

1. Move it to **In Progress** — item-ID lookup as in `/fix-ticket` Step 0.
2. Work it. For anything beyond a one-line change follow `/fix-ticket <N>`
   rather than improvising.
3. Gate: `make check`, `make build`, `make test` — whichever apply.
4. Commit, PR, and merge via `/merge-pr`, which moves the ticket to Completed.

Documentation-only tickets may go straight to `main`; say which you chose.

## Step 4 — When a ticket blocks

A ticket is **blocked, not failed**, when it needs something you cannot supply.
Three kinds here, and they are different:

| Blocked on | What to do |
|---|---|
| **A power cycle** — flash failure, or `flash read err` at boot | Ask the user to unplug and replug the USB. This is the GPIO12 strap, not your change. Resume when they confirm. Do not move the ticket |
| **Hardware not attached, or the second radio** | Comment, move back to **Prioritized** — not Backlog, it was Ready for a reason — and continue |
| **A decision only the user can make** | Comment with the options and what each costs, move to Prioritized, continue |

Do not fake progress and do not guess at hardware results.

## Step 5 — Report

Tickets completed (number → PR), tickets blocked (number → what they wait on),
**which application the board is left running**, and the final state of the
Ready column.

State the application explicitly. Leaving the board on `probe` when it started
on `listen`, without saying so, is the most confusing possible outcome of a long
run — the next person sees a radio that hears nothing and assumes it is broken.
