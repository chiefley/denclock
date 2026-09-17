# DenClock Development Workflow

## Purpose

This document is the authoritative enduring description of the DenClock
development workflow. It describes the workflow currently in force; completed
stories record the requested increments that established or changed it.

The repository uses **stories as the primary unit of active development work**.
Stories are work instructions developed by the human developer in collaboration
with ChatGPT and then given to Codex for implementation.

The filesystem makes active and completed work readily apparent without
requiring component `STATUS.md` files to duplicate that information. A returning
developer should be able to recover what works, why, and where to resume without
reconstructing past conversations or Git history.

## Start or Resume

Read the root README and `STATUS.md` for system-wide orientation, then the
relevant component README, contract, decisions, and supplied active story.
Inspect the implementation and follow established conventions. Implement only
the requested story; do not independently begin another pending story.

## Development Workflow

The normal Den Clock development workflow is:

1. The human developer and ChatGPT develop the requirements for a new story.
2. The resulting story text is given to Codex.
3. Codex creates a numbered story Markdown file in the appropriate component's `docs/stories/` directory.
4. Codex implements the story.
5. Codex reports the implementation and testing results to the human developer.
6. The human developer determines whether the story is complete.
7. Codex must not independently declare a story complete.
8. When the human developer approves the story as complete, Codex:
   - updates any enduring documentation affected by the implementation;
   - moves the story into that story directory's `completed/` subdirectory;
   - updates repository-wide status only when the completed work materially changes overall project status.
9. After human approval and completion of the story's documentation updates, Codex stages the story's changes and saves a commit-message file containing the story title without its sequence-number prefix. Codex provides the command for committing the staged changes using that file but does not execute it.
10. The developer reviews the staged changes and executes the commit.

## Story Location

A story belongs at the lowest repository level that completely contains the work.

Examples:

```text
client/docs/stories/
server/docs/stories/
firmware/sensors/docs/stories/
```

A story that affects the repository or system as a whole belongs in:

```text
docs/stories/
```

Each story directory shall contain a:

```text
completed/
```

subdirectory for stories accepted by the human developer.

Do not create story directories speculatively for components that do not yet need them. When a required `completed/` directory is empty, retain it in Git with a `.gitkeep` placeholder.

## Story Numbering

When Codex receives new story instructions, it shall determine the next appropriate story number in that story scope and create the story file.

Use filenames of the form:

```text
NNN-short-description.md
```

For example:

```text
001-time-display.md
002-weather-display.md
```

Inspect both active stories and the `completed/` subdirectory in that scope.
Start at `001` if there are no stories; otherwise use one greater than the highest
existing sequence number. Do not reuse a completed story's number. Numbering is
independent for each story scope.

Story numbering is primarily for convenient identification and ordering. Git remains the authoritative historical record.

## Active and Completed Stories

A story located directly in:

```text
docs/stories/
```

is active or pending work.

A story located in:

```text
docs/stories/completed/
```

has been accepted as complete by the human developer.

The directory structure therefore carries much of the development-status information that would otherwise require manually maintained component status documents.

## Role of Stories

Stories are **prospective work instructions**.

A story should contain enough information for Codex to perform a bounded increment of work without reconstructing requirements from previous conversations.

As appropriate, a story may contain:

- purpose and context;
- requirements;
- constraints;
- relevant existing interfaces or architecture;
- acceptance criteria;
- testing expectations;
- explicit exclusions or deferred work.

A completed story is retained because it records what Codex was asked to accomplish.

Completed stories are historical records and should not be continually revised to describe later system behavior.

## Role of Contracts

Contracts are distinct from stories.

A contract describes **enduring behavior of the currently implemented system or component**.

Stories say:

> Build this.

Contracts say:

> The implemented system behaves this way.

When completion of a story establishes or changes externally observable or architecturally significant behavior, Codex shall update the appropriate contract as part of closing the story.

