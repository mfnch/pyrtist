# Copyright (C) 2010-2013, 2017 by Matteo Franchin (fnch@users.sf.net)
#
# This file is part of Pyrtist.
#
#   Pyrtist is free software: you can redistribute it and/or modify it
#   under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Pyrtist is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

import math
import fnmatch

from gtk.gdk import Rectangle

from .geom2 import square_metric, Point, rectangles_overlap
from .renderer import draw_square, draw_circle, cut_square
from . import namegen
from .callbacks import Callbacks


class GContext(object):
  layers = (0, 1)

  def __init__(self, drawable, rp_size,
               gc_unsel, gc_sel, gc_drag, gc_line):
    self.drawable = drawable
    self.rp_size = rp_size
    self.gc_unsel = gc_unsel
    self.gc_sel = gc_sel
    self.gc_drag = gc_drag
    self.gc_line = gc_line
    self.layer = 0

  def draw(self, objs, *args, **kwargs):
    """Draw the given objects."""
    layers = kwargs.pop('layers', GContext.layers)
    all_args = (self,) + args
    for layer in layers:
      self.layer = layer
      for obj in objs:
        obj.draw(*all_args)


(REFPOINT_UNSELECTED,
 REFPOINT_SELECTED,
 REFPOINT_DRAGGED) = range(3)

(REFPOINT_LONELY,
 REFPOINT_PARENT,
 REFPOINT_CHILD) = range(3)


