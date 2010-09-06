import time
import sys
sys.path.append("../")

import gtk
from zoomable import Point, Rectangle, View, ZoomableArea, ImageDrawer
from boxdraw import BoxImageDrawer
from editable import BoxViewArea, BoxEditableArea

class Config:
  use_threads = False

config = Config()

class Experiment(object):
  def __init__(self, gladefile="boxer.glade", filename=None, box_exec=None):
    builder = gtk.Builder()
    builder.add_from_file("./experiment.glade")
    sigs = {"on_experiment_destroy": gtk.main_quit,
            "on_zoom_inc_clicked": self.zoom_inc_clicked,
            "on_zoom_dec_clicked": self.zoom_dec_clicked,
            "on_zoom_off_clicked": self.zoom_off_clicked}

    builder.connect_signals(sigs)
    self.window = builder.get_object("experiment")

    self.box_window = BoxEditableArea(filename)
    self.scrolledwindow = builder.get_object("scrolledwindow")
    self.scrolledwindow.add(self.box_window)
    self.window.show_all()

  def zoom_inc_clicked(self, *v):
    self.box_window.zoom_in()

  def zoom_dec_clicked(self, *v):
    self.box_window.zoom_out()

  def zoom_off_clicked(self, *v):
    self.box_window.zoom_off()


def run(filename=None, box_exec=None):
  if config.use_threads:
    gtk.gdk.threads_init()
    gtk.gdk.threads_enter()

  main_window = Experiment(filename=filename, box_exec=box_exec)
  gtk.main()

  if config.use_threads:
    gtk.gdk.threads_leave()

run("poly.box")
#run("/home/fnch/mumag/thesis/sketch.box")
