# Clock fullscreen control (XFCE/X11)

The upper-right rectangle restores the fullscreen clock to a 900x500 desktop
window; the single rectangle returns it to fullscreen. The icon follows actual
window-manager state. The 48px touch target contains a small 20px icon.
The rest of the display keeps running, including settings and forecast polling.

Chromium's locked --kiosk mode prevents this workflow. Use the supplied app-window
launcher instead. It starts fullscreen, retains the existing dedicated profile
and restarts Chromium when closed. Closing the clock still relaunches it; use
Restore window and then the normal minimize button for desktop maintenance.

## One-time installation on the tablet

First deploy the updated client with the existing deployment script.
From WSL in ~/src/denclock, copy the desktop files:

```sh
ssh ben@bensclock 'mkdir -p ~/.local/lib/denclock ~/.local/bin ~/.config/systemd/user; if test -f ~/.local/bin/denclock-kiosk.sh; then cp -p ~/.local/bin/denclock-kiosk.sh ~/.local/bin/denclock-kiosk.sh.before-window-control; fi'
scp tools/kiosk/window-control.py ben@bensclock:.local/lib/denclock/
scp tools/kiosk/denclock-window-control.service ben@bensclock:.config/systemd/user/
scp tools/kiosk/denclock-kiosk.sh ben@bensclock:.local/bin/denclock-kiosk.sh
```

On the tablet through SSH:

```sh
sudo apt install python3 wmctrl x11-utils
chmod +x ~/.local/bin/denclock-kiosk.sh
systemctl --user daemon-reload
systemctl --user enable denclock-window-control.service
```

Keep the existing XFCE autostart entry pointing to
/home/ben/.local/bin/denclock-kiosk.sh. Reboot the tablet to replace the old running
kiosk process. The launcher imports the graphical session environment and starts
the helper as ben; no root service or sudo command runs from the webpage.

The helper listens only on 127.0.0.1:5001 and accepts only the local clock origins
http://localhost:5000 and http://127.0.0.1:5000 with an exact Host check.
It only reads/sets fullscreen state on the single window identified by Chromium's
DenClock class. No arbitrary command, window ID, or path comes from the request.
The button is hidden on LAN pages, so viewing the clock from your laptop does not
control the tablet. An unavailable helper disables the control and displays a
short message, retrying automatically. No server endpoint or runtime changes
are required. This helper is specific to the requested Linux/X11 deployment.

## Verify on the tablet

- After reboot the clock opens fullscreen and RustDesk still connects.
- Tap overlapping rectangles: window controls and desktop become accessible.
- Tap the single rectangle: return to fullscreen; repeat using RustDesk.
- Change fullscreen with desktop controls/F11 and verify the icon catches up.
- Verify the icon clears sensor values at the tablet's resolution.
- Run deploy-and-restart-bensclock.sh and confirm the clock refreshes.
- Check settings, time and forecast continue working in both modes.

Diagnostics: systemctl --user status denclock-window-control.service and
journalctl --user -u denclock-window-control.service.
The developer accepted [Toggle clock fullscreen](stories/completed/015-toggle-clock-fullscreen.md)
on November 3, 2026. Codex did not independently verify the actual tablet.

Automated checks: node --test client/tests/*.test.js and
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tools/kiosk/tests.

## Onboard keyboard accessibility

The launcher passes `--force-renderer-accessibility=complete` to Chromium so
web fields and focus are exposed to assistive technology throughout its lifetime.
Chromium normally enables renderer accessibility on demand; see its
[accessibility overview](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/accessibility/overview.md).
This uses the existing Onboard and desktop AT-SPI accessibility stack. No new
packages, services, browser JavaScript, or keyboard-control endpoint were added.
The dedicated profile and fullscreen/window controls are unchanged.

On bensclock, Onboard was running with auto-show enabled while DenClock fields
did not summon it. Before this change, an AT-SPI inspection encountered an
unreadable child of the Chromium frame. After restart with the flag, AT-SPI
exposed the DenClock web document and Settings button. These observations support
an accessibility-exposure problem but do not establish the exact cause or prove
keyboard activation. The developer verified and accepted the Surface Go behavior, including touch,
editing, focus-loss, fullscreen/windowed operation, and reboot startup in
[Story 016](stories/completed/016-restore-on-screen-keyboard.md) on November 3, 2026.
The launcher flag resolves the observed activation problem; the precise reason
Chromium did not enable sufficient accessibility automatically remains unconfirmed.

### Updating an existing Story 015 installation

Use the established SSH workflow as ben (prefix SSH/SCP with
`sshpass -f .bensclock-password` from the repository root when using the existing
private password file). Before replacing the launcher, preserve a rollback copy:

```sh
ssh ben@bensclock 'test ! -e ~/.local/bin/denclock-kiosk.sh.before-story016 && cp -p ~/.local/bin/denclock-kiosk.sh ~/.local/bin/denclock-kiosk.sh.before-story016'
scp tools/kiosk/denclock-kiosk.sh ben@bensclock:.local/bin/denclock-kiosk.sh
```

A page refresh does not apply Chromium startup flags. Restart the launcher and
its browser, without restarting the server, LoRa service, Onboard, or desktop.
For the installed launcher, run through SSH as ben:

```sh
export DISPLAY=:0
launcher=$(pgrep -f '^bash /home/ben/.local/bin/denclock-kiosk.sh$')
window=$(wmctrl -lx | awk 'tolower($3) ~ /(^|\.)denclock(\.|$)/ {print $1}')
test -n "$launcher" && test -n "$window" || exit 1
kill "$launcher"
wmctrl -ic "$window"
sleep 3
nohup /home/ben/.local/bin/denclock-kiosk.sh > /tmp/denclock-kiosk.log 2>&1 < /dev/null &
```

The launcher also restarts its existing window-control helper. XFCE autostart
continues to use the same launcher on reboot. Story 016's deployment saved the
previous launcher as `~/.local/bin/denclock-kiosk.sh.before-story016`.
To roll back, copy that file over `~/.local/bin/denclock-kiosk.sh` with `cp -p`
and repeat the browser/launcher restart above. Do this if touch testing fails;
report findings before trying a more elaborate solution.

### Verification and diagnosis

On the Surface, open Settings and touch editable fields. Verify Onboard appears,
can enter/edit text, and behaves appropriately when focus leaves a field. Repeat
in restored and fullscreen modes, including use of the fullscreen button. Reboot
and repeat to verify startup. Discard test edits unless deliberately saving them.

If activation fails, check `pgrep -af onboard` and
`gsettings get org.onboard.auto-show enabled` in ben's graphical session. Check
`pgrep -af '^/usr/lib/chromium/chromium .*--app=http://localhost:5000'` for the
accessibility flag and dedicated profile. Inspect `chrome://accessibility` in
that Chromium instance or its AT-SPI tree using existing desktop accessibility
tools; compare whether native applications still invoke Onboard. A running
Onboard process and focused HTML field alone do not prove accessible focus events
are reaching Onboard. Avoid changing global desktop settings until the minimal
launcher change has been assessed.