Do not copy story contents mechanically into a contract. Only enduring implemented behavior belongs in the contract.

Git preserves previous versions of contracts.

## Role of Decision Documentation

Decision documentation records important architectural or design choices and their rationale.

Stories describe requested work.

Contracts describe resulting behavior.

Decision documentation explains why significant choices were made.

When a completed story introduces or changes a durable architectural decision, update the appropriate decision documentation as part of closing the story.

## Status Documentation

Component `STATUS.md` files should not be required merely to duplicate the state already represented by active and completed story directories.

When reviewing or retiring a component `STATUS.md`, preserve information that remains useful by moving it to the appropriate README, contract, decision document, active story, or other enduring documentation. Remove the file only when useful information has been preserved and its remaining purpose is merely to duplicate story workflow state.

Retain a concise root-level `STATUS.md` if it remains useful as a system-wide orientation and context-recovery aid across the independently evolving Den Clock components.

The root status should remain small. It should identify the broad state of major components and point to active work where useful. It should not become a development diary or duplicate detailed component stories.

## Codex Instructions

The repository's Codex/agent instructions must reflect this workflow.

In particular, the instructions must establish that:

- story requirements originate with the human developer;
- Codex creates the numbered story file from the supplied story instructions;
- Codex implements against that story;
- Codex does not independently declare a story complete;
- human approval is required before moving a story to `completed/`;
- story closure includes updating affected contracts and decision documentation when appropriate;
- completed stories are moved rather than copied;
- component status documents are not maintained merely to duplicate story-directory state;
- Git provides version history.

## Review and Story Closeout

Codex reports the implementation and testing results, relevant limitations, and
unresolved decisions to the human developer. A story can be ready for human
review without being complete. Passing checks is evidence for review, not
acceptance. Acceptance of one story does not imply acceptance of another.

Before explicit human approval, leave the story in its active directory and
leave the implementation available for review. Do not stage it as a
completed-story handoff.

After the human approves the story as complete:

1. Update enduring documentation affected by the implementation, including
   appropriate contracts, decision records, and operational guidance. Preserve
   the distinction between implemented behavior, requested work, and rationale.
2. Record human acceptance and a brief result or intentional departures in the
   story. Move it into that scope's `completed/` subdirectory, preserving the
   sequence number and filename. Move rather than copy so it is not also listed
   as active. Repair links to the moved story and relative links inside it.
3. Update root `STATUS.md` only when the completed work materially changes
   overall project status. Do not create component status updates merely to
   duplicate the directory move.
4. Check documentation links and the scope of the changes before preparing the
   commit handoff described below.

Completed stories retain the instructions for the accepted increment. Later
changes belong in later stories and the affected enduring documentation; do not
continually rewrite accepted stories to describe current system behavior.

## Staging and Commit Handoff

After human approval and completion of story closeout, Codex stages the changes belonging to the story while preserving unrelated work. Codex then provides a ready-to-run git commit -m "Story Title" command using the story title without its sequence-number prefix. Codex does not execute the commit. The human developer reviews the staged changes and executes the supplied command.

Preparing the handoff is
not permission for Codex to commit automatically.

## Documentation Placement and History

Keep documentation at the lowest repository level that fully contains its
subject. Root documents cover repository-wide architecture, overall status,
and development conventions. Component READMEs provide purpose, navigation,
run/deployment guidance, and useful operational constraints. Contracts and
decisions remain with their owning component unless they span the system.
Existing architecture documentation may carry decision rationale where that
avoids duplicate records. Distinguish proposed, accepted, and superseded choices.

Add documents when there is useful content, rather than creating empty
hierarchies for hypothetical future components. Distinguish implementation
inspection from actual runtime or deployment verification.

Git provides version history for code and documentation. Story numbering aids
identification and ordering; completed stories preserve requested work;
contracts preserve current implemented behavior. None should become a duplicate
Git history or a chronological development diary.
