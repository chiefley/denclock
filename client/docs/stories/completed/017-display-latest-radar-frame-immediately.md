# DC-RADAR — Display Latest Radar Frame Immediately

**Type:** User Story
**Priority:** Medium
**Component:** DenClock Client — Weather Radar
**Status:** Accepted November 6, 2026

## User Story

As a DenClock user, when I tap the forecast area to display weather radar, I want to see the most recent available radar image immediately, rather than waiting for the entire historical animation sequence to download.

Once the historical frames have loaded, I want the existing radar animation to play normally.

## Background

Currently, entering the radar display causes DenClock to retrieve historical radar images before displaying anything.

On the Surface Go 3, this can take several minutes. The delay occurs every time radar is opened.

This makes the radar display appear unresponsive, even though the most recent image is the one the user is initially most interested in.

## Required Behavior

### 1. Immediate display

When the user opens radar:

- Open the radar view immediately.
- Prioritize loading the latest available radar frame.
- Display that frame as soon as it is ready, without waiting for historical frames.
- If the latest frame is not yet available, retain an appropriate loading indication.

### 2. Historical frame loading

After initiating the latest-frame request:

- Load the historical frames asynchronously.
- Keep the latest image visible throughout loading.
- Do not block user interaction.
- Do not start animation until the required frames are ready.
- Ensure frames are ordered chronologically, oldest to newest.

### 3. Animation

When the historical sequence is ready:

- Begin the existing animation automatically.
- Preserve existing frame timing, looping behavior, and number of repetitions.
- Preserve existing controls and radar presentation.
- Avoid flicker or blank frames when switching from the static image to animation.

### 4. Error handling

- If one or more historical frames fail, retain the latest available radar image.
- Do not leave the radar display blank because historical loading fails.
- If the latest frame fails, provide the existing appropriate error indication.
- Prevent stale asynchronous requests from updating the display after the radar view has been closed.

### 5. Caching and repeated openings

Investigate whether the current implementation unnecessarily downloads the same frames whenever radar is reopened.

Reuse already loaded frames where practical, subject to freshness validation.

Caching must not cause outdated radar imagery to be presented as current without an appropriate indication.

## Implementation Guidance

Before changing code, inspect the current radar implementation to establish:

- The radar imagery provider and its API.
- How historical frame URLs are obtained.
- Whether individual frames can be fetched independently.
- How image loading and decoding are handled.
- How animation and repeated openings currently work.

Prefer the smallest change that achieves the required behavior.

Do not redesign the radar subsystem or introduce additional dependencies unless necessary.

The implementation should distinguish between:

1. Latest frame available.
2. Historical frames loading.
3. Animation ready.
4. Animation playing.
5. Loading failed.

These states need not correspond to separate UI components.

## Acceptance Criteria

- [x] Opening radar starts loading the most recent available image without waiting for historical frames.
- [x] The latest frame becomes visible as soon as it is ready.
- [x] Historical frames continue loading without blocking the display.
- [x] Animation starts automatically when the required frames are ready.
- [x] Existing animation timing and repetition are preserved.
- [x] Historical loading failures do not remove an already displayed latest frame.
- [x] Closing and reopening radar does not produce stale asynchronous updates.
- [x] Repeated openings do not unnecessarily reload usable cached frames.
- [x] Existing forecast display and navigation remain unchanged.

## Testing

Verify behavior under:

1. Normal network conditions.
2. Slow historical frame downloads.
3. Individual frame download failures.
4. Closing radar during loading.
5. Rapidly opening and closing radar.
6. Reopening radar after its data becomes stale.

Include automated tests where practical, particularly for asynchronous loading, frame ordering, and stale-request handling.

## Out of Scope

- Changing radar providers.
- Changing geographic coverage or zoom.
- Redesigning the radar interface.
- Changing animation duration or playback speed.
- Adding new radar controls.

## Completion Requirements

Codex should provide:

- Summary of changes.
- Files modified.
- Tests performed and results.
- Any remaining limitations.
- Confirmation that the existing radar animation behavior is preserved.

Update the DenClock development record with the implementation decision and test results.

## Acceptance and verification

Accepted by the developer on November 6, 2026. The newest complete radar frame
loads before geography and history; historical failures retain that static image.
Caching and freshness checks remain in place. Successful animation retains two
chronological passes at 500 ms per frame. No intentional departures.

`npm test --prefix client` passed all six test files, including delayed history,
frame failures, chronological readiness, cached replay, expiry and late responses
after close/reopen. `git diff --check` passed. Physical Surface Go and live-network
verification were not performed by Codex.
