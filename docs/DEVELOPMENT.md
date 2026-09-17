# Documentation and Development Conventions

A returning developer should learn what works, why, and the specific next step
without reconstructing old conversations or Git history. Start at root
`STATUS.md`, descend to the component, and follow its active story.

## Document Roles

- `README.md`: purpose, navigation, and build/run instructions where relevant.
- `STATUS.md`: current state, active story, latest meaningful verification or
  result, next action, known issues, and links. Rewrite current facts; do not
  accumulate a diary. Distinguish code inspection from deployment verification.
- `docs/contract.md`: currently implemented observable behavior and limits.
- `docs/decisions.md`: important choices, rationale, and whether proposed,
  accepted, or superseded. Existing architecture documents can carry this role
  where that avoids duplication.
- `docs/stories/NNN-name.md`: bounded intent, scope, acceptance criteria, status,
  and, after human acceptance, a concise result or deviations.

Place each document at the lowest level containing its whole subject. Root
architecture covers shared interfaces and system responsibilities; component
API contracts belong to their owner and consumers link to them. Keep useful
existing names. Add documents only when there is content to maintain, not a
full set of empty templates for future components.

## Story Lifecycle and Definition of Done

Use stories for small increments; a milestone may span several stories.
Suggested states: planned, in progress, ready for human review, complete.
Only the human developer decides completion. Passing checks is evidence for
review, not acceptance. Preserve existing evidence of earlier work without
inventing retrospective stories or acceptance records.

On human acceptance, update the component status, accepted story, implemented
contract if behavior changed, and important decision records if needed. Update
root status only for material project-wide changes. For repository-wide stories,
root status is the relevant status document. Include an actionable next step;
if undecided, identify the decision needed.

Keep completed stories in place with their criteria and brief outcome. Link to
Git for detailed history rather than copying changes or old specifications into
new archives. Planned features must not be presented as implemented contracts.
