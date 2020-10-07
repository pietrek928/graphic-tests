import gi

gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
from gl_renderer import _GLRenderer


class GLRenderer(_GLRenderer):
    def __init__(self):
        super().__init__(Gtk.GLArea())
