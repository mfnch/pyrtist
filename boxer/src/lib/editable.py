# Copyright (C) 2010-2013 by Matteo Franchin (fnch@users.sf.net)
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
from zoomable import ZoomableArea, DrawSucceded, DrawFailed
from boxdraw import BoxImageDrawer
from refpoints import GContext, RefPoint, \
  REFPOINT_UNSELECTED, REFPOINT_SELECTED, REFPOINT_DRAGGED
from undoer import Undoer

from config import Configurable

def _cut_point(size, x, y):
  return (max(0, min(size[0]-1, x)), max(0, min(size[1]-1, y)))

def _cut_square(size, x, y, dx, dy):
  if size == None:
    return (int(x), int(y), dx + 1, dy + 1)
  else:
    x = int(x)
    y = int(y)
    x0, y0 = _cut_point(size, x, y)
    x1, y1 = _cut_point(size, x+dx, y+dy)
    sx, sy = (x1 - x0, y1 - y0)
    if sx < 1 or sy < 1: return (x0, y0, -1, -1)
    return (x0, y0, sx+1, sy+1)


class BoxViewArea(ZoomableArea):
  def __init__(self, filename=None, out_fn=None, callbacks=None, **kwargs):
    # Create the Document
    self.document = d = \
      document.Document(callbacks=callbacks, from_args=kwargs)
    if filename != None:
      d.load_from_file(filename)
    else:
      d.new()

    # Create the Box drawer
    self.drawer = drawer = BoxImageDrawer(d)
    drawer.out_fn = out_fn

    # Create the ZoomableArea
    ZoomableArea.__init__(self, drawer, callbacks=callbacks, **kwargs)

  def get_document(self):
    '''Get the Document object associated to this window.'''
    return self.document


class DraggedPoints(object):
  def __init__(self, points, py_initial_pos):
    self.initial_points = \
      dict((k, v.copy(state=REFPOINT_DRAGGED))
           for k, v in points.iteritems())
    self.py_initial_pos = py_initial_pos


# Functions used by the undoer.
def create_fn(_, editable, name, value):
  editable.refpoint_new(value, name=name, use_py_coords=False, with_cb=False)

def move_fn(_, editable, name, value):
  rp = editable.document.refpoints[name]
  editable.refpoint_move(rp, value, use_py_coords=False)

def delete_fn(_, editable, name):
  editable.refpoint_delete(editable.document.refpoints[name])


