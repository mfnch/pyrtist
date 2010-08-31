import sys
sys.path.append("../")

import gtk
from zoomable import Point, Rectangle, View, ZoomableArea, ImageDrawer
import time

# Fake Box image output
class MyDrawerTest(ImageDrawer):
  def __init__(self):
    self.size = Point(100.0, 50.0) # Box coordinates
    self.view = View()

  def update(self, pixbuf_output, pix_size, coord_view=None):
    # We simulate the situation where the test picture is expensive to draw
    # The zoomable object must use buffering when possible!
    #time.sleep(1)

    # First let's determine the coordinate mapping
    if coord_view == None:
      self.view.show(Point(0.0, 50.0), Point(100.0, 0.0),
                     size_in_pixels=pix_size)
      coord_view = self.view
    else:
      self.view.reset(pix_size, coord_view.corner1, coord_view.corner2)

    # Clear the pixbuf
    pixbuf_output.fill(0xffffff00)

    view = self.view
    dx, dy = (self.size.x/10.0, self.size.y/10.0)
    for ix in range(10):
      for iy in range(10):
        color = 0x10000*(ix + 1)*25 + 0x100*(iy + 1)*25
        p1x, p1y = view.valid_coord_to_pix((ix*dx, iy*dy))
        p2x, p2y = view.valid_coord_to_pix((ix*dx + dx, iy*dy + dy))
        px, wx = (p1x, p2x - p1x) if p1x < p2x else (p2x, p1x - p2x)
        py, wy = (p1y, p2y - p1y) if p1y < p2y else (p2y, p1y - p2y)
        if wx > 2 and wy > 2:
          subbuf = pixbuf_output.subpixbuf(px, py, wx, wy)
          subbuf.fill(0x77777700)
          subbuf = pixbuf_output.subpixbuf(px + 1 , py + 1, wx - 2, wy - 2)
          subbuf.fill(color)

    return (Rectangle(Point(0.0, 50.0), Point(100.0, 0.0)), coord_view)


import document

class MyDrawer(ImageDrawer):
  def __init__(self, filename):
    self.document = d = document.Document()
    d.load_from_file(filename)
    
    self.size = Point(100.0, 50.0) # Box coordinates
    self.view = View()

  def update(self, pixbuf_output, pix_size, coord_view=None):
    # We simulate the situation where the test picture is expensive to draw
    # The zoomable object must use buffering when possible!
    #time.sleep(1)

    # First let's determine the coordinate mapping
    if coord_view == None:
      self.view.show(Point(0.0, 50.0), Point(100.0, 0.0),
                     size_in_pixels=pix_size)
      coord_view = self.view
    else:
      self.view.reset(pix_size, coord_view.corner1, coord_view.corner2)

    # Clear the pixbuf
    pixbuf_output.fill(0xffffff00)

    view = self.view
    dx, dy = (self.size.x/10.0, self.size.y/10.0)
    for ix in range(10):
      for iy in range(10):
        color = 0x10000*(ix + 1)*25 + 0x100*(iy + 1)*25
        p1x, p1y = view.valid_coord_to_pix((ix*dx, iy*dy))
        p2x, p2y = view.valid_coord_to_pix((ix*dx + dx, iy*dy + dy))
        px, wx = (p1x, p2x - p1x) if p1x < p2x else (p2x, p1x - p2x)
        py, wy = (p1y, p2y - p1y) if p1y < p2y else (p2y, p1y - p2y)
        if wx > 2 and wy > 2:
          subbuf = pixbuf_output.subpixbuf(px, py, wx, wy)
          subbuf.fill(0x77777700)
          subbuf = pixbuf_output.subpixbuf(px + 1 , py + 1, wx - 2, wy - 2)
          subbuf.fill(color)

    return (Rectangle(Point(0.0, 50.0), Point(100.0, 0.0)), coord_view)

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

    drawer = MyDrawer(filename)
    self.box_window = ZoomableArea(drawer)
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
