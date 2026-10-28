"""Base page object: locators, waits, and actions over pywinauto UIA.

Adapted from the windows_desktop_e2e skill (source: ECC, MIT).
Locator priority is AutomationId > Name > ClassName + index.
"""
import os
import time

from pywinauto import Desktop

import config


class BasePage:
    def __init__(self, window):
        self.window = window

    # --- Locators (priority order) ---

    def by_id(self, auto_id, **kw):
        """AutomationId — most stable. Use as first choice."""
        return self.window.child_window(auto_id=auto_id, **kw)

    def by_name(self, name, **kw):
        """Visible text / accessible name."""
        return self.window.child_window(title=name, **kw)

    def by_class(self, cls, index=0, **kw):
        """Control class + index — fragile, avoid if possible."""
        return self.window.child_window(class_name=cls, found_index=index, **kw)

    def find_first(self, auto_id=None, class_name=None):
        """First descendant matching an AutomationId, else a class name.

        Qt exposes widget objectName as AutomationId in its UIA provider;
        when a build does not, fall back to the Qt class name. Returns a
        wrapper object or None.
        """
        try:
            descendants = self.window.descendants()
        except Exception:
            return None
        if auto_id:
            for ctrl in descendants:
                try:
                    if ctrl.element_info.automation_id == auto_id:
                        return ctrl
                except Exception:
                    pass
        if class_name:
            for ctrl in descendants:
                try:
                    if ctrl.class_name() == class_name:
                        return ctrl
                except Exception:
                    pass
        return None

    # --- Waits ---

    def wait_visible(self, spec, timeout=None):
        spec.wait("visible", timeout=timeout or config.ACTION_TIMEOUT)
        return spec

    def wait_gone(self, spec, timeout=None):
        spec.wait_not("visible", timeout=timeout or config.ACTION_TIMEOUT)
        return spec

    def wait_window(self, title_re, timeout=None):
        """Wait for a new top-level window whose title matches a regex."""
        dlg = Desktop(backend="uia").window(title_re=title_re)
        dlg.wait("visible", timeout=timeout or config.ACTION_TIMEOUT)
        return dlg

    @staticmethod
    def wait_until(fn, timeout=None, interval=0.3):
        """Poll an arbitrary condition — use when UIA events are unreliable."""
        timeout = timeout or config.ACTION_TIMEOUT
        deadline = time.time() + timeout
        while time.time() < deadline:
            try:
                if fn():
                    return True
            except Exception:
                pass
            time.sleep(interval)
        raise TimeoutError(f"Condition not met within {timeout}s")

    # --- Actions ---

    def click(self, spec):
        self.wait_visible(spec)
        spec.click_input()

    def type_text(self, spec, text):
        self.wait_visible(spec)
        ctrl = spec.wrapper_object()
        try:
            ctrl.set_edit_text(text)
        except Exception:
            # Qt fallback: UIA Value Pattern may be incomplete on some
            # widgets — click and type through the keyboard instead.
            import pywinauto.keyboard as kb

            ctrl.click_input()
            kb.send_keys("^a")
            kb.send_keys(text, with_spaces=True)

    def get_text(self, spec):
        ctrl = spec.wrapper_object()
        for attr in ("window_text", "get_value"):
            try:
                value = getattr(ctrl, attr)()
                if value:
                    return value
            except Exception:
                pass
        return ""

    # --- Artifacts ---

    def screenshot(self, name):
        os.makedirs(config.ARTIFACT_DIR, exist_ok=True)
        path = os.path.join(config.ARTIFACT_DIR, f"{name}.png")
        self.window.capture_as_image().save(path)
        return path
