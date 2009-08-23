# Copyright (C) 2008 by Matteo Franchin (fnch@users.sourceforge.net)
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
In this file we define the class ImgView, used to display the resulting image
produced by a Box program and to handle the reference points.
"""

import math

import document, namegen

def debug():
  import sys
  from IPython.Shell import IPShellEmbed
  calling_frame = sys._getframe(1)
  IPShellEmbed([])(local_ns  = calling_frame.f_locals,
                   global_ns = calling_frame.f_globals)

def round_metric(p1, p2):
  """Returns the common euclidean distance between two 2D points p1 and p2."""
  return math.sqrt((p2[0] - p1[0])**2 + (p2[1] - p1[1])**2)

def square_metric(p1, p2):
  """Square metric: the maximum of the absolute values of the coordinates
  of the point p2 - p1.
  When square_metric(p1, p2) <= x, the two 2D points p1, p2 lie inside
  boundaries of a square of side x."""
  return max(abs(p2[0] - p1[0]), abs(p2[1] - p1[1]))

class RefPoint:
  def __init__(self, name):
    self.coords = None
    self.box_coords = None # not supposed to be changed manually...
    self.background = None # ... use the method ImgView.ref_point_coords_set
    self.name = name

  def get_name(self):
    return self.name

class Grid:
  def __init__(self, is_integer=False):
    """Create a new grid. If is_integer == True the align function will return
    a couple of integers, instead of reals."""
    self.offset = (0, 0)
    self.resolution = None
    self.is_integer = is_integer

  def set_offset(self, offset):
    """Set the grid offset."""
    x, y = offset
    self.offset = (x, y)

  def set_resolution(self, resolution):
    """Set the grid offset."""
    self.resolution = resolution

  def align(self, point):
    """Return the point aligned to the grid."""
    x, y = point
    if self.resolution == None:
      return point

    else:
      ox, oy = self.offset
      rx, ry = self.resolution
      x = round(float(x - ox)/rx)*rx + ox
      y = round(float(y - oy)/ry)*ry + oy
      if self.is_integer:
        return (int(x), int(y))
      return (x, y)

class ImgView:
  """Class which allows to display the image produced by Box and the reference
  points.
  """

  def __init__(self, drawable, ref_point_size=3, base_name="gui"):
    self.img = drawable
    self.bbox = None
    self.ref_point = {}
    self.ref_point_list = []
    self.ref_point_size = ref_point_size
    self.base_name = base_name
    self.metric = square_metric
    self.sep_newline = "\n"
    self.sep_comma = ", "
    self.code_max_row = 79
    self.comment_line = lambda line: (("// %s" % line) + self.sep_newline)
    self.grid = Grid()
    self.grid.set_resolution((20, 20))

    def set_next_refpoint(self, value): self.base_name = value

    self.attrs = {"get_next_refpoint_name": lambda self: self.base_name,
                  "set_next_refpoint_name": set_next_refpoint}

  def set_attr(self, attr, value):
    """Set an attribute of the class."""
    if self.attrs.has_key(attr):
      self.attrs[attr] = value

    else:
      raise "The ImgView class has not an attribute with name '%s'" % attr

  def distance(self, p1, p2):
    """Returns the distance between two given points, using the currently
    selected metric.
    """
    return self.metric(p1, p2)

  def auto_name(self, name=None):
    """If name is None or is not given, create automatically a name starting
    from the 'base_name' giving during class initialisation.
    Example: if 'base_name="p"', then returns p1, or p2 (if p1 has been already
    used, etc.)"""
    if name != None: return name
    get_next_refpoint = self.attrs["get_next_refpoint_name"]
    next_name = get_next_refpoint(self)
    while len(next_name.strip()) < 1 or self.ref_point.has_key(next_name):
      next_name = namegen.generate_next_name(next_name)
    set_next_refpoint = self.attrs["set_next_refpoint_name"]
    set_next_refpoint(self, namegen.generate_next_name(next_name))
    return next_name

  def nearest(self, point):
    """Find the ref. point which is nearest to the given point.
    Returns a couple containing made of the reference point and the distance.
    """
    current_d = None
    current = None
    for p in self.ref_point_list:
      d = self.distance(point, p.coords)
      if current_d == None or d <= current_d:
        current = (p, d)
        current_d = d
    return current

  def set_from_file(self, filename, bbox):
    """Load the image from file with associated bounding box 'bbox'."""
    self.ref_point_hide_all()
    self.img.set_from_file(filename)
    self.img.show_now()
    # The py_coords changed when loading the new image file,
    # therefore here we need to convert back from box_coords
    self.bbox = bbox
    for ref_point in self.ref_point_list:
      ref_point.coords = self.map_coords_from_box(ref_point.box_coords)
    self.ref_point_show_all()

  def set_radius(self, radius):
    """Set the radius used to decide whether the method 'pick' should
    pick a point or not."""
    self.ref_point_size = radius

  def pick(self, point):
    """Returns the ref. point closest to the given one (argument 'point').
    If the distance between the two is greater than the current 'radius'
    (see set_radius), return None."""
    current = self.nearest(point)
    if self.ref_point_size == None: return current
    if current == None or current[1] > self.ref_point_size: return None
    return current

  def get_point_list(self):
    """Return a list of GUIPoints corresponding to the current set of reference
    points. The list is compatible to the one which is expected by the method
    add_from_list."""
    refpoints = []
    for p in self.ref_point_list:
      refpoints.append(document.GUIPoint(id=p.name, value=p.box_coords))
    return refpoints

  def add_from_list(self, refpoints):
    """Add reference points from the given list of GUIPoints."""
    for rp in refpoints:
      self.ref_point_box_new(rp.value, rp.id)

  def get_size(self):
    """Return a couple of two reals: the width and height of the image
    view.
    """
    try:
      pixbuf = self.img.get_pixbuf()
      return (pixbuf.get_width(), pixbuf.get_height())
    except:
      return (-1, -1)

  def map_coords_to_box(self, py_coords):
    """Map coordinates of the mouse on the GTK widget to Box coordinates."""

    if self.bbox == None:
      return None

    py_x, py_y = py_coords
    py_sx, py_sy = self.get_size()

    ((box_min_x, box_min_y), (box_max_x, box_max_y)) = self.bbox
    x = box_min_x + (box_max_x - box_min_x)*py_x/py_sx
    y = box_max_y + (box_min_y - box_max_y)*py_y/py_sy
    return (x, y)

  def map_coords_from_box(self, box_coords):
    """Map Box coordinates to on the GTK widget."""

    if self.bbox == None:
      return None

    ((box_min_x, box_min_y), (box_max_x, box_max_y)) = self.bbox
    box_x, box_y = box_coords
    py_sx, py_sy = self.get_size()
    x = py_sx*(box_x - box_min_x)/(box_max_x - box_min_x)
    y = py_sy*(box_max_y - box_y)/(box_max_y - box_min_y)
    return (x, y)

  def ref_point_coords_set(self, ref_point, coords):
    """Set the coordinates for the RefPoint object 'p'."""
    ref_point.coords = coords
    ref_point.box_coords = self.map_coords_to_box(coords)

  def __cut_point(self, x, y):
    mx, my = self.get_size()
    return (max(0, min(mx-1, x)), max(0, min(my-1, y)))

  def __cut_square(self, x, y, dx, dy):
    x, y = (int(x), int(y))
    x0, y0 = self.__cut_point(x, y)
    x1, y1 = self.__cut_point(x+dx, y+dy)
    sx, sy = (x1 - x0, y1 - y0)
    if sx < 1 or sy < 1: return (x0, y0, -1, -1)
    return (x0, y0, sx+1, sy+1)

  def ref_point_draw(self, coords, size, return_background=True):
    x, y = coords
    l0 = size
    dl0 = l0*2
    x0, y0, dx0, dy0 = self.__cut_square(x-l0, y-l0, dl0, dl0)
    if dx0 < 1 or dy0 < 1: return None

    pixbuf = self.img.get_pixbuf()
    subbuf = pixbuf.subpixbuf(x0, y0, dx0, dy0)

    # Before filling, save the background, if necessary
    background = None
    if return_background:
      background = (dx0, dy0, x0, y0, subbuf.copy())

    subbuf.fill(0x000000ff) # Fill it!

    l1 = l0 - 1
    dl1 = 2*l1
    x1, y1, dx1, dy1 = self.__cut_square(x-l1, y-l1, dl1, dl1)
    if dx1 > 0 and dy1 > 0:
      subbuf = pixbuf.subpixbuf(x1, y1, dx1, dy1)
      subbuf.fill(0xffffffff)

    self.img.queue_draw_area(x0, y0, dx0, dy0)
    return background

  def restore_background(self, background):
    if background == None: return
    sx, sy, x, y, pixbuf = background
    dest_pixbuf = self.img.get_pixbuf()
    pixbuf.copy_area(0, 0, sx, sy, dest_pixbuf, x, y)
    self.img.queue_draw_area(x, y, sx, sy)

  def ref_point_box_new(self, box_coords, name=None):
    """Add a new reference point whose coordinates are 'point' (a couple
    of floats) and the name is 'name'. If 'name' is not given, then a name
    is generated automatically using the 'base_name' given during construction
    of the class.
    RETURN the name finally chosen for the point.
    """
    real_name = self.auto_name(name)
    if self.ref_point.has_key(real_name):
      raise "A reference point with the same name exists (%s)" % real_name

    ref_point = RefPoint(real_name)
    ref_point.box_coords = box_coords
    ref_point.coords = None

    self.ref_point[real_name] = ref_point
    self.ref_point_list.append(ref_point)
    return real_name

  def ref_point_new(self, coords, name=None):
    """Add a new reference point whose coordinates are 'point' (a couple
    of floats) and the name is 'name'. If 'name' is not given, then a name
    is generated automatically using the 'base_name' given during construction
    of the class.
    RETURN the name finally chosen for the point.
    """
    real_name = self.auto_name(name)
    if self.ref_point.has_key(real_name):
      raise "A reference point with the same name exists (%s)" % real_name

    ref_point = RefPoint(real_name)
    ref_point.background = self.ref_point_draw(coords, self.ref_point_size)
    self.ref_point_coords_set(ref_point, coords)

    self.ref_point[real_name] = ref_point
    self.ref_point_list.append(ref_point)
    return real_name

  def ref_point_move(self, name, coords):
    ref_point = self.ref_point[name]
    self.restore_background(ref_point.background)
    self.ref_point_coords_set(ref_point, coords)
    ref_point.background = self.ref_point_draw(coords, self.ref_point_size)

  def ref_point_del(self, name):
    ref_point = self.ref_point[name]
    self.restore_background(ref_point.background)
    self.ref_point.pop(name)
    j = None
    for i in range(len(self.ref_point_list)):
      if self.ref_point_list[i].name == name:
        j = i
        break
    if j == None:
      print "ImgView.ref_point_del: shouldn't happen!"
      return
    self.ref_point_list.pop(j)

  def ref_point_del_all(self):
    """Delete all the defined points."""
    self.ref_point_hide_all()
    self.ref_point = {}
    self.ref_point_list = []

  def ref_point_hide_all(self):
    for ref_point in self.ref_point_list:
      self.restore_background(ref_point.background)
      ref_point.background = None

  def ref_point_show_all(self):
    for _, ref_point in self.ref_point.items():
      self.restore_background(ref_point.background)
      ref_point.background = self.ref_point_draw(ref_point.coords,
                                                 self.ref_point_size)