class RefPoint(object):
  def __init__(self, name, value=None, visible=True,
               kind=REFPOINT_LONELY, selected=REFPOINT_UNSELECTED):
    self.kind = kind
    self.related = None
    self.name = name
    self.value = value
    self.visible = visible
    self.selected = selected

  def split_name(self):
    """Split the reference name into a literal prefix and a numerical index."""
    name = self.name
    n = -1
    try:
      while name[n].isdigit():
        n -= 1
    except IndexError:
      return (name, 1)
    else:
      n += 1
      return (name[:n], int(name[n:]))

  def __cmp__(self, rhs):
    if rhs is None:
      return 1
    lhs_name, lhs_idx = self.split_name()
    rhs_name, rhs_idx = rhs.split_name()
    return cmp(lhs_name, rhs_name) or cmp(lhs_idx, rhs_idx)

  def copy(self, state=None, deep=False):
    state = (state if state is not None else self.selected)
    rp = RefPoint(self.name, self.value, self.visible, self.kind, state)

    if self.related is None or not deep:
      return rp

    if self.kind == REFPOINT_PARENT:
      rp.related = list(self.related)
      for i, child in enumerate(self.related):
        if child is not None:
          rp.related[i] = cp = child.copy(deep=False)
          cp.related = rp
    elif self.kind == REFPOINT_CHILD:
      raise NotImplementedError('Deep copy of child refpoints not implemented')
    return rp

  def is_child(self):
    """Whether this is a child refpoint."""
    return self.kind == REFPOINT_CHILD

  def is_parent(self):
    """Whether this is a parent refpoint."""
    return self.kind == REFPOINT_PARENT

  def can_procreate(self):
    """Whether this reference point can have any children."""
    return (self.kind != REFPOINT_CHILD
            and (self.related is None
                 or len(self.related) < 2
                 or None in self.related))

  def get_state(self):
    return self.selected

  def get_children(self):
    """Get the children point of this refpoint."""
    if self.kind == REFPOINT_PARENT and self.related:
      return self.related
    return []

  def get_parent_and_index(self):
    """Get the parent of this direction point."""
    if self.kind == REFPOINT_CHILD:
      parent = self.related
      index = parent.get_children().index(self)
      return (parent, index)
    return None

  def attach(self, rp, index=None):
    """Mark this reference point as a parent of the given reference points."""
    assert self.kind != REFPOINT_CHILD
    self.related = children = self.related or []
    self.kind = REFPOINT_PARENT
    if rp is not None:
      rp.kind = REFPOINT_CHILD
      rp.related = self

    if index is None:
      index = children.index(None) if None in children else len(children)

    assert index < 2
    while index >= len(children):
      children.append(None)

    children[index] = rp

  def detach(self):
    """Detach this children from its parent."""
    if self.kind == REFPOINT_CHILD:
      parent, index = self.get_parent_and_index()
      children = parent.related
      children[index] = None
      self.kind = REFPOINT_LONELY

      # If the parent is left with no children, then de-parentise it.
      if children.count(None) == len(children):
        parent.kind = REFPOINT_LONELY

  def translate(self, vec):
    """Translate the reference point by the given amount."""
    self.value = Point(self.value) + vec

  def translate_to(self, pos):
    """Translate the reference point to the given position."""
    self.value = Point(pos)

  def get_py_source(self):
    if self.kind != REFPOINT_PARENT:
      return "{} = Point({}, {})".format(self.name, *self.value)

    lhs = rhs = None
    ctr = "Point({}, {})".format(*self.value)
    children = self.related
    if len(children) >= 1:
      arg = children[0]
      lhs = (arg.name if arg is not None else 'None')
      if len(children) >= 2:
        arg = children[1]
        rhs = (arg.name if arg is not None else None)
    args = filter(None, (lhs, ctr, rhs))
    return "%s = Tri(%s)" % (self.name, ", ".join(args))

  # How we draw the refpoints in layers:
  # - LAYER 0:
  #   for every child (direction point): draw the line from the parent.
  # - LAYER 1:
  #   child points, parent points and lonely points are drawn

  def is_outside(self, context, view, region, layers=GContext.layers):
    """Whether the specified layer of the refpoint is completely outside the
    given region (rectangle).
    """

    is_outside_retval = True
    kind = self.kind
    rx, ry, rdx, rdy = region.x, region.y, region.width, region.height

    if 0 in layers and kind == REFPOINT_CHILD:
      # We should determine whether the line connecting parent to child is
      # passing inside the region rectangle. We do this only approximatively.
      parent = self.related
      if parent is not None:
        ax, ay = view.coord_to_pix(parent.value)
        bx, by = view.coord_to_pix(self.value)

        # Find the smallest circle containing the rectangle.
        # Underscores are used to indicate leading 2 factors (we multiply by
        # 2 where necessary to avoid using floating point numbers).
        _cx, _cy = 2*rx + rdx, 2*ry + rdy
        __rr2 = rdx*rdx + rdy*rdy

        # Find vectors ab and ac
        abx, aby, _acx, _acy = bx - ax, by - ay, _cx - 2*ax, _cy - 2*ay

        # Find distance between line l0-l1 and center of circle rc.
        __ac2 = _acx*_acx + _acy*_acy
        ab2 = abx*abx + aby*aby
        if ab2 > 0.0:
          _abac = abx*_acx + aby*_acy
          __d2 = __ac2 - _abac*_abac/ab2
          if __d2 <= __rr2:
            is_outside_retval = False
        else:
          if __ac2 <= __rr2:
            is_outside_retval = False

    if 1 in layers:
      # We should determine whether the two rectangles (the region and the
      # refpoint) intersect. We treat all refpoints as rectangles, even if
      # the child are drawn as circles.
      sz = context.rp_size
      cx, cy = map(int, view.coord_to_pix(self.value))

      if rectangles_overlap(cx - sz, cy - sz, cx + sz, cy + sz,
                            rx, ry, rx + rdx, ry + rdy):
        is_outside_retval = False

    return is_outside_retval

  def get_rectangle(self, context, view, layers=GContext.layers):
    """Get the smallest rectangle fully containing the refpoint."""
    drawable = context.drawable

    visible_coords = drawable.get_visible_coords()
    if visible_coords is None:
      return None

    kind = self.kind
    retval = None
    if 0 in layers and kind == REFPOINT_CHILD:
      parent = self.related
      if parent is not None:
        x0, y0 = map(int, view.coord_to_pix(parent.value))
        x1, y1 = map(int, visible_coords.coord_to_pix(self.value))
        xmin, ymin = min(x0, x1), min(y0, y1)
        dx, dy = max(x0, x1) - xmin + 1, max(y0, y1) - ymin + 1
        retval = Rectangle(xmin, ymin, dx, dy)

    if 1 in layers:
      wsize = drawable.window.get_size()
      l0 = context.rp_size
      dl0 = l0*2
      x, y = visible_coords.coord_to_pix(self.value)
      x0, y0, dx0, dy0 = cut_square(wsize, x - l0, y - l0, dl0, dl0)
      if dx0 > 0 and dy0 > 0:
        retval1 = Rectangle(x0, y0, dx0, dy0)
        retval = retval.union(retval1) if retval is not None else retval1

    return retval

  def draw(self, context, view):
    """Draw the reference point."""

    if not self.visible or self.value is None:
      return

    kind = self.kind
    layer = context.layer
    state = self.get_state()
    drawable = context.drawable.window

    size = context.rp_size
    gc = (context.gc_sel if state == REFPOINT_SELECTED else
          context.gc_unsel if state == REFPOINT_UNSELECTED else
          context.gc_drag)

    x, y = view.coord_to_pix(self.value)
    if layer == 0 and kind == REFPOINT_CHILD:
      parent = self.related
      if parent is not None:
        x0, y0 = view.coord_to_pix(parent.value)
        drawable.draw_line(context.gc_line,
                           int(x0), int(y0), int(x), int(y))
    elif layer == 1:
      if kind == REFPOINT_CHILD:
        draw_circle(drawable, x, y, size, gc)
      else:
        draw_square(drawable, x, y, size, gc)


