# Copyright (C) 2010 by Matteo Franchin (fnch@users.sourceforge.net)
#
# This file is part of Pyrtist.
#
#   Pyrtist is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Pyrtist is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

"""
Here we provide a Python interface to call Box and get back a view
(an image) of the picture.
"""

import os
import time

import gtk.gdk
from gtk.gdk import pixbuf_new_from_file

import config
from geom2 import *
from zoomable import View, ImageDrawer, DrawSucceded, DrawFailed, \
                     DrawStillWorking

_box_preamble_centered = '''
def gui(w):
  sf = CairoCmdExecutor.create_image_surface('rgb24', $SX$, $SY$)
  view = w.draw_full_view(sf)
  sf.write_to_png($IMG_FILENAME$)
  with open($INFO_FILENAME$, 'w') as f:
    f.write(repr(view))
'''

_box_preamble_view = '''
def gui(w):
  sf = CairoCmdExecutor.create_image_surface('rgb24', $PX$, $PY$)
  view = View(None, Point($OX$, $OY$), Point($SX$, $SY$))
  view = w.draw_zoomed_view(sf, view)
  sf.write_to_png($IMG_FILENAME$)
  with open($INFO_FILENAME$, 'w') as f:
    f.write(repr(view))
'''

class PyImageDrawer(ImageDrawer):
  def __init__(self, document):
    ImageDrawer.__init__(self)
    self.document = document
    self.out_fn = None
    self.bbox = None
    self.view = View()
    self.executing = False
    self.executed_successfully = False

  def set_output_function(self, fn):
    self.out_fn = fn

  def _raw_execute(self, pix_size, pixbuf_output, preamble=None,
                   img_out_filename=None, extra_substs=[]):
    tmp_fns = []
    info_out_filename = config.tmp_new_filename("info", "dat", tmp_fns)
    if img_out_filename == None:
      img_out_filename = config.tmp_new_filename("img", "png", tmp_fns)

    if preamble == None:
      preamble = ''

    substs = [("$INFO_FILENAME$", repr(info_out_filename)),
              ("$IMG_FILENAME$", repr(img_out_filename))]
    for var, val in substs + extra_substs:
      preamble = preamble.replace(var, str(val))

    def exit_fn():
      with gtk.gdk.lock:
        self.executed_successfully = False
        try:
          if os.path.exists(info_out_filename):
            f = open(info_out_filename, "r")
            ls = f.read().splitlines()
            f.close()
            _, bminx, bminy, bmaxx, bmaxy = \
              [float(x) for x in ls[0].split(",")]
            ox, oy, sx, sy = [float(x) for x in ls[1].split(",")]
            self.bbox = Rectangle(Point(bminx, bmaxy), Point(bmaxx, bminy))
            self.view.reset(pix_size, Point(ox, oy + sy), Point(ox + sx, oy))

          if os.path.exists(img_out_filename):
            pixbuf = pixbuf_new_from_file(img_out_filename)
            sx = pixbuf.get_width()
            sy = pixbuf.get_height()
            pixbuf.copy_area(0, 0, sx, sy, pixbuf_output, 0, 0)
            self.executed_successfully = True

        finally:
          self.executing = False
          config.tmp_remove_files(tmp_fns)
          self.finished_drawing(DrawSucceded(self.bbox, self.view)
                                if self.executed_successfully
                                else DrawFailed())

    def my_out_fn(s):
      with gtk.gdk.lock:
        out_fn = self.out_fn
        if out_fn is not None:
          out_fn(s)

    self.executing = True
    return self.document.execute(preamble=preamble,
                                 out_fn=my_out_fn,
                                 exit_fn=exit_fn)

  def update(self, pixbuf_output, pix_view, coord_view=None,
             img_out_filename=None):
    preamble = ''
    if coord_view == None:
      px, py = pix_view
      substs = [("$SX$", px), ("$SY$", py)]
      preamble += _box_preamble_centered
    else:
      self.view.reset(pix_view, coord_view.corner1, coord_view.corner2)
      px, py = self.view.view_size
      c1x, c1y = self.view.corner1
      c2x, c2y = self.view.corner2
      ox, oy = (min(c1x, c2x), min(c1y, c2y))
      sx, sy = (abs(c2x - c1x), abs(c2y - c1y))
      substs = [("$OX$", ox), ("$OY$", oy),
                ("$SX$", sx), ("$SY$", sy),
                ("$PX$", px), ("$PY$", py)]
      preamble += _box_preamble_view

    killer = self._raw_execute(pix_view, pixbuf_output,
                               preamble=preamble,
                               extra_substs=substs,
                               img_out_filename=img_out_filename)

    # Here we wait for some time to see if we can just draw and proceed.
    # If the time is not enough then we exit and get back the control of the
    # GUI to the user. In this waiting loop it is important to release the
    # lock of the GTK threads, otherwise the other threads will be waiting
    # for this one (which is having a very high time per Python opcode...)
    gtk.gdk.threads_leave()
    for i in range(10):
      if not self.executing:
        break
      time.sleep(0.05)
    gtk.gdk.threads_enter()

    if self.executing:
      return DrawStillWorking(self, killer)

    elif self.executed_successfully:
      return DrawSucceded(self.bbox, self.view)

    else:
      return DrawFailed()

if __name__ == "__main__":
  import sys
  import document
  import zoomable
  d = document.Document()
  d.load_from_file("../examples/poly.box")
  bd = PyImageDrawer(d)
  bd.out_fn = sys.stdout.write
  preamble = _box_preamble_centered
  view = zoomable.View(Point(200, 200), Point(16.0, 47.0), Point(77.0, 4.0))
  bd.update(None, Point(200, 200), view, img_out_filename="poly.png")
