# Copyright (C) 2010-2017 by Matteo Franchin (fnch@users.sf.net)
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

"""
Basic 2D geometry functionality
"""

__all__ = ["sgn", "central_segments_intersection", "segment_adjustment",
           "rectangle_adjustment", "round_metric", "square_metric",
           "Point", "Tri", "Rectangle"]

def sgn(x):
  return (x >= 0.0) - (x <= 0.0)

def isclose(a, b, rel_tol=1e-10):
  return abs(a - b) <= rel_tol * max(abs(a), abs(b))

def central_segments_intersection(s1_width, s2_width):
  diff = (s2_width - s1_width)/2
  return (0, diff, s1_width) if diff >= 0 else (-diff, 0, s2_width)

def segment_adjustment(bbox1, bbox2, view1, view2):
  """Given two segments s1=(bbox1, bbox2), s2=(view1, view2) returns the
  translation which should be applied to s2 such that:
    - if s2 is wider than s1, then s1 is centered in s2;
    - if s2 goes out of the borders of s1 then it is moved back to the
      boundary
  """
  bbox_max, bbox_min = (max(bbox1, bbox2), min(bbox1, bbox2))
  view_max, view_min = (max(view1, view2), min(view1, view2))
  bbox_dx = bbox_max - bbox_min
  view_dx = view_max - view_min
  if view_dx >= bbox_dx:
    return 0.5*((bbox1 + bbox2) - (view1 + view2))
  elif view_min < bbox_min:
    return bbox_min - view_min
  elif view_max > bbox_max:
    return bbox_max - view_max
  else:
    return 0.0

def rectangle_adjustment(r1, r2):
  """Similar to 'segment_adjustment', but works on Rectangle objects."""
  dx = segment_adjustment(r1.corner1.x, r1.corner2.x,
                          r2.corner1.x, r2.corner2.x)
  dy = segment_adjustment(r1.corner1.y, r1.corner2.y,
                          r2.corner1.y, r2.corner2.y)
  return Point(dx, dy)

def round_metric(p1, p2):
  """Returns the common euclidean distance between two 2D points p1 and p2."""
  return math.sqrt((p2[0] - p1[0])**2 + (p2[1] - p1[1])**2)

def square_metric(p1, p2):
  """Square metric: the maximum of the absolute values of the coordinates
  of the point p2 - p1.
  When square_metric(p1, p2) <= x, the two 2D points p1, p2 lie inside
  boundaries of a square of side x."""
  return max(abs(p2[0] - p1[0]), abs(p2[1] - p1[1]))

def determine_zone(x0, x1, x):
  """Return 0, 1, 2 depending on whether x is inside
  ]-infinity, x0], [x0, x1] or [x1, +infinity[."""
  if x < x0:
    return 0
  elif x <= x1:
    return 1
  else:
    return 2

def rectangles_overlap(a0x, a0y, a1x, a1y, b0x, b0y, b1x, b1y):
  """Whether two triangles with vertices (a0x, aoy)-(a1x, a1y) and
  (b0x, boy)-(b1x, b1y) are intersecting each other."""

  a0x, a1x = min(a0x, a1x), max(a0x, a1x)
  a0y, a1y = min(a0y, a1y), max(a0y, a1y)
  bz0x = determine_zone(a0x, a1x, b0x)
  bz0y = determine_zone(a0y, a1y, b0y)
  bz1x = determine_zone(a0x, a1x, b1x)
  bz1y = determine_zone(a0y, a1y, b1y)

  bz0x, bz1x = min(bz0x, bz1x), max(bz0x, bz1x)
  bz0y, bz1y = min(bz0y, bz1y), max(bz0y, bz1y)

  eqx = (bz0x == bz1x)
  eqy = (bz0y == bz1y)
  if eqx or eqy:
    if eqx and eqy:
      return bz0x == 1 and bz0y == 1
    elif eqx:
      return bz0x == 1
    else: # eqy
      return bz0y == 1
  return True


