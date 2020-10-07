import gi

from renderer import GLRenderer

gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

w = Gtk.Window(title='YOOOOOOO')
w.set_default_size(800, 500)

area = Gtk.GLArea()

r = GLRenderer()
w.add(r.area)

w.connect("delete-event", Gtk.main_quit)
w.show_all()
Gtk.main()
