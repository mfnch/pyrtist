# Copyright (C) 2017 by Matteo Franchin (fnch@users.sf.net)
#
# This file is part of Pyrtist.
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

'''Functionality to communicate with the Pyrtist GUI from scripts.'''

__all__ = ('gui',)

from .cairo_cmd_exec import CairoCmdExecutor
from .core_types import Point, Tri, View

import numpy


class GUIPointAttrs(object):
    '''Attributes for points created and managed by the Pyrtist GUI.'''

    def __init__(self, name, old_value=None):
        self.name = name
        self.old_value = old_value


class GUIGate(object):
    '''Object which allows communicating with the Pyrtist GUI.'''

    def __init__(self):
        self._gui_tx_pipe = None
        self._full_view = None
        self._size = None
        self._zoom_window = None
        self._img_filename = None
        self._points = {'old': {}, 'new': {}}

    def connect(self, gui_tx_pipe=None, startup_cmds=None):
        '''Method invoked by the Pyrtist GUI to allow communicating back
        with it.
        '''
        assert self._gui_tx_pipe is None, 'GUIGate object already connected'
        self._gui_tx_pipe = gui_tx_pipe
        self._parse_cmds(startup_cmds)

    def _parse_cmds(self, cmds):
        if cmds is None:
            return
        for cmd in cmds:
            if len(cmd) < 1:
                continue
            cmd_name = '_rx_cmd_{}'.format(cmd[0])
            method = getattr(self, cmd_name, None)
            if method is not None:
                method(*cmd[1:])

    def _rx_cmd_point(self, tag, name, x, y):
        ps = self._points[tag]
        ps[name] = Point(x, y)

    def _rx_cmd_tri(self, tag, name, x, y, lhs_name, rhs_name):
        ps = self._points[tag]
        ps[name] = Tri(ps.get(lhs_name), Point(x, y), ps.get(rhs_name))

    def _rx_cmd_full_view(self, size_x, size_y):
        self._full_view = True
        self._size = (size_x, size_y)

    def _rx_cmd_zoomed_view(self, size_x, size_y, *view_args):
        self._full_view = False
        self._size = (size_x, size_y)
        self._zoom_window = view_args

    def _tx_cmd_image_info(self, view):
        self._gui_tx_pipe.send(('image_info', repr(view)))

    def _tx_cmd_image_data(self, sf):
        data = numpy.frombuffer(sf.get_data(), dtype=numpy.int8).reshape(-1, 4)
        data = str(numpy.getbuffer(numpy.fliplr(data[:, :3]).ravel()))
        args = (sf.get_stride() / 4 * 3, sf.get_width(), sf.get_height(), data)
        self._gui_tx_pipe.send(('image_data', args))

    def __call__(self, w):
        if self._size is None or self._full_view is None:
            return

        sf = CairoCmdExecutor.create_image_surface('rgb24', *self._size)
        if self._full_view:
            view = w.draw_full_view(sf)
        else:
            ox, oy, sx, sy = self._zoom_window
            view = View(None, Point(ox, oy), Point(sx, sy))
            view = w.draw_zoomed_view(sf, view)

        self._tx_cmd_image_info(view)
        self._tx_cmd_image_data(sf)

    def update_vars(self, d):
        '''Put all the variables received from the GUI in the given dictionary.
        '''
        d['gui'] = self
        new_points = self._points['new']
        old_points = self._points['old']
        for name, point in new_points.items():
            point.gui = GUIPointAttrs(name, old_value=old_points.get(name))
        d.update(new_points)

    def move(self, **kwargs):
        if self._gui_tx_pipe is None:
            return
        for name, value in kwargs.items():
            self._gui_tx_pipe.send(('script_move_point',
                                    (name,) + tuple(value)))


gui = GUIGate()
