# Lower Window Button Below Toolbar

## Requirement

Move the client window button down so the Linux toolbar at the top of the
screen does not occlude it. Make no unrelated changes.

## Acceptance criteria

- The window button remains accessible when the top toolbar is visible.
- Preserve the button's size, appearance, and restore/fullscreen behavior.
- Keep its associated status message below it without overlap.
- Leave other controls and display layout unchanged.

## Verification

Check the CSS change and existing window-control tests. Confirm toolbar clearance
on the deployed Linux display during human review.

## Acceptance

Accepted by the developer on November 6, 2026. The button and its associated
status message were lowered by 48 CSS pixels, preserving size and behavior.
Existing window-control tests and whitespace checks passed. No intentional
departures from the requirements.
