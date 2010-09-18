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

"""
Here we provide a Python interface to call Box and get back a view
(an image) of the picture.
"""

import os
import time

from gtk.gdk import pixbuf_new_from_file

import config
from geom2 import *
from zoomable import View, \
                     ImageDrawer # This file provides an implementation
                                 # of the ImageDrawer interface
_box_preamble_common = """
include "g"
GUI = Void
"""

_box_preamble_void = "Window@GUI[]\n"

_box_preamble_centered = """
Window@GUI[
  b = BBox[$]
  view = Point[.x=$SX$, .y=$SY$], size = b.max - b.min
  r = Min[view.x/size.x, view.y/size.y]
  new_size = Point[.x=view.x/r, .y=view.y/r]
  tr = 0.5*(new_size - size), origin = b.min - tr
  \ Window[.Origin[origin], new_size, .Res[r]
           "rgb24"][$, .Save["$IMG_FILENAME$"]]
  out = Str[sep=",", b.n, sep
            b.min.x, sep, b.min.y, sep, b.max.x, sep, b.max.y;
            origin.x, sep, origin.y, sep, new_size.x, sep, new_size.y;]
  \ File["$INFO_FILENAME$", "w"][out]
]
"""

_box_preamble_view = """
Window@GUI[
  b = BBox[$]
  size = Point[.x=$SX$, .y=$SY$], origin = Point[.x=$OX$, .y=$OY$]
  \ Window[size, .Res[($RX$, $RY$)], .Origin[origin]
           "rgb24"][$, .Save["$IMG_FILENAME$"]]
  out = Str[sep=",", b.n, sep
            b.min.x, sep, b.min.y, sep, b.max.x, sep, b.max.y;
            origin.x, sep, origin.y, sep, size.x, sep, size.y;]
  \ File["$INFO_FILENAME$", "w"][out]
]
"""

def escape_string(s):
  """Many languages (including Box and Python) take a string representation,
  where escape sequences are accepted, and transform it into a final string.
  Here we do the opposite. For example, if a string is made of one character
  with ASCII value 0, this string maps it to a string made of two characters:
  '\\' and '\0': "\0" --> "\\\0"
  """
  return s.replace("\\", "\\\\") # At the moment we just do this

class BoxImageDrawer(ImageDrawer):
  def __init__(self, document):
    self.document = document
    self.out_fn = None
    self.bbox = None
    self.view = View()
    self.pixbuf = None
    self.executing = False

  def _raw_execute(self, pix_size, preamble=None, img_out_filename=None,
                   extra_substs=[]):
    tmp_fns = []
    info_out_filename = config.tmp_new_filename("info", "dat", tmp_fns)
    if img_out_filename == None:
      img_out_filename = config.tmp_new_filename("img", "png", tmp_fns)

    if preamble == None:
      preamble = _box_preamble_common + _box_preamble_void

    substs = [("$INFO_FILENAME$", escape_string(info_out_filename)),
              ("$IMG_FILENAME$", escape_string(img_out_filename))]
    for var, val in substs + extra_substs:
      preamble = preamble.replace(var, str(val))

    def exit_fn():
      try:
        if os.path.exists(info_out_filename):
          f = open(info_out_filename, "r")
          ls = f.read().splitlines()
          f.close()
          _, bminx, bminy, bmaxx, bmaxy = [float(x) for x in ls[0].split(",")]
          ox, oy, sx, sy = [float(x) for x in ls[1].split(",")]
          self.bbox = Rectangle(Point(bminx, bmaxy), Point(bmaxx, bminy))
          self.view.reset(pix_size, Point(ox, oy + sy), Point(ox + sx, oy))

        if os.path.exists(img_out_filename):
          self.pixbuf = pixbuf_new_from_file(img_out_filename)

      finally:
        self.executing = False
        config.tmp_remove_files(tmp_fns)

    self.executing = True
    return self.document.execute(preamble=preamble,
                                 out_fn=self.out_fn,
                                 exit_fn=exit_fn)

  def update(self, pixbuf_output, pix_view, coord_view=None,
             img_out_filename=None):
    if coord_view == None:
      px, py = pix_view
      substs = [("$SX$", px), ("$SY$", py)]
      preamble = _box_preamble_common + _box_preamble_centered

    else:
      self.view.reset(pix_view, coord_view.corner1, coord_view.corner2)
      px, py = self.view.view_size
      c1x, c1y = self.view.corner1
      c2x, c2y = self.view.corner2
      ox, oy = (min(c1x, c2x), min(c1y, c2y))
      sx, sy = (abs(c2x - c1x), abs(c2y - c1y))
      rx, ry = (px/sx, py/sy)
      substs = [("$OX$", ox), ("$OY$", oy),
                ("$RX$", rx), ("$RY$", ry),
                ("$SX$", sx), ("$SY$", sy)]
      preamble = _box_preamble_common + _box_preamble_view

    self._raw_execute(pix_view, preamble=preamble, extra_substs=substs,
                      img_out_filename=img_out_filename)

    while self.executing: # Temporary solution
      time.sleep(0.05)

    sx = self.pixbuf.get_width()
    sy = self.pixbuf.get_height()
    self.pixbuf.copy_area(0, 0, sx, sy, pixbuf_output, 0, 0)

    return (self.bbox, self.view)

if __name__ == "__main__":
  import sys
  import document
  import zoomable
  d = document.Document()
  d.load_from_file("poly.box")
  bd = BoxImageDrawer(d)
  bd.out_fn = sys.stdout.write
  preamble = _box_preamble_common + _box_preamble_centered
  view = zoomable.View(Point(200, 200), Point(16.0, 47.0), Point(77.0, 4.0))
  bd.update(None, Point(200, 200), view, img_out_filename="poly.png")
