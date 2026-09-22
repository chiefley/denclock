# Record Deployment Preservation Deferral

## Purpose

Record the developer's decision to defer deployment preservation changes while
the system is evolving.

## Requirements

- Continue publishing and deploying the full publish output for now.
- Defer file exclusions and configuration/log preservation changes until the
  deployment is more stable.
- Record this policy in the LoRa Service deployment guidance without changing
  the deployment script.

## Acceptance Criteria

- Deployment guidance clearly records the deferral and its rationale.
- Current full-output deployment behavior remains unchanged.
- Documentation whitespace checks pass.

## Acceptance and Verification

Accepted by the human developer on 2026-09-22.

Recorded the decision to continue full-output deployment and defer exclusions
and configuration/log preservation until deployment is more stable. No changes
to deployment behavior or departures from the requested scope were required.

Documentation whitespace and the deployment-helper link were checked. The
deployment script remains unchanged; no runtime tests were needed.
