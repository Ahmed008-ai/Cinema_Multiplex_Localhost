"""Local-only web server for Cinema Multiplex.

Uses Python's standard library (no pip packages) to serve the HTML/CSS/JS
frontend and call the existing C backend through a small compiled DLL.
The server listens only on 127.0.0.1; it is not exposed to the network.
"""
from __future__ import annotations

import ctypes
import glob
import json
import os
import shutil
import subprocess
import sys
import threading
import webbrowser
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from typing import Optional
from urllib.parse import unquote, urlparse

ROOT = Path(__file__).resolve().parent
WEB = ROOT / "web"
HOST = "127.0.0.1"
PORT = 8765


def find_gcc() -> Optional[str]:
    """Find GCC on PATH or in common Embarcadero Dev-C++ 6 locations."""
    found = shutil.which("gcc")
    if found:
        return found
    if os.name == "nt":
        patterns = [
            r"C:\Program Files (x86)\Embarcadero\Dev-Cpp\*\bin\gcc.exe",
            r"C:\Program Files\Embarcadero\Dev-Cpp\*\bin\gcc.exe",
        ]
        for pattern in patterns:
            matches = glob.glob(pattern)
            if matches:
                return matches[0]
    return None


def build_backend() -> Path:
    if os.name == "nt":
        output = ROOT / "cinema_backend.dll"
    else:
        output = ROOT / "libcinema_backend.so"
    source = ROOT / "web_backend.c"
    dependencies = [source, ROOT / "backend" / "project.h"] + list((ROOT / "backend").glob("*.c"))
    newest_source = max(path.stat().st_mtime for path in dependencies if path.exists())
    if output.exists() and output.stat().st_mtime >= newest_source:
        return output

    gcc = find_gcc()
    if not gcc:
        raise RuntimeError(
            "GCC was not found. Install/configure the GCC compiler bundled with "
            "Dev-C++ 6.3, or add its bin folder to PATH, then run this launcher again."
        )

    command = [gcc, "-std=c99", "-O2", "-Wall", "-Wextra", "-shared", str(source), "-o", str(output), "-lm"]
    if os.name == "nt":
        command.insert(1, "-static-libgcc")
    else:
        command.insert(1, "-fPIC")

    print("[1/2] Building the C backend bridge...")
    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr)
        raise RuntimeError("The C backend could not be compiled. See the compiler output above.")
    if result.stderr.strip():
        print(result.stderr.strip())
    return output


class CinemaBackend:
    def __init__(self) -> None:
        library_path = build_backend()
        if os.name == "nt":
            gcc = find_gcc()
            if gcc and hasattr(os, "add_dll_directory"):
                self._dll_directory = os.add_dll_directory(str(Path(gcc).parent))
        self.lib = ctypes.CDLL(str(library_path))
        self.lib.api_init.argtypes = []
        self.lib.api_init.restype = ctypes.c_int
        self.lib.api_get_state.argtypes = []
        self.lib.api_get_state.restype = ctypes.c_char_p
        self.lib.api_add_movie.argtypes = [ctypes.c_char_p, ctypes.c_int]
        self.lib.api_add_movie.restype = ctypes.c_char_p
        self.lib.api_register_customer.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        self.lib.api_register_customer.restype = ctypes.c_char_p
        self.lib.api_add_show.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        self.lib.api_add_show.restype = ctypes.c_char_p
        self.lib.api_book.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                                      ctypes.c_int, ctypes.c_int, ctypes.c_char_p]
        self.lib.api_book.restype = ctypes.c_char_p
        self.lib.api_join_waitlist.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.api_join_waitlist.restype = ctypes.c_char_p
        self.lib.api_cancel.argtypes = [ctypes.c_char_p]
        self.lib.api_cancel.restype = ctypes.c_char_p
        self.lib.api_save.argtypes = []
        self.lib.api_save.restype = ctypes.c_char_p
        self.lib.api_shutdown.argtypes = []
        self.lib.api_shutdown.restype = None
        if not self.lib.api_init():
            raise RuntimeError("The C backend could not initialize its booking and waiting-list memory.")

    @staticmethod
    def decode(value: Optional[bytes]) -> dict:
        if not value:
            return {"ok": False, "message": "The C backend returned an empty response."}
        try:
            return json.loads(value.decode("utf-8", errors="replace"))
        except (ValueError, UnicodeDecodeError):
            return {"ok": False, "message": "The C backend returned an invalid response."}

    def state(self) -> dict:
        return self.decode(self.lib.api_get_state())

    def post(self, action: str, data: dict) -> dict:
        enc = lambda value: str(value).encode("utf-8")
        if action == "movies":
            return self.decode(self.lib.api_add_movie(enc(data.get("title", "")), int(data.get("duration", 0))))
        if action == "customers":
            return self.decode(self.lib.api_register_customer(enc(data.get("name", "")), enc(data.get("phone", ""))))
        if action == "shows":
            return self.decode(self.lib.api_add_show(
                int(data.get("movieId", -1)), enc(data.get("date", "")),
                int(data.get("minutes", -1)), int(data.get("dayType", 0)),
                int(data.get("preferredScreen", -1))))
        if action == "bookings":
            return self.decode(self.lib.api_book(
                int(data.get("customerId", -1)), int(data.get("showId", -1)),
                int(data.get("row", -1)), int(data.get("col", -1)),
                int(data.get("comboId", 0)), int(data.get("quantity", 0)),
                enc(data.get("promoCode", "NONE"))))
        if action == "waitlist":
            return self.decode(self.lib.api_join_waitlist(int(data.get("customerId", -1)), int(data.get("showId", -1))))
        if action == "cancel":
            return self.decode(self.lib.api_cancel(enc(data.get("bookingId", ""))))
        if action == "save":
            return self.decode(self.lib.api_save())
        return {"ok": False, "message": "Unknown API action."}


