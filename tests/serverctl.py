"""Build, start and stop the FlowScope servers.

Shared by conftest.py (functional tests) and benchmark.py.

Build location:
    Each server is built with its own CMakeLists into a per-server subdirectory
    of the build root. The build root defaults to ``tests/_build`` but can be
    overridden with the FLOWSCOPE_BUILD_DIR environment variable — useful when
    the repository lives on a filesystem CMake dislikes (e.g. a Windows mount),
    in which case point it at a native path like /tmp/fsbuild.
"""
from __future__ import annotations

import os
import pathlib
import shutil
import signal
import socket
import subprocess
import time

import protocol as proto

REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
HOST = "127.0.0.1"


def build_dir() -> pathlib.Path:
    env = os.environ.get("FLOWSCOPE_BUILD_DIR")
    root = pathlib.Path(env) if env else (pathlib.Path(__file__).resolve().parent / "_build")
    root.mkdir(parents=True, exist_ok=True)
    return root


def port_open(port: int, host: str = HOST, timeout: float = 0.3) -> bool:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(timeout)
        return s.connect_ex((host, port)) == 0


def wait_port(port: int, timeout: float = 15.0) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if port_open(port):
            return True
        time.sleep(0.1)
    return False


def build_all(log=print) -> dict:
    """cmake-configure + build every server. Returns {name: binary Path}."""
    cmake = shutil.which("cmake")
    if not cmake:
        raise RuntimeError(
            "cmake not found on PATH. Install it (e.g. `sudo apt-get install -y cmake`)."
        )
    root = build_dir()
    binaries: dict[str, pathlib.Path] = {}
    for name, info in proto.SERVERS.items():
        src = REPO_ROOT / info["src"]
        out = root / name
        cfg = subprocess.run(
            [cmake, "-S", str(src), "-B", str(out), "-DCMAKE_BUILD_TYPE=Release"],
            capture_output=True, text=True,
        )
        if cfg.returncode != 0:
            raise RuntimeError(f"cmake configure failed for {name}:\n{cfg.stdout}\n{cfg.stderr}")
        bld = subprocess.run(
            [cmake, "--build", str(out), "-j", str(os.cpu_count() or 4)],
            capture_output=True, text=True,
        )
        if bld.returncode != 0:
            raise RuntimeError(f"build failed for {name}:\n{bld.stdout}\n{bld.stderr}")

        binary = out / info["binary"]
        if not binary.exists():
            found = list(out.rglob(info["binary"]))
            if not found:
                raise RuntimeError(f"binary '{info['binary']}' not found for {name} under {out}")
            binary = found[0]
        binaries[name] = binary
        log(f"  built {name:8s} -> {binary}")
    return binaries


class ServerHandle:
    """A single running server process. Started in its own process group so the
    whole group can be torn down reliably even if the server spawns threads."""

    def __init__(self, name: str, port: int, binary):
        self.name = name
        self.port = port
        self.binary = pathlib.Path(binary)
        self.proc: subprocess.Popen | None = None
        self.logpath = build_dir() / f"{name}.run.log"
        self._log = None

    def start(self, wait: float = 15.0) -> "ServerHandle":
        if port_open(self.port):
            raise RuntimeError(f"port {self.port} already in use (server {self.name})")
        self._log = open(self.logpath, "wb")
        self.proc = subprocess.Popen(
            [str(self.binary)],
            cwd=str(self.binary.parent),
            stdout=self._log, stderr=subprocess.STDOUT,
            start_new_session=True,
        )
        if not wait_port(self.port, timeout=wait):
            self.stop()
            raise RuntimeError(f"{self.name} did not open port {self.port} (see {self.logpath})")
        return self

    @property
    def pid(self):
        return self.proc.pid if self.proc else None

    def stop(self) -> None:
        if self.proc and self.proc.poll() is None:
            try:
                os.killpg(os.getpgid(self.proc.pid), signal.SIGTERM)
            except (ProcessLookupError, PermissionError):
                self.proc.terminate()
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                try:
                    os.killpg(os.getpgid(self.proc.pid), signal.SIGKILL)
                except (ProcessLookupError, PermissionError):
                    self.proc.kill()
                self.proc.wait()
        if self._log:
            self._log.close()
            self._log = None


def start_servers(names, binaries) -> dict:
    """Start the named servers. Sub-servers come up before the central gateway
    (the gateway forwards to them, so they must be listening first)."""
    order = [n for n in proto.SUB_SERVERS if n in names]
    if "central" in names:
        order.append("central")
    handles: dict[str, ServerHandle] = {}
    try:
        for name in order:
            handles[name] = ServerHandle(name, proto.SERVERS[name]["port"], binaries[name]).start()
    except Exception:
        stop_servers(handles)
        raise
    return handles


def stop_servers(handles) -> None:
    """Stop servers in reverse dependency order (central first, then sub-servers)."""
    for name in ["central"] + proto.SUB_SERVERS:
        if name in handles:
            handles[name].stop()
