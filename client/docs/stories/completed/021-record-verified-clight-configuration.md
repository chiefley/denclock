# Record Verified Clight Configuration

## Requirements

Update the existing Surface Go 3 Clight guide and configuration assets with the
exact configuration recovered from bensclock. Preserve dynamic ALS discovery:
`iio:device1` is a placeholder replaced by the startup script. Record identical
AC/battery curves of 0.01, 0.02, 0.03, 0.05, 0.08, 0.20, 0.27, 0.35, 0.44, 0.53,
0.60; verbose logging; restore-on-exit; 2000-ms transitions; 10-second timeouts;
and disabled gamma, dimmer, DPMS, screen, and keyboard modules.

Remove obsolete unknown/unverified-curve warnings. Preserve incomplete-build
caveats. Record user verification on Debian 13 / Surface Go 3, including automatic
operation after reboot. Do not change application code or the live tablet.
Validate the asset against the guide and report affected files for human review.

## Recovery and review

Retrieve the launcher and desktop entry through read-only SSH access, preserve
those assets, and align the guide with their actual behavior. No live tablet
files may be changed.

## Acceptance

Accepted by the developer on November 7, 2026. The guide and configuration now
record the verified settings; the recovered launcher and desktop entry are
preserved, with the guide aligned to their behavior. Configuration comparison
against the live file (ignoring whitespace), Bash syntax validation, and whitespace
checks passed. The live tablet was accessed read-only. Build-command documentation
remains intentionally incomplete pending recovery of the build evidence.
