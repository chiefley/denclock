# DC-BG-001 — Time-of-Day Lodge Background Selection

Type: User Story

Priority: Medium

Component: DenClock Client Background Rendering

## User story

As a DenClock user, I want the lodge background to change with time of day,
remaining quiet and connected to real-world daylight.

## Requirements

- Select day, original master twilight, or night using synchronized local time
  and sunrise/sunset from existing weather data where practical, with a safe fallback.
- Preload destination images and transition in approximately one second without
  blank frames, flicker, or conspicuous ghosting.
- Preserve clock, weather, radar, and existing background scaling/alignment.
- Provide a clearly identified temporary GUI directly on the display offering
  Auto, Day, Twilight, Night, usable by touch/mouse on the Surface Go.
- Visibly indicate selection; apply it immediately using the normal transition.
  Auto resumes automatic selection. Overrides live only in memory until reload.
- Isolate the temporary GUI so its removal does not affect automatic operation.
- Evaluate on startup, solar-data changes, and periodically; avoid reloading or
  transitioning an unchanged scene.
- Do not add compositing, masks, weather-driven switching, dependencies, seasonal
  selection, or additional scenes.

## Acceptance and verification

Verify all three images, all four GUI modes, startup, solar selection, transitions,
missing-data fallback, and unchanged existing functionality. Include automated
selection tests and report files, rules, checks, and limitations for human review.

## Future work

After visual evaluation, remove the temporary GUI. Weather-driven selection,
seasonal variations, and library expansion remain future stories.

## Human acceptance

Accepted by the developer on November 6, 2026. Automatic selection uses the
configured forecast location's sunrise/sunset with 30-minute twilight windows
on either side; synchronized local-time windows provide the fallback. Decoded
images fade over one second, and the isolated temporary selector provides
session-only overrides. All seven client test files and whitespace checks passed,
including startup, solar boundaries, fallback, GUI modes, preload failures,
rapid selections, and existing functionality. Codex did not perform physical
touchscreen or browser animation verification. No intentional departures.
