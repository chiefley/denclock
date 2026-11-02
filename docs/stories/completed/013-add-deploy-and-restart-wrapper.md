# Add Deploy and Restart Wrapper

Add a shell script that calls the existing bensclock deployment script with
`denclock-server.service` and `denclock-lora.service` as restart units.

Resolve the deployment script relative to the wrapper so it works from any
working directory. Preserve the existing deployment script and propagate its
exit status on failure. After successful deployment and restarts, use SSH as ben
to send Ctrl+Shift+R to the visible Chromium kiosk on X11 display :0 using
xdotool. Report missing xdotool or refresh failures and do not refresh after a
deployment failure. Document usage and verify syntax, argument forwarding, and
failure handling without performing a deployment. Leave the story active for
human acceptance.

Accepted by the developer on 2026-11-01 after live review.
Wrapper deploys, restarts both services, and hard-refreshes Chromium. Syntax and stubbed success/failure checks passed.