backend: Optional[CinemaBackend] = None


class Handler(BaseHTTPRequestHandler):
    server_version = "CinemaLocal/1.0"

    def log_message(self, fmt: str, *args) -> None:
        # Keep the terminal output calm; only show API failures and startup text.
        if args and str(args[1] if len(args) > 1 else "").startswith("5"):
            super().log_message(fmt, *args)

    def send_json(self, payload: dict, status: int = 200) -> None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path == "/api/state":
            self.send_json(backend.state())
            return
        if parsed.path == "/api/health":
            self.send_json({"ok": True, "service": "Cinema Multiplex local server", "backend": "C"})
            return

        relative = "index.html" if parsed.path in ("", "/") else unquote(parsed.path.lstrip("/"))
        target = (WEB / relative).resolve()
        if WEB.resolve() not in target.parents and target != WEB.resolve():
            self.send_error(403)
            return
        if not target.is_file():
            self.send_error(404)
            return
        content_type = "text/plain; charset=utf-8"
        suffix = target.suffix.lower()
        if suffix == ".html": content_type = "text/html; charset=utf-8"
        elif suffix == ".css": content_type = "text/css; charset=utf-8"
        elif suffix == ".js": content_type = "application/javascript; charset=utf-8"
        elif suffix == ".svg": content_type = "image/svg+xml"
        body = target.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-cache")
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self) -> None:
        parsed = urlparse(self.path)
        if not parsed.path.startswith("/api/"):
            self.send_error(404)
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length <= 0 or length > 32_000:
                self.send_json({"ok": False, "message": "Request body is empty or too large."}, 400)
                return
            data = json.loads(self.rfile.read(length).decode("utf-8"))
            if not isinstance(data, dict):
                raise ValueError("Expected a JSON object")
            action = parsed.path.rsplit("/", 1)[-1]
            result = backend.post(action, data)
            self.send_json(result, 200 if result.get("ok") else 400)
        except (ValueError, TypeError, json.JSONDecodeError) as exc:
            self.send_json({"ok": False, "message": f"Invalid request: {exc}"}, 400)
        except Exception as exc:  # Keep unexpected server failures visible and contained.
            self.send_json({"ok": False, "message": f"Server error: {exc}"}, 500)


def main() -> None:
    global backend
    os.chdir(ROOT)  # The C backend stores its text data files beside this launcher.
    backend = CinemaBackend()
    server = HTTPServer((HOST, PORT), Handler)
    url = f"http://{HOST}:{PORT}"
    print("\n  CINÉ  |  CINEMA MULTIPLEX MANAGEMENT STUDIO")
    print("  ------------------------------------------------")
    print(f"  Website: {url}")
    print("  Backend: existing C modules (loaded as a local DLL/shared library)")
    print("  Data:    movies.txt, shows.txt, seats.txt, customers.txt,")
    print("           bookings.txt and waitlist.txt")
    print("\n  Keep this terminal window open while using the website.")
    print("  Press Ctrl+C here to stop the local server.\n")
    threading.Timer(0.8, lambda: webbrowser.open(url)).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nSaving cinema data and shutting down...")
    finally:
        backend.lib.api_shutdown()
        server.server_close()


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"\nSTARTUP ERROR: {error}")
        print("Check that Python 3 and the GCC compiler bundled with Dev-C++ are installed.")
        input("Press Enter to close...")
        sys.exit(1)
