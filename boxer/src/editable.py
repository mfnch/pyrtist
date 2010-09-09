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

from geom2 import square_metric, Point
import document
from zoomable import ZoomableArea
from boxdraw import BoxImageDrawer
from refpoints import RefPoint

from config import debug

def _cut_point(size, x, y):
  return (max(0, min(size[0]-1, x)), max(0, min(size[1]-1, y)))

def _cut_square(size, x, y, dx, dy):
  x = int(x)
  y = int(y)
  x0, y0 = _cut_point(size, x, y)
  x1, y1 = _cut_point(size, x+dx, y+dy)
  sx, sy = (x1 - x0, y1 - y0)
  if sx < 1 or sy < 1: return (x0, y0, -1, -1)
  return (x0, y0, sx+1, sy+1)

def draw_ref_point(drawable, coords, size, gc):
  drawable_size = drawable.get_size()
  x, y = coords

  l0 = size
  dl0 = l0*2
  x0, y0, dx0, dy0 = _cut_square(drawable_size, x-l0, y-l0, dl0, dl0)
  if dx0 < 1 or dy0 < 1:
    return None

  gc.foreground, gc.background = (gc.background, gc.foreground)
  drawable.draw_rectangle(gc, True, x0, y0, dx0, dy0)
  gc.foreground, gc.background = (gc.background, gc.foreground)

  l1 = l0 - 1
  dl1 = 2*l1
  x1, y1, dx1, dy1 = _cut_square(drawable_size, x-l1, y-l1, dl1, dl1)
  if dx1 > 0 and dy1 > 0:
    drawable.draw_rectangle(gc, True, x1, y1, dx1, dy1)

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


class Configurable(object):
  def __init__(self):
    self._config = {}

  def set_config(self, name, value):
    """Set the configuration option with name 'name' to 'value'."""
    if type(name) == str:
      self._config[name] = value
    else:
      for n, v in name:
        self._config[n] = v

  def set_config_default(self, name, value=None):
    """Set the configuration option only if it is missing."""
    if type(name) == str:
      if name not in self._config:
        self._config[name] = value
    else:
      for n, v in name:
        if n not in self._config:
          self._config[n] = v

  def get_config(self, name, opt=None):
    """Return the value of the configuration option 'name' or 'value'
    if the configuration does not have such an option.
    """
    return self._config[name] if name in self._config else opt


