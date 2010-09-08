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

from geom2 import square_metric, Point
import namegen


class RefPoint(object):
  def __init__(self, name, value=None, visible=True):
    self.name = name
    self.value = value
    self.visible = visible


class RefPoints(object):
  def __init__(self, content=[]):
    self.content = list(content)
    self.by_name = {}
    for rp in content:
      self.by_name[rp.name] = rp
    self._fns = {}

  def __iter__(self):
    return self.content.__iter__()

  def __contains__(self, name):
    return name in self.by_name

  def append(self, rp):
    if rp.name in self.by_name:
      raise ValueError("A reference point with the same name already exists "
                       " (%s)" % rp.name)
    self.by_name[rp.name] = rp
    self.content.append(rp)

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

  def __getitem__(self, key):
    return self.by_name[key]

  def new_name(self, name=None):
    """If name is None or is not given, create automatically a name starting
    from the 'base_name' given during class initialisation.
    Example: if 'base_name="p"', then returns p1, or p2 (if p1 has been already
    used, etc.)"""
    if name != None:
      return name

    get_next_refpoint_name = self._fns.get("get_next_refpoint_name", None)
    name = get_next_refpoint_name() if get_next_refpoint_name else "gui"
    next_name = namegen.generate_next_name(name, increment=0)
    while len(next_name.strip()) < 1 or next_name in self.by_name:
      next_name = namegen.generate_next_name(next_name)
    set_next_refpoint = self._fns.get("set_next_refpoint_name", None)
    if set_next_refpoint:
      set_next_refpoint(self, namegen.generate_next_name(next_name))
    return next_name
