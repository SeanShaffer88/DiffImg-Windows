"""DiffImg-specific page helpers.

Facts these helpers rely on, read from the app source (src/) and
confirmed against the first CI run's failure screenshots:
- `diffimg.exe img1 img2` loads both images; the main window title
  becomes "DiffImg (x64) <version>: (<i>/<n>) [img1] / [img2] (<i>/<n>)"
  (DiffImgWindow::updateTitle).
- With no files, the app shows its FilesDialog (title "Dialog") with
  the lineEditFile1 / lineEditFile2 fields (ui/FilesDialog.ui).
- `--portable` keeps QSettings in INI files next to the exe
  (src/main.cpp), which is what makes a copied build fully isolated.

Locator lesson from run 1 (both tests failed on locators while the
screenshots showed the app working): Qt's UIA tree does not reliably
expose a QGraphicsView, and the owned FilesDialog is not reliably a
separate Desktop top-level window with findable QLineEdit children.
So: search by several strategies, prefer visible text/names that the
user can also see, and dump the real tree on failure (conftest does)
instead of guessing a fourth strategy blind.
"""
import time

from pywinauto import Desktop

import config
from pages.base_page import BasePage


def _descendants(wrapper):
    try:
        return wrapper.descendants()
    except Exception:
        return []


def _auto_id(ctrl):
    try:
        return ctrl.element_info.automation_id or ""
    except Exception:
        return ""


def _class_name(ctrl):
    try:
        return ctrl.class_name() or ""
    except Exception:
        return ""


def _text(ctrl):
    try:
        return ctrl.window_text() or ""
    except Exception:
        return ""


def _is_edit(ctrl):
    if _auto_id(ctrl).startswith("lineEdit"):
        return True
    if "LineEdit" in _class_name(ctrl):
        return True
    try:
        return ctrl.element_info.control_type == "Edit"
    except Exception:
        return False


class MainPage(BasePage):
    @property
    def title(self):
        return self.window.window_text()

    def wait_title_contains(self, *fragments, timeout=None):
        def check():
            title = self.title
            return all(fragment in title for fragment in fragments)

        self.wait_until(check, timeout=timeout or config.LAUNCH_TIMEOUT)
        return self.title

    # --- Image view ---

    def graphics_view(self):
        """The image view, by any UIA shape Qt gives it.

        Matches an AutomationId or class name *containing* the .ui
        objectName, not just exact equality (run 1: exact match found
        nothing while the view was visibly on screen).
        """
        for ctrl in _descendants(self.window):
            if "graphicsview" in _auto_id(ctrl).lower():
                return ctrl
            if "graphicsview" in _class_name(ctrl).lower():
                return ctrl
        return None

    def loaded_image_tab(self, filename):
        """The tab/label carrying a loaded image's file name."""
        for ctrl in _descendants(self.window):
            if _text(ctrl) == filename:
                return ctrl
        return None

    def image_view_present(self, filename=None):
        if self.graphics_view() is not None:
            return True
        return filename is not None and self.loaded_image_tab(filename) is not None

    # --- Files dialog ---

    def _dialog_in(self, root):
        """A FilesDialog under this root: a 'Dialog'-titled element
        that contains at least two edit fields."""
        for ctrl in _descendants(root):
            if _text(ctrl) == "Dialog":
                page = BasePage(ctrl)
                edits = [d for d in _descendants(ctrl) if _is_edit(d)]
                if len(edits) >= 2:
                    return ctrl
        return None

    def find_files_dialog(self, timeout=None):
        """Return the FilesDialog as a wrapper, or None on timeout.

        Strategies, in order:
        1. A top-level window of this process titled "Dialog" whose
           subtree holds the file edit fields.
        2. Any other top-level window of this process whose subtree
           holds two or more edit fields.
        3. A dialog element inside the main window's own UIA subtree
           (owned dialogs can live there instead of top level).
        """
        pid = self.window.process_id()
        deadline = time.time() + (timeout or config.ACTION_TIMEOUT)
        while time.time() < deadline:
            tops = []
            try:
                tops = Desktop(backend="uia").windows()
            except Exception:
                tops = []
            for top in tops:
                try:
                    if top.process_id() != pid or top.handle == self.window.handle:
                        continue
                    if _text(top) == "Dialog" and any(
                        _is_edit(d) for d in _descendants(top)
                    ):
                        return top
                    edits = [d for d in _descendants(top) if _is_edit(d)]
                    if len(edits) >= 2:
                        return top
                except Exception:
                    pass
            found = self._dialog_in(self.window)
            if found is not None:
                return found
            time.sleep(0.3)
        return None
