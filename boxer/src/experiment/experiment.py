# Copyright (C) 2008, 2009, 2010
#  by Matteo Franchin (fnch@users.sourceforge.net)
#
# This file is part of Boxer.
#
#   Boxer is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Boxer is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Boxer.  If not, see <http://www.gnu.org/licenses/>.

import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import gtk.gdk
import os, os.path
import time
import gobject

class Config:
  use_threads = False

config = Config()





class Position(object):
  def __init__(self, x, y):
    self.x = x
    self.y = y
    self.xy = (x, y)

class Size(Position): pass
class CoordSize(Size): pass
class PixSize(Size): pass

class CoordRect(object):
  def __init__(self, corner1, corner2):
    self.corner1 = corner1
    self.corner2 = corner2

  def new_scaled(self, scale_factor):
    hc1x, hc1y = (0.5*self.corner1.x, 0.5*self.corner1.y)
    hc2x, hc2y = (0.5*self.corner2.x, 0.5*self.corner2.y)
    cx, cy = (hc1x + hc2x, hc1y + hc2y)
    wx, wy = ((hc2x - hc1x)*scale_factor, (hc2y - hc1y)*scale_factor)
    c1 = Position(cx - wx, cy - wy)
    c2 = Position(cx + wx, cy + wy)
    return CoordRect(c1, c2)




class BoxImageOutput(object):
  def __init__(self):
    self.size = Size(100.0, 50.0) # Box coordinates

  def update(self, pixbuf_output, pix_view, coord_view=None):
    """This function is called by BoxImageView to update the view (redraw the
    picture). There are two possibilities:
     1. coord_view is not provided. The function knows how many pixel (in both
        x and y directions) are available and has to draw the picture in the
        best possible way in the available space (possibly, increasing the
        image size until the picture hits one of the boundaries);
     2. coord_view is provided. The function has to draw just the provided
        coordinate view of the picture, producing an output filling the whole
        pix_view.
    In both cases the total extent of the figure is returned as a CoordRect.
    """
    pass

# Fake Box image output
class FakeBoxImageOutput(object):
  def __init__(self):
    self.size = Size(100.0, 50.0) # Box coordinates

  def update(self, pixbuf_output, pix_size, coord_view=None):
    # Clear the pixbuf
    pixbuf_output.fill(0)

    # First let's determine the coordinate mapping
    lx, ly = (pix_size.x, pix_size.y)
    if coord_view == None:
      image_aspect = float(self.size.y)/self.size.x
      view_aspect = float(ly)/lx
      if image_aspect > view_aspect:
        # fill vertically
        zoom = ly/self.size.y
      else:
        # fill horizontally
        zoom = lx/self.size.x

      def coord2pix(cx, cy):
        return (int(cx*zoom), int(cy*zoom))

    else:
      v1x = coord_view.corner1.x
      v1y = coord_view.corner1.y
      v2x = coord_view.corner2.x
      v2y = coord_view.corner2.y

      def coord2pix(cx, cy):
        px, py = ((cx - v1x)*lx/(v2x - v1x), (cy - v1y)*ly/(v2y - v1y))
        return (int(min(lx, max(0.0, px))),
                int(min(ly, max(0.0, py))))

    dx, dy = (self.size.x/10.0, self.size.y/10.0)
    for ix in range(10):
      for iy in range(10):
        color = 0x10000*(ix + 1)*25 + 0x100*(iy + 1)*25
        p1x, p1y = coord2pix(ix*dx, iy*dy)
        p2x, p2y = coord2pix(ix*dx + dx, iy*dy + dy)
        px, py = p1x, p1y
        wx, wy = p2x - p1x, p2y - p1y
        subbuf = pixbuf_output.subpixbuf(px, py, wx, wy)
        subbuf.fill(0)
        subbuf = pixbuf_output.subpixbuf(px + 1 , py + 1, wx - 2, wy - 2)
        subbuf.fill(color)

    return CoordRect(Position(0, 0), self.size)



