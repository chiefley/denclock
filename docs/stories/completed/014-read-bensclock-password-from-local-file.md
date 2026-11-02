# Read bensclock Password from Local File

Allow both deployment scripts to use the same SSH/sudo password from an optional
untracked repository-root `.bensclock-password` file. Preserve interactive use
when absent. Require owner-only file permissions, avoid passwords in command
arguments or output, and document setup and dependencies. Verify without live
deployment. Leave active for human review.

Accepted by the developer on 2026-11-01 after live review.
Optional ignored password file supports SSH and sudo. Syntax, permission enforcement, interactive fallback, and stubbed deployment checks passed.