class BoxEditableArea(BoxViewArea, Configurable):
  def __init__(self, *args, **extra_args):
    Configurable.__init__(self)
    BoxViewArea.__init__(self, *args, **extra_args)

    self._dragged_refpoint = None  # RefPoint which is being dragged
    self._fns = {}                 # External handler functions

    self.gc = None

    # Set default configuration
    self.set_config_default([("button_left", 1),
                             ("button_center", 2),
                             ("button_right", 3),
                             ("refpoint_size", 4),
                             ("redraw_on_move", True)])

    # Enable events and connect signals
    mask = (  gtk.gdk.POINTER_MOTION_MASK
            | gtk.gdk.BUTTON_PRESS_MASK
            | gtk.gdk.BUTTON_RELEASE_MASK)
    self.add_events(mask)
    self.connect("realize", self._realize)
    self.connect("button-press-event", self._on_button_press_event)
    self.connect("motion-notify-event", self._on_motion_notify_event)
    self.connect("button-release-event", self._on_button_release_event)

  def _realize(self, myself):
    # Set extra default configuration
    gc = gtk.gdk.GC(self.window)
    gc.copy(self.style.white_gc)
    gc_sel = gtk.gdk.GC(self.window)
    gc_sel.background, gc_sel.foreground = (gc.foreground, gc.background)
    self.set_config_default([("refpoint_gc", gc),
                             ("refpoint_sel_gc", gc_sel)])

  def refpoint_new(self, py_coords, name=None):
    """Add a new reference point whose coordinates are 'point' (a couple
    of floats) and the name is 'name'. If 'name' is not given, then a name
    is generated automatically using the 'base_name' given during construction
    of the class.
    RETURN the name finally chosen for the point.
    """
    screen_view = self.get_visible_coords()
    box_coords = screen_view.pix_to_coord(py_coords)
    refpoints = self.document.refpoints
    real_name = refpoints.new_name(name)
    if real_name in refpoints:
      raise ValueError("A reference point with the same name exists (%s)"
                       % real_name)

    rp = RefPoint(real_name, box_coords)
    refpoints.append(rp)
    self._refpoint_show(rp)
    notify = self._fns.get("notify_refpoint_new", None)
    if notify != None:
      notify(real_name)
    return real_name

  def refpoint_pick(self, py_coords, include_invisible=False):
    """Returns the refpoint closest to the given one (argument 'point').
    If the distance between the two is greater than the current 'radius'
    (see set_radius), return None."""
    screen_view = self.get_visible_coords()
    box_coords = screen_view.pix_to_coord(py_coords)
    refpoints = self.document.refpoints
    current = refpoints.get_nearest(box_coords,
                                    include_invisible=include_invisible)
    s = self.get_config("refpoint_size")
    if s == None:
      return current
    elif current == None:
      return None
    else:
      rp = current[0]
      current_py_coords = screen_view.coord_to_pix(rp.value)
      dist = square_metric(py_coords, current_py_coords)
      if dist <= s:
        return current
      else:
        return None

  def refpoint_select(self, rp):
    refpoints = self.document.refpoints
    if type(rp) == str:
      rp = refpoints[rp]
    if len(refpoints.selection) > 0:
      prev_rp = refpoints.selection[0]
      refpoints.set_selection([rp])
      self._refpoint_show(prev_rp)
      self._refpoint_show(rp)
    else:
      refpoints.set_selection([rp])
      self._refpoint_show(rp)

  def _refpoint_hide(self, rp):
    """Hide the given RefPoint. This is a purely graphical operation, it does
    not change the status of the RefPoint object.
    """
    x, y = self.get_visible_coords().coord_to_pix(rp.value)
    l0 = self.get_config("refpoint_size")
    dl0 = l0*2
    wsize = self.window.get_size()
    x0, y0, dx0, dy0 = _cut_square(wsize, x-l0, y-l0, dl0, dl0)
    if dx0 > 0 and dy0 > 0:
      self.repaint(x0, y0, dx0, dy0)

    magnification = self.last_magnification
    distance = (dl0 + 1)/magnification if magnification != None else None
    overlapping_rps = self.document.refpoints.get_neighbours(rp, distance)
    self.refpoints_draw(overlapping_rps)

  def _refpoint_show(self, rp):
    if rp.visible:
      refpoints = self.document.get_refpoints()
      view = self.get_visible_coords()
      pix_coord = view.coord_to_pix(rp.value)
      rp_size = self.get_config("refpoint_size")
      gc = (self.get_config("refpoint_sel_gc") if refpoints.is_selected(rp)
            else self.get_config("refpoint_gc"))
      draw_ref_point(self.window, pix_coord, rp_size, gc)

  def refpoints_draw(self, rps=None):
    refpoints = self.document.refpoints
    if rps == None:
      rps = refpoints
    view = self.get_visible_coords()
    rp_size = self.get_config("refpoint_size")
    gc_unsel = self.get_config("refpoint_gc")
    gc_sel = self.get_config("refpoint_sel_gc")
    for rp in rps:
      if rp.visible:
        pix_coord = view.coord_to_pix(rp.value)
        gc = gc_sel if refpoints.is_selected(rp) else gc_unsel
        draw_ref_point(self.window, pix_coord, rp_size, gc)

  def refpoint_move(self, rp, py_coords):
    """Move a reference point to a new position."""
    if rp.visible:
      py_coords = Point(py_coords)
      self._refpoint_hide(rp)
      screen_view = self.get_visible_coords()
      box_coords = screen_view.pix_to_coord(py_coords)
      rp.value = box_coords
      self._refpoint_show(rp)

  def refpoint_delete(self, rp):
    """Delete the given reference point."""
    if rp.visible:
      self._refpoint_hide(rp)
      self.document.refpoints.remove(rp)

  def expose(self, draw_area, event):
    ret = ZoomableArea.expose(self, draw_area, event)
    self.refpoints_draw()
    return ret

  def _on_button_press_event(self, eventbox, event):
    """Called when clicking over the DrawingArea."""

    refpoints = self.document.refpoints
    py_coords = event.get_coords()
    picked = self.refpoint_pick(py_coords)
    rp = None
    if picked != None:
      rp, _ = picked
      #self.widget_refpoint_entry.set_text(ref_point.name)

    if event.button == self.get_config("button_left"):
      if rp != None:
        if refpoints.is_selected(rp):
          self._dragged_refpoint = rp
        else:
          self.refpoint_select(rp)

      else:
        box_coords = self.get_visible_coords().pix_to_coord(py_coords)
        if box_coords != None:
          point_name = self.refpoint_new(py_coords)
          self.refpoint_select(point_name)
          #if self.get_paste_on_new():
            #self.textbuffer.insert_at_cursor("%s, " % point_name)

    elif self._dragged_refpoint != None:
      return

    elif event.button == self.get_config("button_center"):
      if rp != None:
        self.textbuffer.insert_at_cursor("%s, " % rp.name)

    elif event.button == self.get_config("button_right"):
      if rp != None:
        self.refpoint_delete(rp)

  def _on_motion_notify_event(self, eventbox, event):
    if self._dragged_refpoint != None:
      py_coords = event.get_coords()
      self.refpoint_move(self._dragged_refpoint, py_coords)

  def _on_button_release_event(self, eventbox, event):
    if self._dragged_refpoint != None:
      py_coords = event.get_coords()
      self.refpoint_move(self._dragged_refpoint, py_coords)
      self._dragged_refpoint = None
      if self.get_config("redraw_on_move"):
        self.buf_needs_update = True
        self.queue_draw()
