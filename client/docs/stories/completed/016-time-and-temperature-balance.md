# Time and Temperature Balance

## Objective

Make a subtle visual-balance adjustment to the top information row: the large
time display currently sits slightly too far left, and the Den and Back Porch
temperature displays appear slightly too low relative to it.

## Requirements

- Shift the large time display slightly right.
- Raise both large temperature displays slightly for more natural alignment.
- Preserve typography, sizing, overall layout structure and existing behavior.
- Keep the adjustment subtle; use only minimal spacing changes.

## Verification and acceptance

Review the top row at 1024 × 600 and a larger viewport. Confirm the intended
small offsets, unchanged typography/sizing, and preserved supporting text,
forecast, background and settings layout. Leave active for human display review
and explicit acceptance.

## Acceptance

Accepted by the developer on November 4, 2026 after visual review, including the
developer's final 5vw clock-group left anchor. The time line has an additional
.75vw rightward nudge, and both temperature value lines are raised 1vh.
Typography, sizing and layout structure are preserved.

Browser comparisons verified the value-line offsets at 1024 × 600 and
1920 × 1280; the final clock-group anchor was adjusted and accepted by the
developer. Diff checks passed. No intentional departures from the requested
scope.
