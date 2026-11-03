import importlib.util
import http.client
import json
from pathlib import Path
import threading
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("window_control", Path(__file__).parents[1] / "window-control.py")
control = importlib.util.module_from_spec(spec)
spec.loader.exec_module(control)


class WindowControlTests(unittest.TestCase):
    def setUp(self):
        self.server = control.HTTPServer(("127.0.0.1", 0), control.Handler)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join()

    def request(self, method="GET", body=None, **headers):
        connection = http.client.HTTPConnection("127.0.0.1", self.server.server_port, timeout=3)
        defaults = {"Host": control.HOST, "Origin": "http://localhost:5000"}
        defaults.update(headers)
        connection.request(method, "/window", body, defaults)
        response = connection.getresponse()
        result = response.status, response.getheader("Access-Control-Allow-Origin"), response.read()
        connection.close()
        return result

    def test_only_local_clock_origin_and_exact_host_are_allowed(self):
        with patch.object(control, "clock_window") as window:
            for headers in ({"Origin": "http://evil.example"}, {"Origin": "null"},
                            {"Host": "evil.example:5001"}, {"Origin": ""}):
                code, cors, _ = self.request("POST", '{"fullscreen":false}', **headers)
                self.assertEqual(code, 403)
                self.assertIsNone(cors)
            window.assert_not_called()

    def test_observed_state_and_boolean_only_updates(self):
        with patch.object(control, "clock_window", return_value="0x123"), \
             patch.object(control, "fullscreen", return_value=True), \
             patch.object(control, "set_fullscreen", return_value=False) as update:
            code, cors, data = self.request()
            self.assertEqual(code, 200)
            self.assertEqual(cors, "http://localhost:5000")
            self.assertEqual(json.loads(data), {"fullscreen": True})
            code, _, data = self.request("POST", '{"fullscreen":false}', **{"Content-Type": "application/json"})
            self.assertEqual(code, 200)
            self.assertEqual(json.loads(data), {"fullscreen": False})
            update.assert_called_once_with("0x123", False)
            for body in ('{"fullscreen":"false"}', '{"fullscreen":false,"command":"x"}'):
                self.assertEqual(self.request("POST", body, **{"Content-Type": "application/json"})[0], 400)
            self.assertEqual(update.call_count, 1)

    def test_missing_window_reports_unavailable(self):
        with patch.object(control, "clock_window", side_effect=RuntimeError("missing")):
            self.assertEqual(self.request()[0], 503)

    def test_window_selection_requires_exact_dedicated_class_and_single_window(self):
        with patch.object(control, "command", return_value="0x1 0 123 chromium.Other host DenClock\n"):
            with self.assertRaises(RuntimeError):
                control.clock_window()
        with patch.object(control, "command", return_value="0x2 0 234 chromium.DenClock host clock\n"):
            self.assertEqual(control.clock_window(), "0x2")
        with patch.object(control, "command", return_value="0x2 0 234 chromium.DenClock host clock\n0x3 0 234 chromium.DenClock host clock\n"):
            with self.assertRaises(RuntimeError):
                control.clock_window()


if __name__ == "__main__":
    unittest.main()