class BoxImageView(gtk.DrawingArea):
  def __init__(self):
    gtk.DrawingArea.__init__(self)
    self.back_width = 2000
    self.back_height = 1000
    self.frame = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, False, 8,
                                self.back_width, self.back_height)

    gobject.signal_new("scroll_adjustment", self.__class__,
                       gobject.SIGNAL_RUN_LAST,
                       gobject.TYPE_NONE,
                       (gtk.Adjustment, gtk.Adjustment))

    self.set_set_scroll_adjustments_signal("scroll_adjustment")

    self.connect("expose_event", self.expose_cb)
    self.connect("scroll_adjustment", self.scroll_adjustment)

    self.drawer = FakeBoxImageOutput()
    self.pic_view = None
    self.pic_bbox = None
    self.zoom_factor = 1.5

  def zoom_in(self):
    if self.pic_view != None:
      self.pic_view = self.pic_view.new_scaled(self.zoom_factor)
    else:
      self.pic_view = self.pic_bbox.new_scaled(self.zoom_factor)
    self.expose_cb()

  def zoom_out(self):
    if self.pic_view != None:
      self.pic_view = self.pic_view.new_scaled(1.0/self.zoom_factor)
    else:
      self.pic_view = self.pic_bbox.new_scaled(1.0/self.zoom_factor)
    self.expose_cb()

  def scroll_adjustment(self, a, b, c):
    print a, b, c

  def get_hadjustment(self):
    return gtk.Adjustment(value=50.0, lower=0.0, upper=150.0, step_incr=10.0,
                          page_incr=50.0, page_size=50.0)

  def get_vadjustment(self):
    return gtk.Adjustment(value=50.0, lower=0.0, upper=150.0, step_incr=10.0,
                          page_incr=50.0, page_size=50.0)

  def set_hadjustment(self, adjustment):
    pass

  def set_vadjustment(self, adjustment):
    pass

  hadjustment = property(get_hadjustment, set_hadjustment)
  vadjustment = property(get_vadjustment, set_vadjustment)

  def set_shadow_type(self, shadow_type):
    pass

  def get_shadow_type(self):
    return None

  def expose_cb(self, draw_area, event):
    ''' Expose callback for the drawing area. '''

    rowstride = self.frame.get_rowstride()

    # FIXME: what should be the result, string guchar an integer result?
    #pixels = frame.get_pixels() + rowstride * event.area.y + event.area.x * 3
    #pixels = frame.get_pixels()[len(frame.get_pixels()) + rowstride * event.area.y + event.area.x * 3]

    coord_view = self.pic_view
    pix_size = PixSize(event.area.width, event.area.height)
    self.pic_bbox = \
      self.drawer.update(self.frame, pix_size, coord_view=coord_view)
    pixels = self.frame.get_pixels()

    draw_area.window.draw_rgb_image(
        draw_area.style.black_gc,
        event.area.x, event.area.y,
        event.area.width, event.area.height,
        'normal',
        pixels, rowstride,
        event.area.x, event.area.y)
    return True

class Experiment(object):
  def __init__(self, gladefile="boxer.glade", filename=None, box_exec=None):
    builder = gtk.Builder()
    builder.add_from_file("./experiment.glade")
    sigs = {"on_experiment_destroy": gtk.main_quit,
            "on_zoom_inc_clicked": self.zoom_inc_clicked,
            "on_zoom_dec_clicked": self.zoom_dec_clicked}

    builder.connect_signals(sigs)
    self.window = builder.get_object("experiment")
    self.box_window = BoxImageView()
    self.scrolledwindow = builder.get_object("scrolledwindow")
    self.scrolledwindow.add(self.box_window)
    self.window.show_all()

  def zoom_inc_clicked(self, *v):
      self.box_window.zoom_in()

  def zoom_dec_clicked(self, *v):
      self.box_window.zoom_out()


def run(filename=None, box_exec=None):
  if config.use_threads:
    gtk.gdk.threads_init()
    gtk.gdk.threads_enter()

  main_window = Experiment(filename=filename, box_exec=box_exec)
  gtk.main()

  if config.use_threads:
    gtk.gdk.threads_leave()

run()
