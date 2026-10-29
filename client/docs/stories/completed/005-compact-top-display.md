# Compact Top Display

Arrange the clock and two existing sensor readings in the top approximately 230 pixels of the 1024 x 600 display. Retain the cozy lodge background. Reduce and condense the time typography, keeping seconds subordinate. Each sensor has two lines: large temperature, then configured label and humidity (Indoor/Outdoor · XX% RH). Keep configurable labels, units, unknown states, stale colors, polling, synchronization and settings intact. Use transparent panels and readable text shadows. Leave the lower area empty; do not implement forecasts.

Verify client tests and preview at 1024 x 600 with fresh, unknown, stale and extreme readings. Human review on the physical display remains required before acceptance.

## Review adjustment

Enlarge display typography as much as fits within the existing top layout,
rather than applying a literal 3× increase. Rebalance margins and available
column widths while keeping seconds and supporting details subordinate.
Keep the settings dialog typography unchanged.

## Acceptance

Accepted by the developer. The compact top layout retains existing clock, sensor,
and settings behavior. Final developer-adjusted main time and whole-temperature
font sizes are `min(15vw, 15vw)`. Client automated tests pass; Codex did not
independently verify the live browser or physical-display fit.
