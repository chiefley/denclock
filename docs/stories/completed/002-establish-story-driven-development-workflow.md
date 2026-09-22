# Establish Story-Driven Development Workflow

Status: Complete; accepted by the human developer, as recorded in [project status](../../../STATUS.md).

## Purpose

Refine the Den Clock repository documentation conventions established by the context-recovery work.

The repository will use **stories as the primary unit of active development work**. Stories are work instructions developed by the human developer in collaboration with ChatGPT and then given to Codex for implementation.

The filesystem should make active and completed work readily apparent without requiring component `STATUS.md` files to duplicate that information.

This is a repository-process change only. Do not change application or firmware behavior as part of this story.

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

Do not create story directories speculatively for components that do not yet need them.

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

Review the existing component `STATUS.md` files. Preserve information that remains useful by moving it to the appropriate README, contract, decision document, active story, or other enduring documentation. Remove component `STATUS.md` files when their remaining purpose is only to duplicate story workflow state.

Retain a concise root-level `STATUS.md` if it remains useful as a system-wide orientation and context-recovery aid across the independently evolving Den Clock components.

The root status should remain small. It should identify the broad state of major components and point to active work where useful. It should not become a development diary or duplicate detailed component stories.

## Codex Instructions

Update the repository's Codex/agent instructions to reflect this workflow.

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

## Reconcile Existing Documentation

Apply the new convention to the documentation already present in the repository.

In particular:

1. Review the existing repository-wide context-recovery story.
2. Review the existing client time-display story.
3. Review existing component `STATUS.md` files.
4. Preserve useful information in the appropriate enduring documents.
5. Establish the necessary `completed/` directories.
6. Move already human-accepted stories to `completed/`.
7. Leave unfinished stories active.
8. Update repository-level development/process documentation to describe the new workflow.
9. Remove obsolete or duplicative documentation only after preserving any useful information it contains.

The previously completed context-recovery/documentation restructuring work has been reviewed, accepted by the human developer, and committed. It may therefore be treated as a completed story.

The Time Display story has **not** yet been completed and must remain active.

## Acceptance Criteria

This story is ready for human review when:

- the repository documentation describes the story-driven workflow defined above;
- Codex/agent instructions implement the workflow;
- story directories distinguish active work from human-approved completed work;
- the completed context-recovery story is archived appropriately;
- the Time Display story remains active;
- component `STATUS.md` files that merely duplicate story state have been reconciled and removed;
- useful information from removed status documents has not been lost;
- contracts remain descriptions of current implemented behavior rather than story histories;
- decision documentation remains focused on durable architectural rationale;
- root-level status remains concise and useful for returning to the overall Den Clock project after an extended absence;
- no application or firmware behavior has been changed.

After carrying out the work, summarize the documentation changes and identify any ambiguity that requires a human decision.

Do not mark this story complete or move it to `completed/` until the human developer explicitly approves it.

Update `docs/DEVELOPMENT.md` so that it becomes the authoritative enduring description of the Den Clock development workflow defined by this story. Do not merely refer readers to this story. The completed story records the change that established the workflow; `DEVELOPMENT.md` describes the workflow currently in force.

## Acceptance and Verification

Accepted by the human developer, as recorded in [project status](../../../STATUS.md).
The enduring workflow and agent instructions were established, useful component
status information was preserved in READMEs and architecture documentation, and
duplicate component STATUS files were removed.

The implementation report recorded successful checks of internal Markdown links,
whitespace, documentation-only scope, and preservation of Time Display requirements.
Runtime tests were not needed for this documentation change.

The requirements above preserve the original story. The
[current development workflow](../../DEVELOPMENT.md) governs subsequent work,
including the commit handoff.
