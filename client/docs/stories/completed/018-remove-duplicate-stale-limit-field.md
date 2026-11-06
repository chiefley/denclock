# Remove Duplicate Stale Limit Field

Type: Bug Fix

Component: DenClock Client Configuration

Priority: Low

## Problem

Stale Limit appears twice for assigned sensors. It should appear only in
Sensor Mappings.

## Requirements

- Remove the duplicate field outside Sensor Mappings, retaining the mapping field.
- Preserve `maxAgeSeconds`, existing values, validation, seconds units, and persistence.
- Do not change stale-sensor detection, unrelated fields, or page structure.
- Inspect both renderings and confirm their shared configuration binding before editing.
- Make the smallest practical change.

## Acceptance criteria

- Every configured sensor has exactly one visible Stale Limit field, under Sensor Mappings.
- Existing values remain unchanged; editing, saving, and reloading restore the saved value.
- Sensor staleness detection continues as before.
- No unrelated configuration fields or layouts change.

## Verification

Report files changed and checks performed, including persistence and stale behavior.
Leave the story active pending explicit human acceptance.

## Acceptance

Accepted by the developer on November 6, 2026. Removed the duplicate primary-position
controls; Sensor Mappings retains the existing `maxAgeSeconds` editor and validation.
All client automated checks passed, including save/reload regression coverage and
existing sensor-staleness checks. Persistence and staleness implementations are
unchanged. No intentional departures from the requirements.
