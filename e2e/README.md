# DiffImg desktop E2E smoke tests

GUI end-to-end tests for DiffImg, written with [pywinauto](https://pywinauto.readthedocs.io/) against the Windows UI Automation tree (Qt 6 exposes one by default). Pilot suite for the `windows_desktop_e2e` skill.

## What is covered

- Bare launch: the file-selection dialog appears; cancelling it leaves the app running; closing the main window exits with code 0.
- Launch with two image files: both load, the window title names both files, an image view is present, and closing exits with code 0.

Tests are isolated: the portable package is copied to a temp folder, the app runs with `--portable` (settings stay next to the copied exe), and per-user folders are redirected per test. The shipped build and the real user profile are never touched.

## Run locally (Windows)

You need a built portable package — either the `DiffImg-<version>-win-x64` folder from a release ZIP, or the `staging/` output of `tools/package_windows.py`.

```powershell
pip install -r e2e/requirements.txt
$env:DIFFIMG_APP_DIR = "C:\path\to\DiffImg-2.6.0-win-x64"
python -m pytest e2e -v
```

Screenshots and an HTML report land in `e2e/artifacts/` (failure screenshots are automatic). Do not commit or publish artifacts — they can show whatever was on screen.

## CI

`.github/workflows/e2e-desktop.yml` builds the app the same way as the existing Windows workflow, packages it, and runs this suite on `windows-2022`. Like the existing workflow it runs on pushes to `master` and on manual dispatch — not on pull requests.

## Evidence note

A green run proves the covered flows work under automation on a clean Windows machine. It does not replace the maintainer's on-machine field test for look, feel, and display-scaling behaviour.
