# Copyright (C) 2010
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

# Basic 2D geometry functionality

__all__ = ["sgn", "central_segments_intersection", "Point", "Rectangle"]

def sgn(x):
  return (x >= 0.0) - (x <= 0.0)

def central_segments_intersection(s1_width, s2_width):
  diff = (s2_width - s1_width)/2
  return (0, diff, s1_width) if diff >= 0 else (-diff, 0, s2_width)

class Point(object):
  def __init__(self, x=None, y=None):
    if y == None:
      self.x, self.y = x if x != None else (0.0, 0.0)
    else:
      self.x = x
      self.y = y

  def __getitem__(self, idx):
    if idx == 0:
      return self.x
    elif idx == 1:
      return self.y
    else:
      raise IndexError("Point index out of range")

  def __iter__(self):
    return self.get_xy().__iter__()

  def __repr__(self):
    return "Point(%s, %s)" % self.xy

  def __add__(self, p):
    return Point(self.x + p.x, self.y + p.y)

  def __sub__(self, p):
    return Point(self.x - p.x, self.y - p.y)

  def get_xy(self):
    return (self.x, self.y)

  def set_xy(self, xy):
    try:
      self.x, self.y = xy   # is iterable
    except:
      self.x = self.y = xy  # is not iterable
    return self

  xy = property(get_xy, set_xy)

  def trunc(self, xmin, xmax, ymin, ymax):
    self.x = min(xmax, max(xmin, self.x))
    self.y = min(ymax, max(ymin, self.y))

  def scale(self, scale_factor):
    sf = Point()
    sf.set_xy(scale_factor)
    self.x *= sf.x
    self.y *= sf.y

class Rectangle(object):
  def __init__(self, corner1, corner2):
    self.corner1 = corner1
    self.corner2 = corner2

  def __repr__(self):
    return "Rectangle(%s, %s)" % (self.corner1, self.corner2)

  def get_dx(self):
    return self.corner2.x - self.corner1.x

  def get_dy(self):
    return self.corner2.y - self.corner1.y

  def translate(self, dr):
    self.corner1 += dr
    self.corner2 += dr

  def new_scaled(self, scale_factor, scale_in=True):
    sx, sy = Point(scale_factor)
    if not scale_in:
      sx, sy = (1/sx, 1/sy)
    hc1x, hc1y = (0.5*self.corner1.x, 0.5*self.corner1.y)
    hc2x, hc2y = (0.5*self.corner2.x, 0.5*self.corner2.y)
    cx, cy = (hc1x + hc2x, hc1y + hc2y)
    wx, wy = ((hc2x - hc1x)*sx, (hc2y - hc1y)*sy)
    c1 = Point(cx - wx, cy - wy)
    c2 = Point(cx + wx, cy + wy)
    return Rectangle(c1, c2)

  def new_reshaped(self, new_shape, scale_factor=None):
    sf = Point(new_shape.x/abs(self.get_dx()),
               new_shape.y/abs(self.get_dy()))
    if scale_factor != None:
      sf.scale(scale_factor)
    return self.new_scaled(sf)
