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
import namegen


class RefPoint(object):
  def __init__(self, name, value=None, visible=True):
    self.name = name
    self.value = value
    self.visible = visible
    self.selected = False


class RefPoints(object):
  def __init__(self, content=[], callbacks=None):
    self.content = list(content)
    self.by_name = {}
    for rp in content:
      self.by_name[rp.name] = rp
    self.selection = []
    self._fns = callbacks if callbacks != None else {}
    callbacks.setdefault("get_next_refpoint_name", None)
    callbacks.setdefault("set_next_refpoint_name", None)
    callbacks.setdefault("refpoint_append", None)
    callbacks.setdefault("refpoint_remove", None)

  def __iter__(self):
    return self.content.__iter__()

  def __contains__(self, name):
    return name in self.by_name

  def __getitem__(self, key):
    return self.by_name[key]

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
    self.selection = []

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
    for rp in self.selection:
      rp.selected = False
    self.selection = []

  def select(self, rps):
    """Add the RefPoint-s in rps to the current selected RefPoint-s."""
    for rp in rps:
      rp.selected = True
    self.selection.extend(rps)

  def set_selection(self, selection):
    """Replace the current selection with the given one."""
    self.clear_selection()
    self.select(selection)

  def is_selected(self, rp):
    """Returns whether the given RefPoint belongs to the current selection.
    """
    assert rp.name in self.by_name
    return rp.selected

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
    self.content.remove(rp)
    self.by_name.pop(rp.name)
    if rp in self.selection:
      self.selection.remove(rp)

    # Remove notification
    fn = self._fns["refpoint_remove"]
    if fn != None:
      fn(self, rp)

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