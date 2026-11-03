#!/usr/bin/env python3
"""Local XFCE/X11 clock-window control. Run as the desktop user, never root."""
import json
import subprocess
import time
from http.server import BaseHTTPRequestHandler, HTTPServer

HOST = "127.0.0.1:5001"
ORIGINS = {"http://localhost:5000", "http://127.0.0.1:5000"}


def command(*args):
    return subprocess.run(args, check=True, capture_output=True, text=True,
                          timeout=2).stdout


def clock_window():
    # Chromium --class=DenClock identifies only the dedicated clock window.
    windows = []
    for row in command("wmctrl", "-lxp").splitlines():
        fields = row.split(None, 5)
        if len(fields) >= 4 and "denclock" in fields[3].lower().split("."):
            windows.append(fields[0])
    if len(windows) != 1:
        raise RuntimeError("Expected one DenClock app window. Check the kiosk launcher.")
    return windows[0]


def fullscreen(window):
    return "_NET_WM_STATE_FULLSCREEN" in command(
        "xprop", "-id", window, "_NET_WM_STATE")


def set_fullscreen(window, desired):
    command("wmctrl", "-ir", window, "-b",
            ("add" if desired else "remove") + ",fullscreen")
    if not desired:
        command("wmctrl", "-ir", window, "-b",
                "remove,maximized_vert,maximized_horz")
        command("wmctrl", "-ir", window, "-e", "0,32,32,900,500")
    # Window-manager requests are asynchronous; report observed state.
    for _ in range(20):
        if fullscreen(window) == desired:
            return desired
        time.sleep(0.05)
    raise RuntimeError("The desktop did not change the clock window state.")


class Handler(BaseHTTPRequestHandler):
    def log_request(self, code="-", size="-"):
        # Avoid filling the journal with successful state polls.
        if str(code) != "200":
            super().log_request(code, size)

    def allowed(self):
        return (self.headers.get("Host") == HOST
                and self.headers.get("Origin") in ORIGINS)

    def reply(self, code, data):
        body = json.dumps(data).encode()
        self.send_response(code)
        if self.allowed():
            self.send_header("Access-Control-Allow-Origin", self.headers["Origin"])
            self.send_header("Vary", "Origin")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_OPTIONS(self):
        if not self.allowed() or self.path != "/window":
            self.reply(403, {"error": "Not allowed."})
            return
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", self.headers["Origin"])
        self.send_header("Vary", "Origin")
        self.send_header("Access-Control-Allow-Methods", "GET, POST")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def handle_window(self, update=False):
        if not self.allowed():
            self.reply(403, {"error": "Only the local clock page may control its window."})
            return
        if self.path != "/window":
            self.reply(404, {"error": "Not found."})
            return
        try:
            desired = None
            if update:
                length = int(self.headers.get("Content-Length", "0"))
                if not 0 < length <= 128 or self.headers.get("Content-Type") != "application/json":
                    self.reply(400, {"error": "Expected a small JSON request."})
                    return
                data = json.loads(self.rfile.read(length))
                if not isinstance(data, dict) or set(data) != {"fullscreen"} or type(data["fullscreen"]) is not bool:
                    self.reply(400, {"error": "Expected a fullscreen boolean."})
                    return
                desired = data["fullscreen"]
            window = clock_window()
            state = set_fullscreen(window, desired) if update else fullscreen(window)
            self.reply(200, {"fullscreen": state})
        except (ValueError, RuntimeError, OSError, subprocess.SubprocessError):
            self.reply(503, {"error": "Clock window control unavailable. Check the desktop helper and launcher."})

    def do_GET(self):
        self.handle_window()

    def do_POST(self):
        self.handle_window(update=True)


if __name__ == "__main__":
    server = HTTPServer(("127.0.0.1", 5001), Handler)
    server.timeout = 1
    server.serve_forever()
