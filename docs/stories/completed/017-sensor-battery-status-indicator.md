# Sensor Battery Status Indicator

## Objective

Add an unobtrusive vertical battery icon above the fractional tenths digit in
each battery-powered sensor's displayed temperature block. Preserve existing
fonts, sizing and top-row balance; add no visible voltage or percentage text,
background panel or surrounding UI border.

## Requirements

- Apply generically to displayed sensor nodes, never to the USB-powered gateway.
- Use four bottom-up fill segments: >3.9 V = four/full; 3.7–3.9 V = three/good;
  3.5–<3.7 V = two/low; 3.4–<3.5 V = one/very low; <3.4 V = empty/critical.
  Shared boundaries use 3.7 V as good, 3.5 V as low, and 3.4 V as very low.
- Full/good: neutral color at 35% opacity. Low/very low: amber at 65%.
  Critical: muted red at 100%. Apply styling to the icon independently of values.
- Use measured Heltec onboard battery voltage carried alongside sensor readings.
  Existing firmware/MQTT transport already supplies this measurement; reuse it.
- Extend DenClock's server presentation snapshot to expose nullable batteryVoltage
  in volts, then render it in each displayed sensor slot without name-specific
  rules. Gateway identity is routing context, not a battery measurement source.
- Hide the indicator for absent/invalid voltage, unassigned positions and startup
  without an observation; do not imply a depleted or full battery when unknown.
  A later report missing battery must clear the previous icon. No new settings,
  sensor views or gateway battery indicator are needed.
- This is an operational voltage-band indicator, not a calibrated state-of-charge
  estimate. Preserve existing temperature/humidity, stale handling and polling.

## Verification and acceptance

Test MQTT-to-snapshot voltage propagation, canonical units, invalid/missing values,
replacement, identities, exact thresholds, icon fill/color/opacity, startup and
missing-data hiding. Check native 1024 × 600 and larger/high-DPI layout: the icon
sits over the tenths digit without moving existing text. Verify both slots and
critical/low/normal states. Human display review and explicit acceptance are
required before moving this story to completed.

## Acceptance

Accepted by the developer on November 4, 2026 after confirming both sensor
indicators are present. Healthy batteries intentionally produce faint icons;
the right-hand icon lies over a bright background area. The developer accepted
the existing styling without further changes.

Implemented nullable battery-voltage propagation from existing MQTT measurements
and compact per-sensor indicators with the requested bands and opacity levels.
Existing sensor firmware and LoRaService already supplied the voltage. Client
and server tests, MQTT-to-HTTP integration, and native-size/high-DPI browser
checks passed, including missing-data hiding and unchanged text geometry.
No intentional departures from the clarified requirements.
