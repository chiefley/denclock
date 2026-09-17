# DenClock Development Workflow

This document is the authoritative enduring description of the development
workflow. Stories are the primary unit of active work. The human developer
works with ChatGPT to define requirements and supplies the resulting story to
Codex for implementation.

## Start or Resume

Read root `STATUS.md` for overall orientation, then the relevant component
README, contract, decisions, and supplied active story. Stories directly in a
`docs/stories/` directory are active or pending; files in its `completed/`
subdirectory have been accepted by the human. Do not begin another pending
story unless the human requests it.

## Create and Implement a Story

1. Determine the lowest repository scope that completely contains the supplied
   work. Use `client/docs/stories/`, `server/docs/stories/`, or
   `firmware/sensors/docs/stories/` for component work, and root `docs/stories/`
   for repository-wide work. Do not create story scopes speculatively.
2. Inspect both active and completed stories in that scope. Use one greater
   than the highest existing sequence number (start at 001); do not reuse a
   completed story's number. Numbering is independent per scope.
3. Create `NNN-short-description.md` from the human-supplied requirements before
   implementation. Preserve purpose, requirements, constraints, acceptance
   criteria, testing expectations, and exclusions as applicable. Stories must
   be usable without reconstructing prior conversations.
4. Ensure that story scope has a `completed/` subdirectory. Use `.gitkeep` when
   empty so Git retains it; do not populate it with unaccepted stories.
5. Implement only the requested story. Report changes, verification results,
   limitations, and unresolved decisions. Mark it ready for human review when
   appropriate; passing checks does not constitute acceptance.

## Human Acceptance and Closure

Only the human developer decides whether a story is complete. Codex must not
independently declare completion or move a story to `completed/`. Acceptance of
one story does not imply acceptance of another.

After explicit human approval:

1. Update affected enduring documentation. Contracts describe currently
   implemented, externally observable or architecturally significant behavior;
   decisions describe durable choices and their rationale. Extract enduring
   facts rather than copying story text mechanically.
2. Record acceptance and any brief outcome or intentional departures in the
   story, then **move**, never copy, it into that scope's `completed/` directory.
   Preserve its number and filename. Repair links to and within the moved file.
3. Update root `STATUS.md` only if the work materially changes overall project
   status. Keep it concise and useful across independently evolving components.
4. Verify documentation links and the change scope. After human approval and
   completion of the story's documentation updates, stage the story's changes,
   preserving unrelated work, and save a commit-message file containing the
   story title without its sequence-number prefix. Provide the command for
   committing the staged changes using that file, but do not execute it.
5. The developer reviews the staged changes and executes the commit.

Before acceptance, leave the story active and the implementation available for
review; do not stage it as a completed-story handoff.

## Enduring Documents and History

- `README.md`: purpose, navigation, operational guidance, and useful component
  facts or unresolved setup constraints.
- `docs/contract.md`: enduring behavior of the currently implemented component,
  not planned requirements or story history. Git preserves prior versions.
- `docs/decisions.md` or an existing architecture document: significant design
  choices, rationale, and proposed/accepted/superseded distinctions.
- Root `STATUS.md`: broad system state and pointers to active work. It is a
  context-recovery aid, not a diary or detailed story index.
- Stories: prospective work instructions. Completed stories retain what was
  requested and are not continually rewritten to match later behavior.

Keep documents at the lowest level containing their subject. Component status
files are not required merely to duplicate story-directory state. Before
removing one, preserve useful information in a README, contract, decision
record, or appropriate story. Avoid empty document hierarchies for future work.
Git is the authoritative version history; do not duplicate it in diaries or
retrospective stories. Distinguish source inspection from deployment verification.
