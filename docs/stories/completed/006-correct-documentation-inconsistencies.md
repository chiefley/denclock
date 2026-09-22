# Correct Documentation Inconsistencies

## Purpose

Correct the obsolete gateway scaffold reference, the completed workflow story's
pending status, and conflicting commit-handoff instructions.

## Requirements

- Remove the obsolete gateway scaffold entry from README while retaining the
  link to the implemented gateway under firmware/sensors.
- Correct the completed workflow story's status and closeout note to agree with
  its completed location and root STATUS acceptance record. Preserve its original
  requirements as historical instructions.
- Align the development workflow summary with Staging and Commit Handoff:
  after acceptance, stage the story changes and provide a commit command using
  the unnumbered title; the developer commits. No message file is required.
- Change documentation only.

## Acceptance Criteria

- README no longer describes the implemented gateway as a future scaffold.
- The completed workflow story accurately records its accepted status.
- The enduring workflow describes one consistent commit handoff.
- Changed documentation links and diff whitespace checks pass.

## Acceptance and Verification

Accepted by the human developer on 2026-09-22.

Removed the obsolete gateway scaffold reference, corrected the earlier workflow
story's acceptance record, and aligned the enduring commit-handoff instructions.
No departures from the requested scope were required.

Changed documentation links and diff whitespace checks passed. This was a
documentation-only change; no runtime tests were needed.
