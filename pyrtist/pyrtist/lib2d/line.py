__all__ = ('Line',)

import math
import copy

from .base import *
from .core_types import *
from .path import Path
from .window import Window
from .cmd_stream import CmdStream, Cmd
from .style import *

class Line(PointTaker):
    def __init__(self, *args):
        self.stroke_style = Border()
        self.close = False
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
def fn(close, line):
    line.close = (close == Close.yes)

@combination(int, Line)
@combination(float, Line)
@combination(Color, Line)
@combination(Cap, Line)
@combination(Join, Line)
@combination(StrokeStyle, Line)
def fn(child, line):
    line.stroke_style.take(child)

@combination(Line, Path)
def fn(line, path):
    if len(line) > 0:
        points = iter(line)
        path.cmd_stream(Cmd(Cmd.move_to, points.next()))
        path.cmd_stream(Cmd(Cmd.line_to, point) for point in points)
        if line.close:
            path.cmd_stream(Cmd(Cmd.close_path))

@combination(Line, Window, 'Line')
def fn(line, window):
    window.take(Stroke(Path(line), line.stroke_style))
