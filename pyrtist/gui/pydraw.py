# Copyright (C) 2010, 2017 by Matteo Franchin (fnch@users.sf.net)
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
Here we provide a Python interface to call Python and get back a view
(an image) of the picture.
"""

import os
import time

import gtk.gdk
from gtk.gdk import pixbuf_new_from_file

from . import config
from .callbacks import Callbacks
from .geom2 import *
from .zoomable import (View, ImageDrawer, DrawSucceded, DrawFailed,
                       DrawStillWorking)


class PyImageDrawer(ImageDrawer):
  def __init__(self, document, callbacks=None):
    super(PyImageDrawer, self).__init__()
    self.document = document
    self.callbacks = cbs = Callbacks.share(callbacks)
    cbs.default('script_write_out')

    self.bbox = None
    self.view = View()
    self._executing = False
    self._executed_successfully = False
    self._startup_cmds = None
    self._pix_size = None
    self._pixbuf_output = None
    self._image_info = None
    self._image_data = None

  def _rx_cmd_out(self, stream_name, out):
    self.callbacks.call('script_write_out', out)

  def _rx_cmd_image_info(self, view_repr):
    self._image_info = [float(x)
                        for line in view_repr.splitlines()
                        for x in line.split(',')]

  def _rx_cmd_image_data(self, args):
    self._image_data = args

  def _rx_cmd_exit(self):
      self._executed_successfully = False
      try:
        if self._image_info is not None:
          (bminx, bminy, bmaxx, bmaxy, ox, oy, sx, sy) = self._image_info
          self.bbox = Rectangle(Point(bminx, bmaxy), Point(bmaxx, bminy))
          self.view.reset(self._pix_size, Point(ox, oy + sy), Point(ox + sx, oy))

        if self._image_data is not None:
          stride, width, height, data = self._image_data
          self._image_data = None
          pixbuf = gtk.gdk.pixbuf_new_from_data(data, gtk.gdk.COLORSPACE_RGB,
                                                False, 8, width, height, stride)
          sx = pixbuf.get_width()
          sy = pixbuf.get_height()
          pixbuf.copy_area(0, 0, sx, sy, self._pixbuf_output, 0, 0)
          self._executed_successfully = True

      finally:
        self._executing = False
        self.finished_drawing(DrawSucceded(self.bbox, self.view)
                              if self._executed_successfully
                              else DrawFailed())

  def _callback(self, name, *args):
    method = getattr(self, '_rx_cmd_' + name, None)
    if method is None:
      return
    if name in ('exit', 'out'):
      with gtk.gdk.lock:
        method(*args)
    else:
      method(*args)

  def _raw_execute(self, pix_size, pixbuf_output, startup_cmds=None):
    self._pix_size = pix_size
    self._pixbuf_output = pixbuf_output
    self._image_info = None
    self._image_data = None
    self._executing = True
    return self.document.execute(callback=self._callback,
                                 startup_cmds=startup_cmds)

  def give_old_refpoints(self, old_refpoints):
    '''Provide old values of the refpoints (e.g. before dragging them). This
    information will be passed to the script the next time it is executed.
    '''
    self._startup_cmds = \
      self.document.build_refpoint_set_cmds('old', old_refpoints)

  def pop_startup_cmds(self):
    '''Obtain the startup commands that the script will use to set up the
    environment.
    '''
    cmds = self._startup_cmds
    self._startup_cmds = None
    return cmds or []

  def update(self, pixbuf_output, pix_view, coord_view=None):
    startup_cmds = self.pop_startup_cmds()
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
                               startup_cmds=startup_cmds)

    # Here we wait for some time to see if we can just draw and proceed.
    # If the time is not enough then we exit and get back the control of the
    # GUI to the user. In this waiting loop it is important to release the
    # lock of the GTK threads, otherwise the other threads will be waiting
    # for this one (which is having a very high time per Python opcode...)
    try:
      gtk.gdk.threads_leave()

      for i in range(10):
        if not self._executing:
          break
        time.sleep(0.05)

    finally:
      gtk.gdk.threads_enter()

    if self._executing:
      return DrawStillWorking(self, killer)
    elif self._executed_successfully:
      return DrawSucceded(self.bbox, self.view)
    else:
      return DrawFailed()
