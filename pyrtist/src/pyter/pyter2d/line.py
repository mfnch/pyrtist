from base import *
from base_types import *
from path import Path
from window import Window
from cmd_stream import CmdStream, Cmd
from style import *

class Line(PointTaker):
    def __init__(self, *args):
        self.stroke_style = Border()
        self.close = False
        super(Line, self).__init__(*args)

@combination(Close, Line)
def fn(close, line):
    line.close = close

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
