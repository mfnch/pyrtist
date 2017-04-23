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

'''Implementation of Put functionality for DeepWindows.

This module allows passing Put objects to DeepWindow in order to place a
DeepWindow inside another DeepWindow, similarly to what is done for Windows
in the pyrtist.lib2d module.
'''

__all__ = ('SimplePut', 'Put', 'Near')

from ..lib2d.base import combination
from ..lib2d import SimplePut, Put, Near
from .core_types import Point, Point3, DeepMatrix
from .deep_window import DeepWindow
from .cmd_stream import DeepCmdArgFilter


@combination(SimplePut, DeepWindow, 'SimplePut')
def simple_put_at_deep_window(simple_put, deep_window):
    flt = DeepCmdArgFilter.from_matrix(simple_put.matrix or DeepMatrix())
    deep_window.cmd_stream.add(simple_put.get_window().cmd_stream, flt)

@combination(DeepWindow, Put)
def deep_window_at_put(deep_window, put):
    put.window = deep_window

@combination(Put, DeepWindow, 'Put')
def put_at_deep_window(put, deep_window):
    xy_constraints = []
    z_constraints = []
    for c in put.constraints:
        src, dst, weight = (c.src, Point3(c.dst), float(c.weight))
        if not isinstance(src, (Point, Point3)):
            reference_point = put.window.get(src)
            if reference_point is None:
                raise ValueError('Cannot find reference point {}'
                                 .format(repr(src)))
            src = reference_point
        src = Point3(src)
        xy_constraints.append(Near(src.get_xy(), dst.get_xy(), weight))
        z_constraints.append((src.z, dst.z, weight))

    # Calculate the xy part of the matrix.
    t = put.auto_transform.calculate(put.transform, xy_constraints)
    mx = t.get_matrix()
    flt = DeepCmdArgFilter.from_matrix(mx)
    deep_window.cmd_stream.add(put.window.cmd_stream, flt)
