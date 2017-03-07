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

__all__ = ('Line',)

import math
import copy

from .base import *
from .core_types import *
from .path import Path
from .window import Window
from .cmd_stream import CmdStream, Cmd
from .style import *
from .put import Put, Near
from .transform import Transform


class Line(PointTaker):
    def __init__(self, *args):
        self.stroke_style = Border()
        self.close = False
        self.arrows = None
        self.scale = Scale(1.0)
        super(Line, self).__init__(*args)

    def __getitem__(self, index):
        if isinstance(index, int):
            return self.points[index]
        n = len(self.points)
        prev_idx = math.floor(index)
        prev = self.points[int(prev_idx) % n]
        succ = self.points[int(math.ceil(index)) % n]
        x = index - prev_idx
        return prev*(1.0 - x) + succ*x

    def split_segments_inplace(self, num_parts):
        '''In-place version of split_segments().'''
        assert num_parts >= 1, 'Invalid argument to split()'
        if len(self.points) < 2 or num_parts == 1:
            return
        prev = self.points[0]
        out_points = []
        for point in self.points[1:]:
            dv = (point - prev)/num_parts
            for i in range(num_parts):
                out_points.append(prev + dv*i)
            prev = point
        out_points.append(prev)
        self.points = out_points

    def split_segments(self, num_parts):
        '''Return a new Line with all segments split in `num_parts` parts.'''
        ret = copy.deepcopy(self)
        ret.split_segments_inplace(num_parts)
        return ret


@combination(Close, Line)
def close_at_line(close, line):
    line.close = (close == Close.yes)

@combination(Scale, Line)
def scale_at_line(scale, line):
    line.scale = scale

@combination(int, Line)
@combination(float, Line)
@combination(Color, Line)
@combination(Cap, Line)
@combination(Join, Line)
@combination(StrokeStyle, Line)
def style_at_line(child, line):
    line.stroke_style.take(child)

@combination(Line, Path)
def line_at_path(line, path):
    if len(line) > 0:
        points = iter(line)
        path.cmd_stream(Cmd(Cmd.move_to, points.next()))
        path.cmd_stream(Cmd(Cmd.line_to, point) for point in points)
        if line.close:
            path.cmd_stream(Cmd(Cmd.close_path))

def _place_arrow(line, idx, head_point, tail_point, arrow_window):
    scale, arrow = line.arrows[idx]
    head = arrow['head']
    if line.stroke_style is not None:
        width = line.stroke_style.width
        if width is not None:
            scale *= width
    scale = 0.5 * scale / (head - arrow['tail']).norm()
    t = Transform(translation=head_point - head,
                  scale_factors=scale,
                  rotation_center=head)
    placed_arrow = Put(arrow, t, 'r', Near('tail', tail_point))
    arrow_window.take(placed_arrow)
    return placed_arrow['join']

@combination(Line, Window)
def line_at_window(line, window):
    if len(line.points) < 2:
        return
    if line.arrows is None:
        window.take(Stroke(Path(line), line.stroke_style))
        return
    else:
        if line.close:
            raise NotImplementedError('Cannot place arrows on closed lines')

    path = Path()
    cmds = path.cmd_stream
    arrow_window = Window()

    final_idx = len(line.points) - 1
    point_iter = enumerate(line.points)
    start_idx, start_point = point_iter.next()
    for end_idx, end_point in point_iter:
        if start_idx == 0:
            if start_idx in line.arrows:
                # This is the start arrow.
                sp = _place_arrow(line, start_idx, start_point, end_point,
                                  arrow_window)
            else:
                sp = start_point
            cmds.take(Cmd(Cmd.move_to, sp))

        if end_idx < final_idx:
            cmds.take(Cmd(Cmd.line_to, end_point))
        else:
            if end_idx in line.arrows:
                # This is the end arrow.
                ep = _place_arrow(line, end_idx, end_point, start_point,
                                  arrow_window)
            else:
                ep = end_point
            cmds.take(Cmd(Cmd.line_to, ep))

        start_idx, start_point = (end_idx, end_point)

    window.take(Stroke(path, line.stroke_style), arrow_window)

@combination(Window, Line)
def window_at_line(window, line):
    if line.arrows is None:
        line.arrows = {}
    idx = max(0, len(line.points) - 1)
    if idx not in line.arrows:
        line.arrows[idx] = (line.scale, window)
    elif idx + 1 not in line.arrows:
        line.arrows[idx + 1] = (line.scale, window)
    else:
        raise ValueError('Too many windows given to Line')
