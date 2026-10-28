"""E2E configuration for DiffImg desktop smoke tests.

Everything is driven by environment variables so the same suite runs
on a developer PC (against an unzipped portable build) and in CI
(against the freshly packaged staging folder).
"""
import os

# Folder that contains diffimg.exe (the portable package folder).
APP_DIR = os.environ.get("DIFFIMG_APP_DIR", "")
APP_EXE_NAME = "diffimg.exe"

LAUNCH_TIMEOUT = int(os.environ.get("LAUNCH_TIMEOUT", "20"))
ACTION_TIMEOUT = int(os.environ.get("ACTION_TIMEOUT", "15"))

ARTIFACT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "artifacts")

# Main window title is "DiffImg (x64) <version>..." on x64 builds.
MAIN_TITLE_RE = r"DiffImg.*"
