# Story: Establish Documentation and Context Recovery

Status: Complete; human acceptance and prior commit confirmed in the supplied
story-driven workflow instructions.

## Intent and Scope

Reconcile existing documentation so human and Codex can resume after long
interruptions. Documentation only; no application, configuration, deployment
script, or firmware behavior changes.

## Acceptance Criteria

- Root navigation leads to current status and component documentation.
- Status, implemented contracts, design rationale, and planned stories are distinct.
- Existing client requirements survive under the client as planned work.
- Server documentation reflects source and exposes deployment uncertainties.
- Codex instructions reserve story completion for the human and make relevant
  status/contract/decision updates part of closure.
- Useful hardware and deployment material is retained; no speculative firmware
  documentation hierarchy or retrospective story history is introduced.
- Internal Markdown links resolve and the diff contains documentation only.

## Review Notes

The client specification moved from root into its first story. The server API
contract was extracted from its README. Existing run/service commands remain.
See root status for unresolved choices. No runtime or device verification is
claimed by this documentation story. Human acceptance is confirmed.
