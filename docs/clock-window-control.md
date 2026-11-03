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
