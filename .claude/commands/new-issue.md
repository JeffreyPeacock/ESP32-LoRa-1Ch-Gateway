# File a new issue and put it on the board

Create a GitHub issue for this repository and add it to the project board.

Parse as much as possible from `$ARGUMENTS` — free-form text is fine. Only ask
for what is genuinely missing.

## Config

Read `.claude/board-config.md` for the repo, project and field IDs, the label
set, the milestones and the priority rubric. **The identifiers there are live.** Repository, project #13, and every field
and option ID are filled in, and both the repository and the project are
public. Do not invent IDs; read them from that file.

## Steps

1. **Confirm the parse in one line** — title, labels, milestone, priority, and a
   sentence of body. If something critical is missing, ask. Otherwise proceed.

2. **Create the issue.** The body says what the task is, why it matters, and how
   you will know it is done. Where the commands are known, put them in a fenced
   block — tickets here should be executable, not merely descriptive.

   This project's value is mostly written findings, so a ticket whose outcome is
   "we now know X" is legitimate. Say what would settle the question.

   ```bash
   gh issue create --repo JeffreyPeacock/ESP32-LoRa-1Ch-Gateway \
     --title "<title>" --body "<body>" \
     --label "<label>[,<label>]" --milestone "<milestone>"
   ```

   **Never paste unmasked serial output into a body.** See the masking note in
   `.claude/board-config.md`.

3. **Add it to the board.**
   ```bash
   gh project item-add 13 --owner JeffreyPeacock --url <issue-url>
   ```

4. **Get the item ID.** Retry once after a few seconds if empty — attachment
   lags briefly.
   ```bash
   gh api graphql -f query='
   { repository(owner:"JeffreyPeacock", name:"ESP32-LoRa-1Ch-Gateway") {
       issue(number: <N>) { projectItems(first:5){ nodes{ id project{ number } } } } } }' \
     --jq '.data.repository.issue.projectItems.nodes[] | select(.project.number==13) | .id'
   ```

5. **Set Status = Backlog, then Priority.** Two mutations, same shape:
   ```bash
   gh api graphql -f query='mutation {
     updateProjectV2ItemFieldValue(input:{
       projectId:"PVT_kwHOAdChXs4Bhr4A" itemId:"<item-id>"
       fieldId:"PVTSSF_lAHOAdChXs4Bhr4AzhgmtoE"
       value:{ singleSelectOptionId:"da2e6d4d" } }){ projectV2Item{ id } } }'
   ```

6. **Report:** issue number and URL, labels, milestone, priority, and that it is
   on the board in Backlog. If you chose a priority the user did not give, say
   so plainly so they can correct it.