class RefPoints(object):
  def __init__(self, content=[], callbacks=None):
    self.content = list(content)
    self.by_name = {}
    for rp in content:
      self.by_name[rp.name] = rp
    self.selection = {}
    self.callbacks = cbs = Callbacks.share(callbacks)
    cbs.default("get_next_refpoint_name")
    cbs.default("set_next_refpoint_name")
    cbs.default("refpoint_append")
    cbs.default("refpoint_remove")

  def __iter__(self):
    return self.content.__iter__()

  def __contains__(self, name):
    return name in self.by_name

  def __getitem__(self, key):
    return self.by_name[key]

  def get_dirpoints(self):
    """Get refpoints with directions (parent refpoints)."""
    return [rp for rp in self.content if rp.kind == REFPOINT_PARENT]

  def load(self, refpoints):
    """Replace the current RefPoint-s with the given ones.
    ('refpoints' is a list of RefPoint objects)
    """

    # Notify RefPoint removals
    fn = self.callbacks["refpoint_remove"]
    if fn is not None:
      for rp in self.content:
        fn(self, rp)

    self.content = list(refpoints)
    self.by_name = {}
    for rp in refpoints:
      self.by_name[rp.name] = rp
    self.selection = {}

    # Notify RefPoint additions
    fn = self.callbacks["refpoint_append"]
    if fn is not None:
      for rp in self.content:
        fn(self, rp)

  def remove_all(self):
    """Remove all the RefPoint objects inserted so far."""
    self.load([])

  def clear_selection(self):
    """Clear the current selection of RefPoint-s."""
    for rp in self.selection.itervalues():
      rp.selected = REFPOINT_UNSELECTED
    self.selection = {}

  def select(self, *rps, **kwargs):
    """Add the RefPoint-s in rps to the current selected RefPoint-s."""
    flip = kwargs.pop('flip', False)
    if len(kwargs) > 0:
      raise TypeError("Unexpected keyword argument(s) {}"
                      .format(', '.join(kwargs.keys())))
    selection = self.selection
    if flip:
      for rp in rps:
        if rp.selected:
          rp.selected = REFPOINT_UNSELECTED
          selection.pop(rp.name, None)
        else:
          rp.selected = REFPOINT_SELECTED
          selection[rp.name] = rp
    else:
      for rp in rps:
        rp.selected = REFPOINT_SELECTED
        selection[rp.name] = rp

  def deselect(self, *rps):
    """Remove the RefPoint-s in rps from the current selected RefPoint-s."""
    selection = self.selection
    for rp in rps:
      if rp.name in selection:
        rp.selected = REFPOINT_UNSELECTED
        selection.pop(rp.name)

  def set_selection(self, *selection):
    """Replace the current selection with the given one."""
    self.clear_selection()
    self.select(*selection)

  def is_selected(self, rp):
    """Returns whether the given RefPoint belongs to the current selection.
    """
    assert rp.name in self.by_name
    return (rp.selected != REFPOINT_UNSELECTED)

  def set_visibility(self, selection, visible):
    """Show or hide the refpoints specified by the string ``selection``.
    Jolly characters can be used. For example: gui* means any refpoints
    whose name starts with "gui"
    """
    refpoints_names = self.by_name.keys()
    try:
      selected_refpoints = fnmatch.filter(refpoints_names, selection)
    except:
      return

    rps = map(lambda rp_name: self.by_name[rp_name], selected_refpoints)
    for rp in rps:
      rp.visible = visible
    return rps

  def append(self, rp):
    if rp.name in self.by_name:
      raise ValueError("A reference point with the same name already exists "
                       " (%s)" % rp.name)
    self.by_name[rp.name] = rp
    self.content.append(rp)

    # Append notification.
    self.callbacks.call("refpoint_append", self, rp)

  def remove(self, rp):
    ret = [rp]
    if rp.kind == REFPOINT_PARENT:
      # Do also remove the children
      for child in rp.related:
        if child is not None:
          self.remove(child)
          ret.append(child)
    elif rp.kind == REFPOINT_CHILD:
      # Update parent
      parent = rp.related
      children = parent.related
      children[children.index(rp)] = None
      if len(filter(None, children)) == 0:
        parent.kind = REFPOINT_LONELY
        parent.related = None

    self.content.remove(rp)
    self.by_name.pop(rp.name)
    self.selection.pop(rp.name, None)

    # Remove notification
    self.callbacks.call("refpoint_remove", self, rp)
    return ret

  def get_nearest(self, point, include_invisible=False):
    """Find the refpoint which is nearest to the given point.
    Returns a couple made of the reference point and the distance.
    """
    current_d = None
    current = None
    for p in self.content:
      if p.visible or include_invisible:
        d = square_metric(point, p.value)
        if current_d is None or d <= current_d:
          current = (p, d)
          current_d = d
    return current

  def get_neighbours(self, rp, distance=None):
    """Find the RefPoint-s whose distance from 'rp' is not greater than 'd'.
    """
    if distance is not None:
      neighbours = []
      for rpi in self.content:
        if square_metric(rpi.value, rp.value) <= distance and rpi != rp:
          neighbours.append(rpi)
      return neighbours
    else:
      return [rpi for rpi in self.content if rpi != rp]

  def get_visible_ratio(self, selection):
    """Return a number between 0.0 and 1.0 indicating the ratio
    (num. visible points)/(num. total points)."""
    refpoints_names = self.by_name.keys()
    try:
      selected_refpoints = fnmatch.filter(refpoints_names, selection)
    except:
      return 0.5

    num_total = len(selected_refpoints)
    if num_total < 1:
      return 0.5

    else:
      num_visible = 0
      for rp_name in selected_refpoints:
        rp = self.by_name[rp_name]
        if rp.visible:
          num_visible += 1
      return num_visible/float(num_total)

  def new_name(self, name=None):
    """If name is None or is not given, create automatically a name starting
    from the 'base_name' given during class initialisation.
    Example: if 'base_name="p"', then returns p1, or p2 (if p1 has been already
    used, etc.)"""
    if name is not None:
      return name

    name = self.callbacks.call("get_next_refpoint_name", self) or 'gui'
    next_name = namegen.generate_next_name(name, increment=0)
    while len(next_name.strip()) < 1 or next_name in self.by_name:
      next_name = namegen.generate_next_name(next_name)
    self.callbacks.call("set_next_refpoint_name",
                        self, namegen.generate_next_name(next_name))
    return next_name
