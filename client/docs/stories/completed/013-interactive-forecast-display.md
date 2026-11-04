# Interactive Forecast Display

## Objective

Tap or click the forecast area to toggle the existing hourly forecast and a
new ten-day forecast, keeping the transparent, minimalist appearance and current
position and dimensions at 1024 × 600. The hourly view remains the initial view.
Preserve the existing 24-hour forecast range, chart appearance and behavior.
The developer clarified that the request’s references to 12 hours mean the
existing 24-hour forecast; changing its range is outside this story.

## Requirements

- The whole forecast area toggles immediately with touch or mouse. Add no visible
  buttons, tabs or navigation. Avoid text selection and unwanted touch behavior.
- Show ten consecutive calendar days beginning today in the forecast location's
  timezone, using synchronized time. Include abbreviated weekdays, highs, lows,
  and precipitation probability when available. A condition indicator is optional
  only if it fits without clutter. Visual simplicity takes priority over density.
- Reuse SVG charting, typography, colors and transparency. Show restrained high
  and low temperature curves, weekday labels and small legible temperature labels.
  No vertical temperature axis, gridlines or extra decorations in the daily view.
  Scale to the available height with padding so labels do not clip.
- Preserve the hourly display and automatic refresh behavior. Reuse the existing
  weather infrastructure; obtain daily data as needed and cache both views so
  taps do not fetch. Do not synthesize missing weather data; gaps or unavailable
  data must not break either display.
- Keep changes simple, with no new charting framework or opaque panels.

## Verification and acceptance

Automated checks should cover parsing/selection, calendar-day and DST handling,
missing data, temperature units/scaling, view switching without network requests,
refresh and recovery. Check the browser at 1024 × 600 for both views, background
visibility, legibility, whole-area mouse/touch interaction and no new controls.
The developer reviews the physical display and explicitly accepts the story
before it moves to completed.

## Acceptance

Accepted by the developer on November 3, 2026. The existing 24-hour forecast
remains the initial view, with its selection and chart rendering unchanged.
Tapping the forecast toggles a transparent ten-day high/low chart using cached
daily data from the existing refresh path. Optional condition icons were omitted
to preserve visual simplicity.

Client tests and 1024 × 600 browser checks passed, covering touch/mouse/keyboard
switching, refresh, missing data, date handling and label bounds. Physical-display
acceptance is provided by the developer. No intentional departures from the
clarified requirements.
