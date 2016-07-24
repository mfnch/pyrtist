from base import combination
from point import PointTaker
from path import Path
from window import Window
from cmd_stream import CmdStream, Cmd
from style import Color, Style, Stroke

class Close(object):
    def __init__(self, value=True):
        self.value = value

class Line(PointTaker):
    def __init__(self, *args):
        self.style = Style()
        self.close = False
        super(Line, self).__init__(*args)

@combination(Close, Line)
def fn(close, line):
    line.close = close.value

@combination(Color, Line)
def fn(color, line):
    line.style(color)

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
    window.take(Stroke(Path(line), line.style))
