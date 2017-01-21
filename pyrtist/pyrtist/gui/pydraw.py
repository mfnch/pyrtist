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

class PyImageDrawer(ImageDrawer):
  def __init__(self, document):
    super(PyImageDrawer, self).__init__()
    self.document = document
    self.out_fn = None
    self.bbox = None
    self.view = View()
    self.executing = False
    self.executed_successfully = False

  def set_output_function(self, fn):
    self.out_fn = fn

  def _raw_execute(self, pix_size, pixbuf_output,
                   startup_cmds=None, img_out_filename=None):
    tmp_fns = []
    info_out_filename = config.tmp_new_filename("info", "dat", tmp_fns)
    if img_out_filename is None:
      img_out_filename = config.tmp_new_filename("img", "png", tmp_fns)

    startup_cmds.append(('set_tmp_filenames', info_out_filename,
                         img_out_filename))

    def exit_fn():
      self.executed_successfully = False
      try:
        if os.path.exists(info_out_filename):
          with open(info_out_filename, "r") as f:
            ls = f.read().splitlines()
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

    def callback(name, *args):
      with gtk.gdk.lock:
        if name == 'exit':
          exit_fn()
        elif name == 'out':
          if self.out_fn is not None:
            self.out_fn(args[1])

    self.executing = True
    return self.document.execute(callback=callback, startup_cmds=startup_cmds)

  def update(self, pixbuf_output, pix_view, coord_view=None,
             img_out_filename=None):
    startup_cmds = []
    if coord_view is None:
      px, py = pix_view
      startup_cmds.append(('full_view', px, py))
    else:
      self.view.reset(pix_view, coord_view.corner1, coord_view.corner2)
      px, py = self.view.view_size
      c1x, c1y = self.view.corner1
      c2x, c2y = self.view.corner2
      ox, oy = (min(c1x, c2x), min(c1y, c2y))
      sx, sy = (abs(c2x - c1x), abs(c2y - c1y))
      startup_cmds.append(('zoomed_view', px, py, ox, oy, sx, sy))

    killer = self._raw_execute(pix_view, pixbuf_output,
                               startup_cmds=startup_cmds,
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
