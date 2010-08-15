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


# Fake Box image output
class BoxImageOutput(object):
  def __init__(self):
    self.size = Size(100.0, 50.0) # Box coordinates

  def update(self, coord_top_left, coord_bot_right):
    pass

class BoxImageView(gtk.DrawingArea):
  def __init__(self):
    gtk.DrawingArea.__init__(self)
    self.back_width = 200
    self.back_height = 400
    self.frame = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, False, 8,
                                self.back_width, self.back_height)

    gobject.signal_new("xxx", self.__class__,
                       gobject.SIGNAL_RUN_LAST,
                       gobject.TYPE_NONE,
                       (gtk.Adjustment, gtk.Adjustment))

    self.set_set_scroll_adjustments_signal("xxx")

    self.connect("expose_event", self.expose_cb)
    self.connect("xxx", self.xxx)

  def xxx(self, a, b, c):
    print a, b, c

  def get_hadjustment(self):
    return gtk.Adjustment(value=50, lower=10, upper=60, step_incr=10,
                          page_incr=50, page_size=200)

  def get_vadjustment(self):
    return gtk.Adjustment(value=50, lower=10, upper=60, step_incr=10,
                          page_incr=50, page_size=200)

  def set_hadjustment(self, adjustment):
    pass

  def set_vadjustment(self, adjustment):
    pass

  def set_shadow_type(self, shadow_type):
    pass

  def get_shadow_type(self):
    return None

  def expose_cb(self, draw_area, event):
    ''' Expose callback for the drawing area. '''
    print "expose_cb: entering"
    rowstride = self.frame.get_rowstride()

    # FIXME: what should be the result, string guchar an integer result?
    #pixels = frame.get_pixels() + rowstride * event.area.y + event.area.x * 3
    #pixels = frame.get_pixels()[len(frame.get_pixels()) + rowstride * event.area.y + event.area.x * 3]
    pixels = self.frame.get_pixels()

    draw_area.window.draw_rgb_image(
        draw_area.style.black_gc,
        event.area.x, event.area.y,
        event.area.width, event.area.height,
        'normal',
        pixels, rowstride,
        event.area.x, event.area.y)

    print "expose_cb: leaving"
    return True

class Experiment(object):
  def __init__(self, gladefile="boxer.glade", filename=None, box_exec=None):
    builder = gtk.Builder()
    builder.add_from_file("./experiment.glade")
    sigs = {"on_experiment_destroy": gtk.main_quit}

    builder.connect_signals(sigs)
    self.window = builder.get_object("experiment")
    self.box_window = BoxImageView()
    self.scrolledwindow = builder.get_object("scrolledwindow")
    self.scrolledwindow.add(self.box_window)
    self.window.show_all()


def run(filename=None, box_exec=None):
  if config.use_threads:
    gtk.gdk.threads_init()
    gtk.gdk.threads_enter()

  main_window = Experiment(filename=filename, box_exec=box_exec)
  gtk.main()

  if config.use_threads:
    gtk.gdk.threads_leave()

run()
