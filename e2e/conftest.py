"""Fixtures for the DiffImg E2E smoke suite.

Isolation follows the windows_desktop_e2e skill, Tier 1:
- the portable package is copied once per session into a temp folder,
  so tests never run (or write settings into) the shipped build;
- every launch passes --portable, so the app's QSettings INI files
  land next to the copied exe instead of in the real user profile;
- APPDATA / LOCALAPPDATA / TEMP are redirected per test as a second
  layer for anything the app stores outside its settings.
"""
import os
import shutil
import subprocess
import sys

# Qt 6 enables accessibility by default; setting this also covers Qt 5.
os.environ.setdefault("QT_ACCESSIBILITY", "1")

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import pytest
from pywinauto import Application

import config
from pages.main_page import MainPage


def _app_dir():
    if not config.APP_DIR:
        pytest.exit(
            "DIFFIMG_APP_DIR is not set — point it at the portable "
            "package folder that contains diffimg.exe",
            returncode=1,
        )
    exe = os.path.join(config.APP_DIR, config.APP_EXE_NAME)
    if not os.path.isfile(exe):
        pytest.exit(f"diffimg.exe not found in DIFFIMG_APP_DIR: {config.APP_DIR}",
                    returncode=1)
    return config.APP_DIR


@pytest.fixture(scope="session")
def package_copy(tmp_path_factory):
    """One isolated copy of the portable package for the whole run."""
    dest = tmp_path_factory.mktemp("diffimg-app") / "app"
    shutil.copytree(_app_dir(), dest)
    return dest


class LaunchedApp:
    def __init__(self, proc, window):
        self.proc = proc
        self.window = window
        self.page = MainPage(window)

    def close_and_wait(self, timeout=10):
        """Close the main window; return the process exit code."""
        try:
            self.window.close()
        except Exception:
            pass
        return self.proc.wait(timeout=timeout)


@pytest.fixture
def launch(request, package_copy, tmp_path):
    """Launch the copied app in a per-test sandbox; returns a launcher."""
    launched = []

    def _launch(*args):
        sandbox_env = os.environ.copy()
        sandbox_env["QT_ACCESSIBILITY"] = "1"
        sandbox_env["APPDATA"] = str(tmp_path / "AppData" / "Roaming")
        sandbox_env["LOCALAPPDATA"] = str(tmp_path / "AppData" / "Local")
        sandbox_env["TEMP"] = sandbox_env["TMP"] = str(tmp_path / "Temp")
        for path in (sandbox_env["APPDATA"], sandbox_env["LOCALAPPDATA"],
                     sandbox_env["TEMP"]):
            os.makedirs(path, exist_ok=True)

        exe = str(package_copy / config.APP_EXE_NAME)
        proc = subprocess.Popen(
            [exe, "--portable", *[str(a) for a in args]],
            cwd=str(package_copy),
            env=sandbox_env,
        )
        pw_app = Application(backend="uia").connect(
            process=proc.pid, timeout=config.LAUNCH_TIMEOUT
        )
        win = pw_app.window(title_re=config.MAIN_TITLE_RE)
        win.wait("visible", timeout=config.LAUNCH_TIMEOUT)
        app = LaunchedApp(proc, win)
        launched.append(app)
        return app

    yield _launch

    # Failure screenshot + UIA tree dump, then graceful exit with a
    # force-kill fallback. The tree dump is what turns the next
    # locator failure from a guess into a read.
    failed = getattr(getattr(request.node, "rep_call", None), "failed", False)
    for app in launched:
        if failed:
            try:
                app.page.screenshot(f"FAIL_{request.node.name}")
            except Exception:
                pass
            try:
                import contextlib
                import io

                from pywinauto import Desktop

                os.makedirs(config.ARTIFACT_DIR, exist_ok=True)
                buf = io.StringIO()
                with contextlib.redirect_stdout(buf):
                    try:
                        app.window.print_control_identifiers()
                    except Exception as exc:
                        print(f"<main window tree failed: {exc}>")
                    pid = app.window.process_id()
                    for top in Desktop(backend="uia").windows():
                        try:
                            if top.process_id() == pid and top.handle != app.window.handle:
                                print(f"\n===== top-level: {top.window_text()!r} =====")
                                top.print_control_identifiers()
                        except Exception:
                            pass
                path = os.path.join(
                    config.ARTIFACT_DIR, f"FAIL_{request.node.name}_uiatree.txt"
                )
                with open(path, "w", encoding="utf-8") as fh:
                    fh.write(buf.getvalue())
            except Exception:
                pass
        try:
            app.proc.wait(timeout=1)
        except Exception:
            try:
                app.window.close()
                app.proc.wait(timeout=5)
            except Exception:
                app.proc.kill()


@pytest.hookimpl(wrapper=True)
def pytest_runtest_makereport(item, call):
    result = yield
    setattr(item, f"rep_{result.when}", result)
    return result


@pytest.fixture
def image_pair(tmp_path):
    """Two same-size PNGs that differ in one block of pixels."""
    from PIL import Image

    left = tmp_path / "left.png"
    right = tmp_path / "right.png"
    Image.new("RGB", (64, 64), (30, 30, 30)).save(left)
    img = Image.new("RGB", (64, 64), (30, 30, 30))
    for x in range(16, 32):
        for y in range(16, 32):
            img.putpixel((x, y), (200, 60, 60))
    img.save(right)
    return str(left), str(right)
