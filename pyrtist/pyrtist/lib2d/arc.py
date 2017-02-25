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

__all__ = ('Arc',)

import math

from .core_types import Point, Close, Through
from .primitive import Primitive
from .base import combination
from .circle import Circle
from .cmd_stream import Cmd


class Arc(Primitive):
    def __init__(self, *args):
        super(Arc, self).__init__()
        self.circle = Circle()
        self.angle_begin = None
        self.angle_end = None
        self.direction = 1
        self.close = Close.yes
        self.take(*args)

    def __getitem__(self, index):
        c = self.circle
        rx, ry = c.get_radii()
        if self.direction > 0:
            angle = self.angle_begin*(1.0 - index) + self.angle_end*index
        else:
            angle = self.angle_begin*(1.0 - index) + (self.angle_end - 2*math.pi)*index
        return c.center + Point(rx*math.cos(angle), ry*math.sin(angle))

    def get_angles(self):
        angle_begin = self.angle_begin
        angle_end = self.angle_end

    def build_path(self):
        radius = self.circle.radii[0]
        center = self.circle.center
        start = center + radius*Point.vangle(self.angle_begin)
        one_zero = center + Point(radius, 0.0)
        zero_one = center + Point(0.0, radius)
        cmds = [Cmd(Cmd.move_to, start),
                Cmd(Cmd.ext_arc_to, center, one_zero, zero_one,
                    self.angle_begin, self.angle_end, self.direction)]
        if self.close == Close.yes:
            cmds.append(Cmd(Cmd.close_path))
        return cmds


@combination(Close, Arc)
def close_at_arc(close, arc):
    arc.close = close

@combination(Through, Arc)
def point_at_arc(through, arc):
    arc.circle.take(through)
    center = arc.circle.center
    angle_inside = (through[1] - center).angle()
    angle_begin = (through[0] - center).angle()
    angle_end = (through[2] - center).angle()
    if angle_end < angle_begin:
        angle_end += 2.0*math.pi
    if angle_inside < angle_begin:
        angle_inside += 2.0*math.pi
    arc.angle_end = angle_end
    arc.angle_begin = angle_begin
    arc.direction = (1 if (angle_begin <= angle_inside <= angle_end)
                     else -1)
