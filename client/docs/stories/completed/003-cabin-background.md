# Display the Denclock Cabin Background

Status: Complete; accepted by the human developer on 2026-09-18.

Implemented a separate decorative background layer using the canonical cabin
image with centered cover scaling. Clock layout and synchronization remain
unchanged. Publish verification confirmed inclusion of the image and stylesheet;
whitespace checks passed. The developer reviewed and accepted the result.

As a Denclock user, I want the main display to use the Denclock cabin image
as its full-screen background, so that the clock has its intended visual
appearance rather than a plain background.

## Acceptance Criteria

- Use the canonical cabin image supplied under `client/images/` in this
  repository (called `src/client/images/` in the supplied instructions):
  `Denclock-cabin-master.png`.
- Display the image as the full-screen background of the Denclock client.
- Scale/crop as necessary to completely fill the 1024 × 600 display while
  preserving the image's aspect ratio.
- Keep the existing clock and other UI elements overlaid on the background.
- Do not affect positioning or operation of existing controls or data.
- Keep background implementation separate from the clock/data UI so the image
  can later be selected dynamically.
- Do not implement season, weather, or day/night selection.

## Future Extension

Selection based on season, current weather, and day/night state is explicitly
outside the scope of this story.
