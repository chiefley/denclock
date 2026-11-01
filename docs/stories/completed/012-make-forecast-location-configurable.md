# Make the weather forecast location configurable

Add Forecast Location to the existing settings UI, accepting city/state or US ZIP
code. Resolve coordinates and persist with existing presentation settings. Refresh
when changed, restore after restart, and show the location beside the lower-right Open-Meteo credit (as requested in
human review), keeping the forecast heading unobtrusive.
Invalid locations and service failures must preserve the last valid configuration.
Keep forecast location independent of sensor assignments. Preserve Granger Township,
Ohio and its current coordinates as the default for existing installations. Reuse
existing architecture; avoid unrelated UI/CSS changes. Test changes, persistence,
and forecast retrieval.

## Acceptance

Accepted by the human developer on 2026-10-31 after functional review.
Implemented with existing presentation persistence, browser geocoding and forecast
validation, automatic refresh, and the legacy default coordinates. Human review
changed location placement from the heading to the lower-right weather credit.
Client and server automated checks passed, including persistence across restart,
invalid settings, service failures, and in-flight location changes. Live city/state
and ZIP lookup and forecast retrieval were verified. Open-Meteo resolves the
default area using “Granger, OH” rather than “Granger Township, OH”.
