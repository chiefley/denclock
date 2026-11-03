# Toggle clock fullscreen

## Request
Provide a small upper-right control, using a single rectangle in windowed mode
and overlapping rectangles in fullscreen mode. Touch or remote mouse use must
restore access to the desktop and return to the fullscreen clock without a keyboard.

## Requirements
- Preserve clock, sensor, forecast, and settings layout and behavior.
- Reflect actual window state, including changes outside the button.
- Replace locked Chromium kiosk launch with a fullscreen app window on XFCE/X11.
- Keep desktop control local to the tablet and restricted to its clock window.
- Document installation and verification; preserve reboot startup and relaunch.
- Leave active for human verification on the tablet.

## Acceptance

Accepted by the developer on November 3, 2026. Implemented the upper-right
restore/fullscreen control and the local XFCE/X11 helper and app-window launcher,
with deployment guidance and refresh support. Client tests and all four desktop
helper tests pass. Physical-tablet acceptance is provided by the developer;
Codex did not independently verify the tablet. No intentional departures.
