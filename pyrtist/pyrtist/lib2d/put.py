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

__all__ = ('Put', 'SimplePut', 'Near')

import math

from .base import *
from .core_types import *
from .cmd_stream import *
from .window import Window
from .transform import Transform
from .auto_transform import Constraint as Near, AutoTransform


class SimplePut(object):
    def __init__(self, window, matrix=None):
        self.window = window
        self.matrix = matrix

    def get_window(self):
        if self.window is None:
            raise ValueError('No window was given to SimplePut')
        return self.window

@combination(SimplePut, Window, 'SimplePut')
def fn(simple_put, window):
    flt = CmdArgFilter.from_matrix(simple_put.matrix or Matrix())
    window.cmd_stream.add(simple_put.window.cmd_stream, flt)


class Put(Taker):
    def __init__(self, *args):
        super(Put, self).__init__()
        self.window = None
        self.transform = Transform()
        self.matrix = None
        self.auto_transform = AutoTransform()
        self.constraints = []
        self.take(*args)
        self.place()

    def place(self):
        resolved_constraints = []
        for c in self.constraints:
            src, dst, weight = (c.src, Point(c.dst), float(c.weight))
            if not isinstance(src, Point):
                reference_point = self.window.get(src)
                if reference_point is None:
                    raise ValueError('Cannot find reference point {}'
                                     .format(repr(src)))
                src = reference_point
            resolved_constraints.append(Near(Point(src), dst, weight))

        self.transform = \
          self.auto_transform.calculate(self.transform, resolved_constraints)
        self.matrix = self.transform.get_matrix()
        return self.transform

    def __getitem__(self, key):
        return self.matrix.apply(self.window[key])


@combination(Window, Put)
def window_at_put(window, put):
    put.window = window

@combination(str, Put)
def str_at_put(transform_str, put):
    put.auto_transform = AutoTransform.from_string(transform_str)

@combination(Transform, Put)
def transform_at_put(transform, put):
    put.transform = transform

@combination(tuple, Put)
@combination(Point, Put)
def point_at_put(point, put):
    put.transform.translation = point

@combination(Scale, Put)
def scale_at_put(scale, put):
    put.transform.scale_factors = scale

@combination(Center, Put)
def scale_at_put(center, put):
    put.transform.rotation_center = center

@combination(AngleDeg, Put)
def scale_at_put(angle, put):
    put.transform.rotation_angle = angle*math.pi/180.0

@combination(Near, Put)
def near_at_put(near, put):
    put.constraints.append(near)

@combination(Put, Window, 'Put')
def put_at_window(put, window):
    transform = put.place()
    flt = CmdArgFilter.from_matrix(transform.get_matrix())
    window.cmd_stream.add(put.window.cmd_stream, flt)