class BoxEditableArea(BoxViewArea, Configurable):
  def __init__(self, *args, **kwargs):
    self.undoer = kwargs.pop('undoer', None) or Undoer()

    self._dragged_refpoints = None     # RefPoints which are being dragged
    self._fns = {"refpoint_new": None, # External handler functions
                 "refpoint_delete": None,
                 "refpoint_pick": None,
                 "refpoint_press": None,
                 "refpoint_press_middle": None}

    Configurable.__init__(self, from_args=kwargs)
    BoxViewArea.__init__(self, *args, callbacks=self._fns, **kwargs)

    # Set default configuration
    self.set_config_default(button_left=1, button_center=2, button_right=3,
                            refpoint_size=4, redraw_on_move=True)

    # Enable events and connect signals
    mask = (  gtk.gdk.POINTER_MOTION_MASK
            | gtk.gdk.BUTTON_PRESS_MASK
            | gtk.gdk.BUTTON_RELEASE_MASK)
    self.add_events(mask)
    self.connect("realize", self._realize)
    self.connect("button-press-event", self._on_button_press_event)
    self.connect("motion-notify-event", self._on_motion_notify_event)
    self.connect("button-release-event", self._on_button_release_event)

  def set_callback(self, name, callback):
    """Set the callbacks."""
    if name not in self._fns:
      raise ValueError("Cannot find '%s'. Available callbacks are: %s."
                       % (name, ", ".join(self._fns.keys())))
    else:
      self._fns[name] = callback

  def _call_back(self, name, *args):
    fn = self._fns.get(name, None)
    if fn != None:
      fn(*args)

  def _realize(self, myself):
    # Set extra default configuration
    unsel_gc = self.window.new_gc()
    unsel_gc.copy(self.style.white_gc)

    sel_gc = self.window.new_gc()
    sel_gc.copy(self.style.white_gc)
    colormap = sel_gc.get_colormap()
    sel_gc.foreground = colormap.alloc_color(gtk.gdk.Color(65535, 65535, 0))

    drag_gc = self.window.new_gc()
    drag_gc.copy(self.style.white_gc)
    colormap = drag_gc.get_colormap()
    drag_gc.foreground = \
      colormap.alloc_color(gtk.gdk.Color(32767, 65535, 32767))

    line_gc = self.window.new_gc()
    line_gc.copy(self.style.black_gc)

    self.set_config_default(refpoint_gc=unsel_gc,
                            refpoint_sel_gc=sel_gc,
                            refpoint_drag_gc=drag_gc,
                            refpoint_line_gc=line_gc)

  def refpoint_new(self, coords, name=None, use_py_coords=True, with_cb=True):
    """Add a new reference point whose coordinates are 'point' (a couple
    of floats) and the name is 'name'. If 'name' is not given, then a name
    is generated automatically using the 'base_name' given during construction
    of the class.
    RETURN the name finally chosen for the point.
    """
    screen_view = self.get_visible_coords()
    box_coords = (screen_view.pix_to_coord(coords) if use_py_coords
                  else coords)
    refpoints = self.document.refpoints
    real_name = refpoints.new_name(name)
    if real_name in refpoints:
      raise ValueError("A reference point with the same name exists (%s)"
                       % real_name)

    rp = RefPoint(real_name, box_coords)
    refpoints.append(rp)
    self._refpoint_show(rp)

    self.undoer.begin_group()
    self.undoer.record_action(delete_fn, self, real_name)
    if with_cb:
      self._call_back("refpoint_new", self, rp)

    self.undoer.end_group()

    return rp

  def refpoint_pick(self, py_coords, include_invisible=False):
    """Returns the refpoint closest to the given one (argument 'point').
    If the distance between the two is greater than the current 'radius'
    (see set_radius), return None."""
    screen_view = self.get_visible_coords()
    if screen_view:
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
        return current if dist <= s else None
    else:
      return None

  def refpoint_select(self, rp, add=False):
    refpoints = self.document.refpoints
    if type(rp) == str:
      rp = refpoints[rp]

    touched_points = dict(refpoints.selection)
    if add:
      refpoints.select(rp, flip=True)
    else:
      refpoints.clear_selection()
      refpoints.select(rp)
    touched_points.update(refpoints.selection)
    self._refpoint_show(*touched_points.values())

  def _refpoint_hide(self, *rps):
    """Hide the given RefPoint. This is a purely graphical operation, it does
    not change the status of the RefPoint object.
    """
    visible_coords = self.get_visible_coords()
    if visible_coords != None:
      l0 = self.get_config("refpoint_size")
      dl0 = l0*2
      wsize = self.window.get_size()
      magnification = self.last_magnification
      distance = ((dl0 + 1)/magnification if magnification != None else None)
      for rp in rps:
        x, y = visible_coords.coord_to_pix(rp.value)
        x0, y0, dx0, dy0 = _cut_square(wsize, x-l0, y-l0, dl0, dl0)
        if dx0 > 0 and dy0 > 0:
          self.repaint(x0, y0, dx0, dy0)
        overlapping_rps = self.document.refpoints.get_neighbours(rp, distance)
        self._draw_refpoints(overlapping_rps)

  def _refpoint_show(self, *rps):
    return self._draw_refpoints(rps)

  def _draw_refpoints(self, rps=None):
    refpoints = self.document.refpoints
    if rps == None:
      rps = refpoints
    view = self.get_visible_coords()
    if view != None:
      rp_size = self.get_config("refpoint_size")
      gc_unsel = self.get_config("refpoint_gc")
      gc_sel = self.get_config("refpoint_sel_gc")
      gc_drag = self.get_config("refpoint_drag_gc")
      gc_line = self.get_config("refpoint_line_gc")
      context = GContext(self.window, gc_unsel, gc_sel, gc_drag, gc_line)
      for rp in rps:
        rp.draw(context, view, rp_size)

  def refpoint_move(self, rp, coords, use_py_coords=True):
    """Move a reference point to a new position."""
    screen_view = self.get_visible_coords()
    box_coords = (screen_view.pix_to_coord(Point(coords)) if use_py_coords
                  else coords)
    if rp.visible:
      self._refpoint_hide(rp)
      self.undoer.record_action(move_fn, self, rp.name, rp.value)
      rp.value = box_coords
      self._refpoint_show(rp)

  def refpoint_delete(self, rp):
    """Delete the given reference point."""
    if rp.visible:
      removed_rps = self.document.refpoints.remove(rp)
      for removed_rp in removed_rps:
        self.undoer.record_action(create_fn, self,
                                  removed_rp.name, removed_rp.value)
        self._refpoint_hide(removed_rp)

      self._call_back("refpoint_delete", self, rp)

  def refpoint_set_visibility(self, rp, show):
    """Set the state of visibility of the given RefPoint."""
    if rp.visible == show:
      return
    else:
      self.document.refpoints.set_visibility(rp, show)
      if show:
        self._refpoint_show(rp)
      else:
        self._refpoint_hide(rp)

  def refpoints_set_visibility(self, selection, visibility):
    rps = self.document.refpoints.set_visibility(selection, visibility)
    for rp in rps:
      if rp.visible:
        self._refpoint_show(rp)
      else:
        self._refpoint_hide(rp)

  def expose(self, draw_area, event):
    ret = ZoomableArea.expose(self, draw_area, event)
    if isinstance(self.drawer_state, (DrawSucceded, DrawFailed)):
      self._draw_refpoints()
    return ret

  def _on_button_press_event(self, eventbox, event):
    """Called when clicking over the DrawingArea."""

    # If we don't have Box coordinates we just ignore all click events
    if not self.has_box_coords():
      return

    refpoints = self.document.refpoints
    py_coords = event.get_coords()
    picked = self.refpoint_pick(py_coords)
    state = event.get_state()
    rp = None
    if picked != None:
      rp = picked[0]
      self._call_back("refpoint_pick", self, rp)

    shift_pressed = (state & gtk.gdk.SHIFT_MASK)
    ctrl_pressed = (state & gtk.gdk.CONTROL_MASK)
    if event.button == self.get_config("button_left"):      
      if rp != None:
        if ctrl_pressed and not shift_pressed and rp.can_procreate():
          visible_coords = self.get_visible_coords()
          box_coords = visible_coords.pix_to_coord(py_coords)
          if box_coords != None:
            rp_child = self.refpoint_new(py_coords)
            rp.make_parent_of(rp_child)
            self.refpoint_select(rp_child)
            self._dragged_refpoints = \
              DraggedPoints(refpoints.selection, py_coords)
            self._call_back("refpoint_press", rp_child)
        elif refpoints.is_selected(rp) and not shift_pressed:
          self._dragged_refpoints = \
            DraggedPoints(refpoints.selection, py_coords)
        else:
          self.refpoint_select(rp, add=shift_pressed)

      elif not shift_pressed:
        #  ^^^ don't want to create a new point while we are selecting others
        visible_coords = self.get_visible_coords()
        box_coords = visible_coords.pix_to_coord(py_coords)
        if box_coords != None:
          rp = self.refpoint_new(py_coords)
          self.refpoint_select(rp)
          self._call_back("refpoint_press", rp)

    elif self._dragged_refpoints != None:
      return

    elif event.button == self.get_config("button_center"):
      if rp != None:
        self._call_back("refpoint_press_middle", self, rp)

    elif event.button == self.get_config("button_right"):
      if rp != None:
        self.refpoint_delete(rp)

  def _on_motion_notify_event(self, eventbox, event):
    if self._dragged_refpoints != None:
      py_coords = event.get_coords()
      self._drag_refpoints(py_coords)

  def _on_button_release_event(self, eventbox, event):
    if self._dragged_refpoints != None:
      py_coords = event.get_coords()
      self._drag_refpoints(py_coords)
      if event.button == self.get_config("button_left"):
        self._drag_confirm(py_coords)
      self._dragged_refpoints = None
      if self.get_config("redraw_on_move"):
        self.buf_needs_update = True
        self.queue_draw()

  def _drag_refpoints(self, py_coords):
    drps = self._dragged_refpoints
    rps = self.document.refpoints.selection
    assert drps != None
    screen_view = self.get_visible_coords()
    transform = screen_view.pix_to_coord
    box_vec = (Point(transform(Point(py_coords))) -
               Point(transform(Point(drps.py_initial_pos))))
    for rp_name, rp in drps.initial_points.iteritems():
      if rp.visible:
        self._refpoint_hide(rp)
        rp.translate_to(Point(rps[rp_name].value) + box_vec)
        self._refpoint_show(rp)

  def _drag_confirm(self, py_coords):
    drps = self._dragged_refpoints
    rps = self.document.refpoints.selection
    assert drps != None
    screen_view = self.get_visible_coords()
    transform = screen_view.pix_to_coord
    box_vec = (Point(transform(Point(py_coords))) -
               Point(transform(Point(drps.py_initial_pos))))
    for rp_name, rp in drps.initial_points.iteritems():
      if rp.visible:
        self.undoer.record_action(move_fn, self, rp_name, rps[rp_name].value)
        rps[rp_name].translate(box_vec)
