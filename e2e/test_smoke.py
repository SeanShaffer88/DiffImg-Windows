"""Smoke tests for DiffImg (Qt 6) — pilot suite for the
windows_desktop_e2e skill.

Covered flows:
1. Bare launch shows the file-selection dialog; cancelling it leaves
   the main window running, and closing the window exits cleanly.
2. Launching with two image files loads them: the window title names
   both files and the image view is present; closing exits with code 0.

These tests run on Windows only (CI: windows-2022, see
.github/workflows/e2e-desktop.yml). A green run is Partially Verified
evidence — the maintainer's on-machine field test remains the verdict
for look and feel.
"""
import pytest

from pages.base_page import BasePage

pytestmark = pytest.mark.smoke


def test_bare_launch_shows_files_dialog(launch):
    app = launch()

    dialog = app.page.find_files_dialog()
    assert dialog is not None, "Files dialog did not appear on bare launch"
    app.page.screenshot("bare_launch_files_dialog")

    # Cancel the dialog: click its Cancel button when one is
    # reachable, else send Escape (a QDialog rejects on Escape).
    dialog_page = BasePage(dialog)
    try:
        dialog_page.click(dialog_page.by_name("Cancel"))
    except Exception:
        import pywinauto.keyboard as kb

        kb.send_keys("{ESC}")

    app.page.wait_until(
        lambda: app.page.find_files_dialog(timeout=1) is None,
        timeout=10,
    )
    assert app.proc.poll() is None, "App exited when the files dialog was cancelled"

    assert app.close_and_wait() == 0


def test_two_images_load_and_title_updates(launch, image_pair):
    left, right = image_pair
    app = launch(left, right)

    title = app.page.wait_title_contains("left.png", "right.png")
    assert "left.png" in title and "right.png" in title

    assert app.page.image_view_present("left.png"), (
        "No image view found (no graphics view element and no "
        "loaded-image tab named left.png)"
    )
    app.page.screenshot("two_images_loaded")

    assert app.close_and_wait() == 0
