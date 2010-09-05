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

__all__ = ["sgn", "central_segments_intersection", "segment_adjustment",
           "rectangle_adjustment", "Point", "Rectangle"]

def sgn(x):
  return (x >= 0.0) - (x <= 0.0)

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
    if type(r) == float:
      return Point(r*self.x, r*self.y)
    else:
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
      #print obj, ("is inside" if v else "is outside"), self
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
