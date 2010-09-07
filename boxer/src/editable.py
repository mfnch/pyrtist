# Copyright (C) 2010 by Matteo Franchin (fnch@users.sourceforge.net)
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

"""
Here we extend ZoomableArea so that it can show Box pictures, together with
the Boxer reference points and allows the user to insert/move/... them.
"""

import sys

import gtk

from geom2 import Point
import document
from zoomable import ZoomableArea
from boxdraw import BoxImageDrawer

from config import debug

black = gtk.gdk.Color("black")
white = gtk.gdk.Color("white")

def __cut_point(size, x, y):
  return (max(0, min(size[0]-1, x)), max(0, min(size[1]-1, y)))

def __cut_square(size, x, y, dx, dy):
  x = int(x)
  y = int(y)
  x0, y0 = __cut_point(size, x, y)
  x1, y1 = __cut_point(size, x+dx, y+dy)
  sx, sy = (x1 - x0, y1 - y0)
  if sx < 1 or sy < 1: return (x0, y0, -1, -1)
  return (x0, y0, sx+1, sy+1)

def draw_ref_point(drawable, coords, size, gc, bord_gc):
  drawable_size = drawable.get_size()
  x, y = coords

  l0 = size
  dl0 = l0*2
  x0, y0, dx0, dy0 = __cut_square(drawable_size, x-l0, y-l0, dl0, dl0)
  if dx0 < 1 or dy0 < 1:
    return None

  drawable.draw_rectangle(gc, True, x0, y0, dx0, dy0)

  l1 = l0 - 1
  dl1 = 2*l1
  x1, y1, dx1, dy1 = __cut_square(drawable_size, x-l1, y-l1, dl1, dl1)
  if dx1 > 0 and dy1 > 0:
    drawable.draw_rectangle(bord_gc, True, x1, y1, dx1, dy1)

class RefPoint(object):
  def __init__(self, name, visible=True):
    self.coords = None
    self.box_coords = None # not supposed to be changed manually...
    self.background = None # ... use the method ImgView.ref_point_coords_set
    self.name = name
    self.visible = visible

  def get_name(self):
    return self.name

class BoxViewArea(ZoomableArea):
  def __init__(self, filename=None, out_fn=None, **extra_args):
    # Create the Document
    self.document = d = document.Document()
    if filename != None:
      d.load_from_file(filename)

    # Create the Box drawer
    self.drawer = drawer = BoxImageDrawer(d)
    drawer.out_fn = out_fn if out_fn != None else sys.stdout.write

    # Create the ZoomableArea
    ZoomableArea.__init__(self, drawer, **extra_args)

class BoxEditableArea(BoxViewArea):
  def __init__(self, *args, **extra_args):
    BoxViewArea.__init__(self, *args, **extra_args)

    self._dragged_refpoint = None # RefPoint which is being dragged

    self.gc = None

    # Enable events and connect signals
    mask = (  gtk.gdk.POINTER_MOTION_MASK
            | gtk.gdk.BUTTON_PRESS_MASK
            | gtk.gdk.BUTTON_RELEASE_MASK)
    self.add_events(mask)
    self.connect("button-press-event", self._on_button_press_event)
    self.connect("motion-notify-event", self._on_motion_notify_event)
    self.connect("button-release-event", self._on_button_release_event)

  def get_config(self, name, opt=None):
    return opt

  def get_gc(self):
    gc = self.gc
    if gc != None:
      return gc
    else:
      self.gc = gc = gtk.gdk.GC(self.window)
      gc.copy(self.style.white_gc)
      gc.set_background(gtk.gdk.Color(blue=1.0))
      return gc

  def refpoint_pick(self, point, include_invisible=False):
    """Returns the refpoint closest to the given one (argument 'point').
    If the distance between the two is greater than the current 'radius'
    (see set_radius), return None."""
    current = self.document.refpoints.get_nearest(point, include_invisible)
    s = self.get_config("refpoint_size", 4)
    if s == None:
      return current
    elif current == None or current[1] > s:
      return None
    return current

  def refpoints_draw(self):
    refpoints = self.document.get_refpoints()
    view = self.get_visible_coords()
    for refpoint in refpoints:
      pix_coord = view.coord_to_pix(refpoint.value)
      draw_ref_point(self.window, pix_coord, 4,
                     self.style.black_gc, self.get_gc())


  def refpoint_move(self, name, py_coords):
    """Move a reference point to a new position."""
    pass

  def expose(self, draw_area, event):
    ret = ZoomableArea.expose(self, draw_area, event)
    self.refpoints_draw()
    return ret

  def _on_button_press_event(self, eventbox, event):
    """Called when clicking over the DrawingArea."""
    print "click"
    return

    py_coords = event.get_coords()
    picked = self.imgview.pick(py_coords)
    ref_point = None
    if picked != None:
      ref_point, _ = picked
      self.widget_refpoint_entry.set_text(ref_point.get_name())

    if event.button == self.button_left:
      if ref_point != None:
        if ref_point == self.imgview.selected:
          self.dragging_ref_point = ref_point
        else:
          self.imgview.refpoint_select(ref_point)

      else:
        box_coords = self.imgview.map_coords_to_box(py_coords)
        if box_coords != None:
          point_name = self.imgview.ref_point_new(py_coords)
          self.imgview.refpoint_select(point_name)
          if self.get_paste_on_new():
            self.textbuffer.insert_at_cursor("%s, " % point_name)

    elif self.dragging_ref_point != None:
      return

    elif event.button == self.button_center:
      if ref_point != None:
        self.textbuffer.insert_at_cursor("%s, " % ref_point.name)

    elif event.button == self.button_right:
      if ref_point != None:
        self.imgview.ref_point_del(ref_point.name)

  def _on_motion_notify_event(self, eventbox, event):
    if self._dragged_refpoint != None:
      py_coords = event.get_coords()
      self.imgview.ref_point_move(self.dragged_refpoint.name, py_coords)

  def _on_button_release_event(self, eventbox, event):
    if self._dragged_refpoint != None:
      py_coords = event.get_coords()
      self.imgview.ref_point_move(self._dragged_refpoint, py_coords)
      self._dragged_refpoint = None
      #self.menu_run_execute(None)
