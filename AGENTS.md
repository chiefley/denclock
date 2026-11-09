# DenClock Development Instructions

## Project

DenClock is a dedicated clock and weather display for the den.

The project is being developed incrementally. There is substantial
discovery ahead, so avoid speculative architecture and premature
abstraction. Implement the simplest solution that satisfies the current
requirement, and refactor when experience demonstrates a need.

## Target Platform

All new DenClock deployments target Surface Go 3 tablets running Debian 13,
XFCE/X11 and LightDM. The Windows NUC installation is deprecated; preserve its
historical files but do not use it as the deployment baseline.
Development takes place in the existing WSL workspace at ~/src/denclock.
Use the master Surface Go provisioning guide and current subsystem runbooks.

## Repository Structure

- `server/` - DenClock server application
- `client/` - DenClock display application
- `firmware/` - Embedded firmware
- `docs/` - Repository-wide architecture, hardware, and development conventions

Firmware currently anticipates:

- `firmware/lora-gateway/`
- `firmware/sensors/`

Do not create additional directory hierarchies merely to anticipate
possible future requirements.

## Development Philosophy

- Prefer simple, readable implementations.
- Avoid premature abstraction.
- Avoid adding frameworks or dependencies without a concrete benefit.
- Do not implement anticipated features unless requested.
- Keep server and client responsibilities clearly separated.
- Make incremental changes that leave the project in a working state.
- Preserve existing behavior when extending functionality.
- Update documentation when an architectural decision changes.

## Working With This Repository

Before making substantial changes:

1. Read `README.md`.
2. Read root `STATUS.md`, then the relevant component README, contract, decisions, and supplied active story.
3. Inspect the existing implementation before proposing new structure.
4. Follow established project conventions where they exist.

When requirements are ambiguous, favor the smallest implementation that
can be easily changed as the design evolves.

## Development And Deployment Environment

Development host: your XPS 14, VS Code/WSL
Deployment target: Surface Go 3, Debian 13, native 1920×1280 touchscreen

## Story-Driven Work and Human Acceptance

Follow [the authoritative development workflow](docs/DEVELOPMENT.md).

- Story requirements originate with the human developer, usually in collaboration
  with ChatGPT. Create a numbered Markdown story from the supplied instructions
  before implementation, at the lowest scope that contains the work.
- Choose the next number across that scope's active and completed stories.
  Implement against the supplied story; do not independently start pending work.
- Ensure each existing story scope has a `completed/` directory; do not create
  story scopes speculatively.
- Report implementation and verification results for human review. Only explicit
  human acceptance permits declaring completion and moving the story to
  `completed/`. Move rather than copy, and repair affected links.
- Closure includes updates to affected enduring contracts and design decisions
  when appropriate. Contracts describe implemented behavior, decisions explain
  durable rationale, and completed stories preserve the requested work.
- Do not maintain component STATUS files just to duplicate story-directory state.
  Preserve useful information elsewhere before removing duplicative documents.
  Update root status only for material project-wide changes.
- After acceptance and closure, stage the story's changes and provide the story
  title without its sequence prefix as the suggested commit message. Staging
  has no message field. The human developer commits; do not commit automatically.
- Git provides version history. Do not continually revise completed stories to
  describe later behavior or maintain development diaries.
