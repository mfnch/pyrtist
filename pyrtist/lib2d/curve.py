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

__all__ = ('Curve',)

from .base import Taker, combination
from .core_types import Point, Tri, Close
from .path import Path
from .window import Window
from .cmd_stream import CmdStream, Cmd
from .style import Style

class Curve(Taker):
    def __init__(self, *args):
        self.style = Style()
        self.close = True
        self.tris = []
        super(Curve, self).__init__(*args)

@combination(Tri, Curve)
def tri_at_curve(tri, curve):
    curve.tris.append(tri)

@combination(Point, Curve)
def point_at_curve(point, curve):
    curve.take(Tri(point))

@combination(Close, Curve)
def fn(close, curve):
    curve.close = (close == Close.yes)

@combination(Curve, Path)
def curve_at_path(curve, path):
    if len(curve.tris) >= 2:
        tris = iter(curve.tris)
        prev_tri = first_tri = tris.next()
        path.cmd_stream.take(Cmd(Cmd.move_to, first_tri.p))

        for tri in tris:
            path.cmd_stream.take(Cmd(Cmd.curve_to, prev_tri.op, tri.ip, tri.p))
            prev_tri = tri

        if curve.close:
            path.cmd_stream.take(Cmd(Cmd.curve_to, prev_tri.op, first_tri.ip,
                                     first_tri.p))

@combination(Curve, CmdStream)
def curve_at_cmd_stream(curve, cmd_stream):
    cmd_stream.take(Path(curve), curve.style)

@combination(Curve, Window, 'Curve')
def fn(curve, window):
    window.take(CmdStream(curve))
