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

def attach_fn(_, editable, child_name, parent_name, index):
  rp_parent = editable.document.refpoints[parent_name]
  rp_child = editable.document.refpoints[child_name]
  editable.refpoint_attach(rp_child, rp_parent, index)

def detach_fn(_, editable, child_name):
  editable.refpoint_detach(editable.document.refpoints[child_name])


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

  def get_refpoints(self, include_dragged=True):
    """Return the refpoints associated to this object (including the dragged
    ones)."""
    for rp in self.document.refpoints:
      yield rp
    if include_dragged and self._dragged_refpoints != None:
      for rp in self._dragged_refpoints.initial_points.itervalues():
        yield rp

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
    #line_gc.copy(self.style.white_gc)
    #line_gc.set_function(gtk.gdk.XOR)

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
    self.repaint_rps(rp)

    self.undoer.begin_group()
    self.undoer.record_action(delete_fn, self, real_name)
    if with_cb:
      self._call_back("refpoint_new", self, rp)
    self.undoer.end_group()

    return rp

  def refpoint_attach(self, rp_child, rp_parent, index=None):
    """Attach a reference point rp_child to another reference point rp_parent
    using the given index."""
    v = self.refpoint_set_visibility(rp_child, False)
    self.undoer.record_action(detach_fn, self, rp_child.name)
    rp_parent.attach(rp_child, index=index)
    self.refpoint_set_visibility(rp_child, v)

  def refpoint_detach(self, rp_child):
    """Detach a reference point from its parent."""
    parent_and_index = rp_child.get_parent_and_index()
    if parent_and_index:
      rp_parent, index = parent_and_index
      v = self.refpoint_set_visibility(rp_child, False)
      self.undoer.record_action(attach_fn, self,
                                rp_child.name, rp_parent.name, index)
      rp_child.detach()
      self.refpoint_set_visibility(rp_child, v)

  def refpoint_move(self, rp, coords, use_py_coords=True):
    """Move a reference point to a new position."""
    screen_view = self.get_visible_coords()
    box_coords = (screen_view.pix_to_coord(Point(coords)) if use_py_coords
                  else coords)
    if rp.visible:
      v = self.refpoint_set_visibility(rp, False)
      rp_children = rp.get_children()
      v_children = tuple(self.refpoint_set_visibility(rp_child, False)
                         for rp_child in rp_children)

      self.undoer.record_action(move_fn, self, rp.name, rp.value)
      rp.value = box_coords

      self.refpoint_set_visibility(rp, v)
      for i, rp_child in enumerate(rp_children):
        self.refpoint_set_visibility(rp_child, v_children[i])

  def refpoint_delete(self, rp, force=False):
    """Delete the given reference point."""
    if rp.visible or force:
      self.undoer.begin_group()
      children = rp.get_children()
      for child in children:
        self.refpoint_delete(child, force=True)

      self.refpoint_set_visibility(rp, False)
      self.refpoint_detach(rp)
      self.document.refpoints.remove(rp)
      self.undoer.record_action(create_fn, self, rp.name, rp.value)
      self.undoer.end_group()
      self._call_back("refpoint_delete", self, rp)

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
    self.repaint_rps(*touched_points.values())

  def get_gcontext(self, clip_rect=None):
    args = tuple(self.get_config(val)
                 for val in ("refpoint_size", "refpoint_gc", "refpoint_sel_gc",
                             "refpoint_drag_gc", "refpoint_line_gc"))
    if clip_rect != None:
      new_args = list(args)
      for i, arg in enumerate(args):
        if isinstance(arg, gtk.gdk.GC):
          new_arg = self.window.new_gc()
          new_arg.copy(arg)
          new_arg.set_clip_rectangle(clip_rect)
          new_args[i] = new_arg
      args = new_args

    return GContext(self, *args)

  def _draw_refpoints(self, *rps):
    if len(rps) == 0:
      rps = self.document.refpoints
    view = self.get_visible_coords()
    if view != None:
      context = self.get_gcontext()
      context.draw(rps, view)

  def refpoint_set_visibility(self, rp, show, force=False):
    """Set the state of visibility of the given RefPoint."""
    old_visible = rp.visible
    if old_visible != show or force:
      rp.visible = show
      self.repaint_rps(rp)
    return old_visible

  def refpoints_set_visibility(self, selection, visibility):
    """Set the visibility for a selection of points."""
    rps = self.document.refpoints.set_visibility(selection, visibility)
    self.repaint_rps(*rps)

  def repaint_region(self, region):
    """Repaint a region."""
    if region.width <= 0 or region.height <= 0:
      return

    # First repaint the background.
    view = self.get_visible_coords()
    if view == None:
      return
    BoxViewArea.repaint(self, region.x, region.y, region.width, region.height)

    # Construct the context.
    gc = self.get_gcontext(clip_rect=region)

    # Repaint all objects inside the region layer-by-layer.
    for layer in GContext.layers:
      layers = (layer,)

      # First find all the objects (partially or totally) inside the region.
      rps_inside_region = [rp for rp in self.get_refpoints()
                           if not rp.is_outside(gc, view, region,
                                                layers=layers)]
      if len(rps_inside_region) == 0:
        continue

      # Redraw all the refpoints, using the region as a clipping rectangle.
      gc.draw(rps_inside_region, view, layers=layers)

  def repaint_rps(self, *rps):
    """Repaint the area occupied by the specified refpoints."""

    view = self.get_visible_coords()
    if view == None:
      return

    gc = self.get_gcontext()

    for rp in rps:
      region = rp.get_rectangle(gc, view)
      if region != None:
        self.repaint_region(region)

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
            self.undoer.begin_group()
            rp_child = self.refpoint_new(py_coords, with_cb=False)
            self.refpoint_attach(rp_child, rp)
            self._call_back("refpoint_new", self, rp_child)
            self.undoer.end_group()

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
        self.refpoint_set_visibility(rp, False)
        rp.translate_to(Point(rps[rp_name].value) + box_vec)
        self.refpoint_set_visibility(rp, True)

  def _drag_confirm(self, py_coords):
    drps = self._dragged_refpoints
    rps = self.document.refpoints.selection
    assert drps != None
    screen_view = self.get_visible_coords()
    transform = screen_view.pix_to_coord
    box_vec = (Point(transform(Point(py_coords))) -
               Point(transform(Point(drps.py_initial_pos))))

    self.undoer.begin_group()    
    for rp_name, rp in drps.initial_points.iteritems():
      if rp.visible:
        self.undoer.record_action(move_fn, self, rp_name, rps[rp_name].value)
        rps[rp_name].translate(box_vec)
    self.undoer.end_group()
