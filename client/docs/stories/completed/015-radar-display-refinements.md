# Radar Display Refinements

## Objective

Make radar an intentional exception to the photograph-first presentation: improve
storm readability, add cached replay, and expose radar timestamp and age.

## Requirements

- Render the radar map at 100% opacity, completely covering the photograph within
  the existing radar rectangle. Preserve forecast-container size/position, subdued
  geography, clear precipitation colors and both existing forecast appearances.
- Add a small corner replay button with a recognizable circular arrow and at least
  a 48px touch target. It stays visible when paused. Pressing it restarts the
  oldest available cached frame, plays twice and pauses on newest, without
  advancing the forecast view or needlessly requesting data. Automatic replay on
  entering radar continues.
- Keep 24-hour → ten-day → radar → 24-hour rotation and existing refresh/recovery.
- Show the newest available radar frame's timestamp and age in minutes, for
  example `Radar 11:05 AM · 8 min old`. Age must advance at least every minute
  while paused, using the frame timestamp rather than download time. Keep it
  visible when paused and identify unusually old imagery (20 minutes) with a
  subtle but noticeable stale warning.
- Full opacity and replay are deliberate exceptions to otherwise minimalist UI.

## Provider timestamp constraint

RainViewer supplies the composite frame-generation timestamp, not an exact shared
observation time for the different radar sources. Use this actual frame timestamp
and describe the limitation in operational documentation; never imply it is the
metadata download time or synthesize an unavailable observation timestamp.

## Verification and acceptance

Test replay during/after animation, two chronological passes/newest hold, no
parent cycling or extra requests, hidden/unavailable/expired states and cleanup.
Test minute rollover, stale age, metadata refresh without newer data, new frames,
and timezone formatting. Check opaque coverage, button hit size and overlays at
1024 × 600 and a larger high-DPI viewport; compare both forecasts unchanged.
Leave active for developer display review and explicit acceptance.

## Acceptance

Accepted by the developer on November 4, 2026 after display review. Implemented
the opaque radar surface, cached replay control, newest-frame timestamp/age and
stale warning while preserving both forecast charts.

Client tests, native-size/high-DPI browser checks and a live-data preview passed.
Verification covered two-pass replay without cycling or extra requests, paused
age updates, stale indication and cleanup. The documented provider limitation
remains: RainViewer supplies composite frame-generation time rather than an exact
shared observation time. No intentional departures from the clarified scope.
