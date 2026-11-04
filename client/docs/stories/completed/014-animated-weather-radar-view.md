# DenClock — Animated Weather Radar View

## Objective

Extend DenClock's forecast display to support a third view showing animated weather radar.

Preserve the existing minimalist appearance, touchscreen interaction, and visibility of the background photograph.

The implementation should work on the current 1024 × 600 touchscreen while remaining responsive for potential deployment on Microsoft Surface Go tablets.

## 1. Forecast View Rotation

The existing forecast area cycles through three modes:

1. 24-hour forecast (existing)
2. 10-day forecast (existing)
3. Animated weather radar (new)

Tapping or clicking anywhere in the forecast area advances to the next mode.

After radar, the next tap returns to the 12-hour forecast.

Preserve the current initial display and existing behavior of the two forecast views.

No additional navigation buttons are needed.

## 2. Radar Data Source

Use the free RainViewer Weather Maps API for historical radar imagery.

Documentation:
[https://www.rainviewer.com/api/weather-maps-api.html](https://www.rainviewer.com/api/weather-maps-api.html)

Metadata endpoint:
[https://api.rainviewer.com/public/weather-maps.json](https://api.rainviewer.com/public/weather-maps.json)

The API provides approximately two hours of historical radar frames at ten-minute intervals.

Use the current API specifications, including its zoom, color scheme, and request-rate restrictions.

Do not depend on nowcast or future radar frames, which are no longer available through the free API.

Retrieve the tile host and frame paths dynamically from the metadata response.

Include the required, unobtrusive RainViewer attribution.

## 3. Radar Presentation

Use Leaflet or an equivalent lightweight mapping solution, preferably reusing any existing application dependencies.

- Center the map on DenClock's configured weather location.
- Use a fixed regional zoom showing approaching weather systems, constrained by the radar provider's supported zoom levels.
- Display an understated dark geographic base map.
- Overlay radar precipitation imagery.
- Use a small, subtle home-location marker.
- Avoid prominent roads, city labels, borders, and unnecessary map decorations.
- Do not display standard zoom controls or mapping toolbars.

The radar should fill the existing forecast rectangle without changing the surrounding layout.

The background image should remain visible wherever practical, using the same overall transparency philosophy as the existing forecast displays.

Radar precipitation must remain sufficiently opaque to distinguish weather patterns.

## 4. Radar Animation

- Animate the available historical frames chronologically.
- Use approximately 400–600 milliseconds per frame.
- After two complete animation cycles, stop on the newest available frame.
- Whenever the user switches back to radar, replay the animation.
- Display a small, unobtrusive frame timestamp.
- Avoid flashing, flickering, or displaying blank frames during transitions.
- Preload or cache frames as appropriate to provide smooth animation.

An absence of precipitation should display a valid, quiet map rather than an error message.

## 5. Data Refresh and Reliability

- Refresh radar metadata approximately every five minutes while the application is running.
- Cache radar frames and map tiles wherever practical.
- Respect RainViewer's published request limits.
- Avoid unnecessary network requests when switching between forecast views.
- Handle network failures, missing frames, and unavailable radar data gracefully.
- Preserve the last valid imagery temporarily if a refresh fails, clearly distinguishing stale data from current imagery.
- Avoid allowing radar failures to disrupt the clock or existing forecasts.

## 6. Responsive Layout

Do not hardcode radar dimensions or positioning for the current 1024 × 600 monitor.

The radar map must adapt to the forecast container.

Ensure that the map redraws correctly when the container changes size or becomes visible after switching forecast modes.

Support the current touchscreen and mouse interaction.

Keep future Surface Go deployment in mind, including higher pixel density and different display aspect ratios.

## 7. Preserve Existing Functionality

Do not redesign the clock, background image, current weather, 24-hour forecast, or 10-day forecast.

Do not introduce additional UI controls beyond the existing tap-to-cycle mechanism.

Ensure that any radar animation timers and map resources are properly cleaned up when appropriate.

## Acceptance Criteria

- DenClock starts with its existing 12-hour forecast.
- First tap shows the 10-day forecast.
- Second tap shows animated weather radar.
- Third tap returns to the 24-hour forecast.
- Radar is centered on the configured location.
- Animation shows actual historical radar data.
- The radar pauses on its most recent frame after two cycles.
- Radar updates automatically without disturbing the clock.
- The display remains visually consistent with DenClock.
- Layout works on the existing touchscreen and scales to larger displays.
- Network or radar service failures do not interrupt normal operation.

## Design Philosophy

DenClock is primarily an attractive clock and weather display, not a conventional weather application.

The radar should be a quiet, useful addition to the existing interface.

Avoid unnecessary controls, borders, labels, and other visual clutter.

**Preserve the photograph-first aesthetic.**

## Clarified hourly range

Preserve the existing 24-hour forecast as the initial view and the view following
radar. References to 12 hours in the supplied request do not change the accepted
hourly range or its appearance.

## Implementation boundaries

Use historical frames only, RainViewer color 2 (Universal Blue), zoom no higher
than 7, and throttled requests below the published 100/IP/minute transition limit.
A fixed, noninteractive canvas tile map is an equivalent lightweight mapping
solution for this tap-to-cycle display; it needs no pan/zoom controls or new
framework. Use the existing browser lifecycle and configured location. Keep the
previous frame until complete replacements are loaded. Human display acceptance
remains required; leave this story active until explicitly accepted.

## Acceptance

Accepted by the developer on November 3, 2026 after display review during a
dry-weather interval. The base-map opacity was increased to 65% following
review. Precipitation appearance during rain has not yet been observed by the
developer; new observations may inform later work.

Implemented the three-view cycle with historical radar, cached two-cycle
playback, automatic refresh, responsive mapping and graceful failure handling.
Existing hourly and daily chart parsing, selection and rendering are unchanged.
Client tests, native-size/high-DPI browser checks, and live loading of thirteen
RainViewer historical frames passed. The fixed canvas map satisfies the
lightweight mapping requirement without a new framework. No intentional
departures from the clarified requirements.
