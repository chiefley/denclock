# 004 — Add Gear-Triggered Display Settings UI

## Purpose

Establish a touchscreen settings interface in the plain HTML/CSS/JavaScript
client without changing configuration or adding settings APIs.

## Requirements

- Add a visible gear with an accessible Settings label in the lower-left corner,
  without a visible button rectangle or obscuring time or readings. Keep it
  unobtrusive. Open a full-screen settings view.
- Support the 1024×600 NUC touchscreen and LAN browsers, large touch targets,
  and scrolling in smaller windows.
- Include Celsius/Fahrenheit, upper-left/right sensor assignment (including
  Unassigned), and each position's display label and stale limit in seconds.
- Explicitly label the controls as a preview using the current example in
  server/docs/presentation-configuration.md, not values read from the service.
- Allow temporary local edits only while open; discard them when closing.
- Disable Save and visibly explain: “Saving settings will be available in a later update.”
- Cancel and Escape return to the clock and restore focus to the gear.
- Support keyboard navigation, visible focus, and associated field labels.
- Continue time synchronization and presentation polling while open; show
  current time/readings on return without changing the actual presentation.

## Boundaries

No configuration endpoints, file writes, browser storage, live presentation
changes, service restarts, or MQTT changes. No LoRa mapping, broker, gateway,
Windows time, or service-management settings. Live configuration loading,
validation, persistence, and application belong to the subsequent story.

## Acceptance and Verification

Verify opening/cancel/Escape, touch sizing and scrolling, preview/Save explanation,
edit discard, and isolation from live time/sensor rendering. Run existing client
checks and report browser or physical touchscreen checks that cannot be performed.
Leave this story active and changes uncommitted for human review; acceptance
and moving to completed require explicit human approval.


## Accepted Result

Accepted by the human developer on October 26, 2026. Implemented the full-screen
example settings preview, lower-left borderless gear, temporary form edits,
and Cancel/Escape focus restoration without changing live presentation.
Existing client checks, JavaScript syntax, and diff checks passed. Codex did
not perform browser, LAN, or physical touchscreen verification. No intentional
departures from the requested scope.
