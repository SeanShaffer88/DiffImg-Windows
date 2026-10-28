"""DiffImg-specific page helpers.

Facts these helpers rely on, read from the app source (src/):
- `diffimg.exe img1 img2` loads both images; the main window title
  becomes "DiffImg (x64) <version>: (<i>/<n>) [img1] / [img2] (<i>/<n>)"
  (DiffImgWindow::updateTitle).
- With no files, the app shows its FilesDialog (a modeless QDialog
  child window) holding the lineEditFile1 / lineEditFile2 fields
  (ui/FilesDialog.ui objectNames).
- `--portable` keeps QSettings in INI files next to the exe
  (src/main.cpp), which is what makes a copied build fully isolated.
"""
import time

from pywinauto import Desktop

import config
from pages.base_page import BasePage


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

    def graphics_view(self):
        """The first image view (objectName graphicsView1 in the .ui)."""
        return self.find_first(auto_id="graphicsView1", class_name="QGraphicsView")

    def find_files_dialog(self, timeout=None):
        """Return the FilesDialog top-level window, or None on timeout.

        The dialog is a separate top-level window owned by the app
        process; it is identified by the file-path edit fields inside
        it (objectName lineEditFile1, class QLineEdit).
        """
        pid = self.window.process_id()
        deadline = time.time() + (timeout or config.ACTION_TIMEOUT)
        main_hwnd = self.window.handle
        while time.time() < deadline:
            for top in Desktop(backend="uia").windows():
                try:
                    if top.process_id() != pid or top.handle == main_hwnd:
                        continue
                    page = BasePage(top)
                    if page.find_first(
                        auto_id="lineEditFile1", class_name="QLineEdit"
                    ) is not None:
                        return top
                except Exception:
                    pass
            time.sleep(0.3)
        return None
