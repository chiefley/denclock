# Master Surface Go 3 Provisioning Guide

## Request

Create one authoritative, sequential provisioning guide for a fresh Surface Go 3,
with `iansclock` as the first reproduction target and `bensclock` as the working
reference. Research current GitHub documentation, implementation, accepted stories,
underlying project conversations, and read-only inspection through Tailscale/SSH.

## Requirements

- Cover hardware/firmware, Debian, desktop/login, networking/SSH/Tailscale,
  RustDesk, Clight, application deployment/services, settings, power, acceptance,
  maintenance, and recovery in installation order.
- Distinguish observed, documented, and inferred findings; separate executed
  procedures from proposed or newly adapted commands. Do not invent missing history.
- Separate common configuration from household/device choices. Preserve subsystem
  runbooks and reference them instead of duplicating their extensive details.
- Compare the live reference with documentation; flag unresolved conflicts and
  limitations. Do not collect or record secrets.
- Include unresolved questions, a first-install observation log, a complete
  acceptance checklist, and prioritized automation opportunities.
- Use the latest GitHub version; do not modify application code or the tablet.
  Do not implement provisioning automation.

## Acceptance criteria

- The existing Surface guide becomes the authoritative master, with navigation
  and superseded guidance clearly identified.
- A new engineer can follow the installation sequence and identify every hold
  point and device-specific value before making changes.
- Report source coverage, checks, remaining gaps, and fresh-device readiness.

## Acceptance

Accepted by the developer on November 7, 2026. The existing Surface guide now
provides the master provisioning sequence, linked subsystem references, observed
reference-system findings, unresolved questions, an installation observation log,
and acceptance and future-automation checklists. Documentation links, shell syntax,
secret-pattern and whitespace checks passed. The tablet was inspected read-only;
application code and tablet configuration were unchanged. Fresh-device testing
remains for the supervised `iansclock` trial, with the documented hold points.