class Tri(object):
  def __init__(self, *args):
    self.args = args

  def __iter__(self):
    return iter(self.args)

class Point(object):
  def __init__(self, x=None, y=None):
    if y == None:
      if x == None:
        self.x, self.y = (0.0, 0.0)
      else:
        try:
          self.x, self.y = x
        except:
          self.x = self.y = x
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

  def __mul__(self, r):
    try:
      return Point(r*self.x, r*self.y)

    except:
      raise ValueError("Multiplication between Point and '%s' is not "
                       "supported." % type(r))

  def __rmul__(self, r):
    return self.__mul__(r)

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

  def get_center(self):
    """Return the coordinate of the center of the Rectangle."""
    return 0.5*(self.corner1 + self.corner2)

  def set_center(self, center_position):
    """Translate the Rectangle such that its center position becomes the given
    one.
    """
    t = center_position - self.get_center()
    self.translate(t)

  center = property(get_center, set_center)

  def set_center_x(self, center_pos_x):
    """Similar to 'set_center' but works only on the x coordinte."""
    tx = center_pos_x - self.get_center().x
    self.translate(Point(tx, 0.0))

  def set_center_y(self, center_pos_y):
    """Similar to 'set_center' but works only on the y coordinte."""
    ty = center_pos_y - self.get_center().y
    self.translate(Point(0.0, ty))

  def contains(self, obj):
    """Whether the geometric entity 'obj' (either a Point or a Rectangle)
    is inside the rectangle."""
    if isinstance(obj, Point):
      c1x, c1y = self.corner1
      c2x, c2y = self.corner2
      v = ((c1x < obj.x < c2x if c1x <= c2x else c2x < obj.x < c1x) and
           (c1y < obj.y < c2y if c1y <= c2y else c2y < obj.y < c1y))
      return v

    elif isinstance(obj, Rectangle):
      return self.contains(obj.corner1) and self.contains(obj.corner2)

    else:
      raise ValueError("Don't know how to determine whether an '%s' object "
                       "is inside the rectangle." % type(obj))

  def get_dx(self):
    """Get the width of the rectangle."""
    return self.corner2.x - self.corner1.x

  def get_dy(self):
    """Get the height of the rectangle."""
    return self.corner2.y - self.corner1.y

  def translate(self, dr):
    """Translate the rectangle by the given quantity dr (a Point object)."""
    self.corner1 += dr
    self.corner2 += dr

  def copy(self):
    """Return a copy of the Rectangle."""
    return Rectangle(Point(self.corner1), Point(self.corner2))

  def scale(self, scale_factor, scale_in=True):
    """Scale the rectangle while keeping its center in the same position.
    'scale_factor' specify the scale factor (either a scalar or a Point
    with scale factors in each direction), while 'scale_in' specifies whether
    the scale factor should be used to zoom in (True) or zoom out (False).
    The last case is equivalent to use the reciprocal scale factor, with
    'scale_in==True'.
    """
    sx, sy = Point(scale_factor)
    if not scale_in:
      sx, sy = (1/sx, 1/sy)
    hc1x, hc1y = (0.5*self.corner1.x, 0.5*self.corner1.y)
    hc2x, hc2y = (0.5*self.corner2.x, 0.5*self.corner2.y)
    cx, cy = (hc1x + hc2x, hc1y + hc2y)
    wx, wy = ((hc2x - hc1x)*sx, (hc2y - hc1y)*sy)
    self.corner1 = Point(cx - wx, cy - wy)
    self.corner2 = Point(cx + wx, cy + wy)

  def new_scaled(self, scale_factor, scale_in=True):
    p = self.copy()
    p.scale(scale_factor, scale_in=scale_in)
    return p

  def new_reshaped(self, new_shape, scale_factor=None):
    sf = Point(new_shape.x/abs(self.get_dx()),
               new_shape.y/abs(self.get_dy()))
    if scale_factor != None:
      sf.scale(scale_factor)
    return self.new_scaled(sf)

