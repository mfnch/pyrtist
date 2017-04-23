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

__all__ = ('Poly', 'Rectangle')

from .base import *
from .core_types import *
from .path import Path
from .window import Window
from .cmd_stream import CmdStream, Cmd
from .style import *
from .primitive import Primitive


class Poly(Primitive):
    def __init__(self, *args):
        super(Poly, self).__init__()
        self.close = Close.yes
        self.points = []
        self.margins = []
        self.num_margins = 0
        self.margin_left = 0.0
        self.margin_right = 0.0
        self.take(*args)

    def __len__(self):
        return len(self.points)

    def __iter__(self):
        return iter(self.points)

    def build_path(self):
        cmds = []
        points = self.points
        if len(points) < 3:
            return cmds

        # Correct margins (this is quite tricky, but is needed to get Poly
        # behave how the user expects)
        margins = self.margins
        margins_at_0 = ((self.margin_left, margins[0][1])
                        if len(margins) > 1 else
                        (margins[0][0], self.margin_right))

        vertex0 = points[-2]
        vertex1 = points[-1]
        prev_margin = margins[-2][1]
        point0 = vertex1 + (vertex0 - vertex1)*prev_margin

        cmds.append(Cmd(Cmd.move_to, point0))

        for i, vertex2 in enumerate(self.points):
             margin_left, margin_right = (margins[i] if i > 0
                                          else margins_at_0)
             degenerate_corner = (prev_margin == 0.0) or (margin_left == 0.0)

             segment_vector = vertex2 - vertex1
             point1 = vertex1 + margin_left*segment_vector
             point2 = vertex2 - margin_right*segment_vector

             if degenerate_corner:
                if prev_margin != 0.0:
                    cmds.append(Cmd(Cmd.line_to, vertex1))
                cmds.append(Cmd(Cmd.line_to, point2))
             else:
                cmds.append(Cmd(Cmd.ext_joinarc_to,
                                point0, vertex1, point1))
                cmds.append(Cmd(Cmd.line_to, point2))

             prev_margin = margin_right
             point0 = point2
             vertex0 = vertex1
             vertex1 = vertex2

        # Close the polygon, if requested
        if self.close == Close.yes:
            cmds.append(Cmd(Cmd.close_path))
        return cmds

@combination(Point, Poly)
def point_at_poly(point, poly):
    poly.points.append(point)
    poly.margins.append((poly.margin_left, poly.margin_right))
    poly.num_margins = 0
    poly.margin_right, poly.margin_left = (poly.margin_left, poly.margin_right)

@combination(tuple, Poly)
def tuple_at_poly(tp, poly):
    poly.take(Point(tp))

@combination(int, Poly)
@combination(float, Poly)
def scalar_at_poly(scalar, poly):
    if poly.num_margins == 0:
        poly.margin_left = ml = max(0.0, min(1.0, scalar))
        poly.margin_right = max(0.0, min(1.0 - ml, ml))
    elif poly.num_margins == 1:
        poly.margin_right = max(0.0, min(1.0 - poly.margin_left, scalar))
    else:
        raise TypeError('Only two margins can be given between two points')
    poly.num_margins += 1

@combination(Close, Poly)
def fn(close, poly):
    poly.close = close

@combination(Poly, CmdStream)
def fn(poly, cmd_stream):
    cmd_stream.take(Path(poly), poly.style)

@combination(Poly, Window, 'Poly')
def fn(poly, window):
    window.take(CmdStream(poly))

class Rectangle(PointTaker, Primitive):
    def __init__(self, *args):
        super(Rectangle, self).__init__()
        corners = [arg for arg in args if isinstance(arg, Point)]
        if len(corners) != 2:
            raise TypeError('Rectangle takes exactly 2 points')
        self.corner1, self.corner2 = corners
        self.take(*args)

    def build_path(self):
        c1 = self.corner1
        c2 = self.corner2
        p = Poly(c1, Point(c1.x, c2.y), c2, Point(c2.x, c1.y))
        return p.build_path()

@combination(Rectangle, Window, 'Rectangle')
def fn(rectangle, window):
    window.take(CmdStream(rectangle))
