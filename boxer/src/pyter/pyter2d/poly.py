from base import combination
from point import PointTaker
from path import Path
from window import Window
from cmd_stream import CmdStream, Cmd
from style import Color, Style, Stroke
from point import Point

class Poly(PointTaker):
    def __init__(self, *args):
        self.style = Style()
        self.close = False
        super(Poly, self).__init__(*args)

@combination(Color, Poly)
def fn(color, poly):
    poly.style(color)

@combination(Poly, Window, 'Poly')
def fn(poly, window):
    window.cmd_stream(Path(poly), poly.style)

@combination(Poly, Path)
def fn(poly, path):
    if len(poly) > 0:
        points = iter(poly)
        path.cmd_stream(Cmd(Cmd.move_to, points.next()))
        path.cmd_stream(Cmd(Cmd.line_to, point) for point in points)
        if poly.close:
            path.cmd_stream(Cmd(Cmd.close_path))

class Rectangle(PointTaker):
    def __init__(self, corner1, corner2, *args):
        self.style = Style()
        self.corner1 = Point(corner1)
        self.corner2 = Point(corner2)
        super(Rectangle, self).__init__(*args)

@combination(Rectangle, Window, 'Rectangle')
def fn(rectangle, window):
    c1 = rectangle.corner1
    c2 = rectangle.corner2
    p = Poly(c1, Point(x=c1.x, y=c2.y), c2, Point(x=c2.x, y=c1.y))
    p.style = rectangle.style
    window(p)
