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


class RefPoint(object):
  def __init__(self, name, value=None, visible=False):
    self.name = name
    self.value = value
    self.visible = visible


class RefPoints(object):
  def __init__(self, content=[]):
    self.content = list(content)

  def __iter__(self):
    return self.content.__iter__()

  def append(self, obj):
    self.content.append(obj)

  def get_nearest(self, point, include_invisible=False):
    """Find the refpoint which is nearest to the given point.
    Returns a couple made of the reference point and the distance.
    """
    current_d = None
    current = None
    for p in self.ref_point_list:
      if p.visible or include_invisible:
        d = square_metric(point, p.coords)
        if current_d == None or d <= current_d:
          current = (p, d)
          current_d = d
    return current
