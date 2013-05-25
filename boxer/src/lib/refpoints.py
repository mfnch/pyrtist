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

import fnmatch

from geom2 import square_metric, Point
from renderer import draw_square, draw_circle
import namegen


class GContext(object):
  def __init__(self, drawable, gc_unsel, gc_sel, gc_drag, gc_line):
    self.drawable = drawable
    self.gc_unsel = gc_unsel
    self.gc_sel = gc_sel
    self.gc_drag = gc_drag
    self.gc_line = gc_line


(REFPOINT_UNSELECTED,
 REFPOINT_SELECTED,
 REFPOINT_DRAGGED) = range(3)

(REFPOINT_LONELY,
 REFPOINT_PARENT,
 REFPOINT_CHILD) = range(3)


class RefPoint(object):
  def __init__(self, name, value=None, visible=True,
               kind=REFPOINT_LONELY, selected=REFPOINT_UNSELECTED):
    global removeme_last
    self.kind = kind
    self.related = None
    self.name = name
    self.value = value
    self.visible = visible
    self.selected = selected

  def copy(self, state=None):
    state = (state if state != None else self.selected)
    return RefPoint(self.name, self.value, self.visible, self.kind, state)

  def can_procreate(self):
    """Whether this reference point can have any children."""
    return (self.kind != REFPOINT_CHILD
            and (self.related == None
                 or len(self.related) < 2
                 or None in self.related))

  def get_state(self):
    return self.selected

  def get_children(self):
    """Get the children point of a direction point."""
    assert self.kind == REFPOINT_PARENT
    return self.related

  def make_parent_of(self, *rps):
    """Mark this reference point as a parent of the given reference points."""
    self.related = children = self.related or []
    for rp in rps:
      rp.kind = REFPOINT_CHILD
      rp.related = self
      self.kind = REFPOINT_PARENT
      if None in children:
        children[children.index(None)] =rp
      else:
        assert len(children) < 2
        children.append(rp)

  def translate(self, vec):
    """Translate the reference point by the given amount."""
    self.value = Point(self.value) + vec

  def translate_to(self, pos):
    """Translate the reference point to the given position."""
    self.value = Point(pos)

  def get_box_source(self):
    kind = self.kind
    if kind == REFPOINT_PARENT:
      children = self.related
      n = len(children)
      if children != None and n > 0:
        pin = children[0]
        pout = children[1] if n > 1 else None
        args = []
        for i, arg in enumerate((pin, self, pout)):
          if arg != None:
            args.append("Point[.x=%s, .y=%s]" % tuple(arg.value))
          elif i == 0:
            args.append(";")

        return "%s = Tri[%s]" % (self.name, ", ".join(args))

    v = self.value
    return ("%s = Point[.x=%s, .y=%s]" % (self.name, v[0], v[1]))

  def draw(self, context, view, size):
    """Draw the reference point."""
    if self.visible:
      if self.value != None:
        state = self.get_state()
        drawable = context.drawable
        gc = (context.gc_sel if state == REFPOINT_SELECTED else
              context.gc_unsel if state == REFPOINT_UNSELECTED else
              context.gc_drag)

        x, y = view.coord_to_pix(self.value)
        kind = self.kind
        if kind == REFPOINT_LONELY:
          draw_square(drawable, x, y, size, gc)
        elif kind == REFPOINT_CHILD:
          parent = self.related
          if parent != None:
            x0, y0 = view.coord_to_pix(parent.value)
            drawable.draw_line(context.gc_line,
                               int(x0), int(y0), int(x), int(y))
          draw_circle(drawable, x, y, size, gc)
        elif kind == REFPOINT_PARENT:
          draw_square(drawable, x, y, size, gc)


class RefPoints(object):
  def __init__(self, content=[], callbacks=None):
    self.content = list(content)
    self.by_name = {}
    for rp in content:
      self.by_name[rp.name] = rp
    self.selection = {}
    self._fns = cbs = callbacks if callbacks != None else {}
    cbs.setdefault("get_next_refpoint_name", None)
    cbs.setdefault("set_next_refpoint_name", None)
    cbs.setdefault("refpoint_append", None)
    cbs.setdefault("refpoint_remove", None)

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
    fn = self._fns["refpoint_remove"]
    if fn != None:
      for rp in self.content:
        fn(self, rp)

    self.content = list(refpoints)
    self.by_name = {}
    for rp in refpoints:
      self.by_name[rp.name] = rp
    self.selection = {}

    # Notify RefPoint additions
    fn = self._fns["refpoint_append"]
    if fn != None:
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

  def select(self, *rps, **named_args):
    """Add the RefPoint-s in rps to the current selected RefPoint-s."""
    selection = self.selection
    if named_args.get("flip", False):
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

    # Append notification
    fn = self._fns["refpoint_append"]
    if fn != None:
      fn(self, rp)

  def remove(self, rp):
    ret = [rp]
    if rp.kind == REFPOINT_PARENT:
      # Do also remove the children
      for child in rp.related:
        if child != None:
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
    fn = self._fns["refpoint_remove"]
    if fn != None:
      fn(self, rp)

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
        if current_d == None or d <= current_d:
          current = (p, d)
          current_d = d
    return current

  def get_neighbours(self, rp, distance=None):
    """Find the RefPoint-s whose distance from 'rp' is not greater than 'd'.
    """
    if distance != None:
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
    if name != None:
      return name

    fn = self._fns["get_next_refpoint_name"]
    name = fn(self) if fn != None else "gui"
    next_name = namegen.generate_next_name(name, increment=0)
    while len(next_name.strip()) < 1 or next_name in self.by_name:
      next_name = namegen.generate_next_name(next_name)
    fn = self._fns["set_next_refpoint_name"]
    if fn != None:
      fn(self, namegen.generate_next_name(next_name))
    return next_name
