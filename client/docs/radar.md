# Animated radar

The forecast cycles through **24 hours → ten days → radar → 24 hours** using the
existing whole-area tap/click or Enter/Space interaction, except for the replay
button and attribution links. It starts hourly on
page load. Attribution links navigate normally. Clock, sensors, settings, and the
two forecast charts retain their existing presentation and refresh behavior.

## Map and data

Radar uses only `radar.past` from the
[RainViewer metadata endpoint](https://api.rainviewer.com/public/weather-maps.json).
Host and opaque frame paths come from that response; paths are not constructed
from timestamps. Up to thirteen historical frames are sorted chronologically.
No nowcast or satellite imagery is requested.

The [Weather Maps API](https://www.rainviewer.com/api/weather-maps-api.html) and
[transition guidance](https://www.rainviewer.com/api/transition-faq.html) specify
historical coverage of about two hours, maximum zoom 7, Universal Blue (color 2),
and a 100-request/IP/minute limit. The general API FAQ is less restrictive; this
implementation follows the stricter transition limit. Metadata refreshes every
five minutes, even outside radar mode. Tile requests are serialized with at least
800 ms between starts (at most 75/minute per display); metadata adds one request
per five minutes. Other devices sharing the same public IP also consume the
provider's allowance.

A small canvas renderer supplies a fixed Web Mercator regional map. It uses
zoom 7 and keeps the longer map edge at 512 map pixels, scaling to the container
rather than adding tiles for high-DPI screens. Canvas resolution follows display
pixel ratio up to 2. The map is centered on the configured weather coordinates,
with a subtle home marker. No pan/zoom gestures, map toolbars, or mapping
framework are needed.

The geographic layer uses [OpenStreetMap standard tiles](https://tile.openstreetmap.org),
darkened/desaturated to keep labels and roads subdued. Radar is deliberately
opaque: a solid dark backing covers the photograph throughout the map rectangle,
even while tiles are loading or unavailable. Both the geographic and precipitation
layers have 100% CSS opacity; transparent pixels in precipitation tiles reveal
the geographic map. This exception does not affect the two forecast charts. Visible credit
links identify RainViewer and OpenStreetMap contributors. Only the currently
visible map's tiles are requested; there is no background geographic prefetch.
Normal browser HTTP caching, User-Agent and Referer are preserved in accordance
with the [OSM tile policy](https://operations.osmfoundation.org/policies/tiles/).
No key, backend proxy or additional runtime package is required.

## Loading, playback and recovery

Tiles load only when radar is visible and synchronized time is available.
The newest complete frame appears first while the rest of the history loads.
Initial preload may take about a minute (longer for a taller viewport or slow
network) because requests are deliberately throttled. A loading message remains
visible. Once ready, complete frames play oldest-to-newest twice at 500 ms per
frame and hold on the newest successfully loaded frame. Re-entering radar
replays cached history. The small circular-arrow button at the map's lower-left
has a 48 × 48 CSS-pixel touch target and stays visible when paused. Tap/click or
keyboard activation restarts the oldest available cached frame, plays two passes,
and holds newest. It does not cycle the parent forecast or request data. The
button is disabled without usable frames, while hidden, or when imagery expires.
Transparent, successfully loaded radar tiles represent a
valid quiet map and are not considered failures.

Frames are assembled offscreen before replacing the visible image, so loading
or missing tiles never produce a partially drawn animation frame. Failed frames
are skipped and identified with an availability message. Tile results are cached
in a bounded 192-entry memory cache in addition to ordinary browser caching;
failed tiles have a five-minute retry cooldown. Newly published frames are loaded
on refresh, while cached frames avoid repeat downloads. Resize/reveal redraws
use current container dimensions and cancel obsolete loading work. Location
changes clear the old location's imagery before rebuilding.

Metadata and tile requests time out after eight seconds. A failed refresh retains
last usable imagery with an update-delay message. Metadata not successfully
refreshed for ten minutes or a newest available frame twenty minutes old is also
marked delayed. Imagery whose newest available
frame is thirty minutes old relative to synchronized server UTC is hidden as
expired. The newest loaded frame's timestamp and age remain visible, including
while paused or expired: for example, `Radar 11:05 AM EST · 8 min old`. Age uses
synchronized server UTC minus the frame timestamp, rounded down to whole minutes
and clamped to zero, and updates through the existing forecast render loop. It
never resets merely because metadata or tiles were downloaded again. During
animation, this freshness label still describes the newest available frame, not
the older frame currently being drawn. At twenty minutes, an amber `stale` label
also warns about age, independently of metadata download freshness.

RainViewer exposes the composite **frame-generation time**, not one exact
observation time for all underlying radar sources; this is the actual provider
frame timestamp used for the time and age display. Times are formatted in the
forecast location timezone, explicitly UTC until that timezone is known. No
observation time is inferred from metadata generation or download time.

Animation stops when leaving radar or when the page is hidden. Cleanup aborts
metadata/loading work, clears timers, disconnects resize observation and releases
cached canvases and tiles. Radar failure does not stop the clock or forecast
polling, and users can still cycle out of an unavailable radar view.

## Verification

Run `npm test --prefix client` for parsing, tile projection/limits, request
throttling, caching, two-cycle playback/replay, expiry, refresh recovery, location
changes, resizing, and three-view rotation. Browser checks should cover native
1024 × 600 layout, a larger high-DPI viewport, transparent dry tiles, missing
frames, network failure, attribution links, and leaving/re-entering during loads.
For physical review, inspect actual precipitation, legibility, centering and tap
behavior. The developer accepted [Animated Weather Radar View](stories/completed/014-animated-weather-radar-view.md)
on November 3, 2026 during a dry-weather interval, following the base-map opacity
adjustment. Precipitation appearance during rain has not yet been observed by the
developer; subsequent observations may inform later work.

## Radar refinement review

[Radar Display Refinements](stories/completed/015-radar-display-refinements.md)
was accepted by the developer on November 4, 2026 after display review.
The solid radar surface and replay control deliberately supersede the earlier
transparency and no-controls preferences in the accepted radar story.
