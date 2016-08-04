from base import combination, Close
from point import PointTaker
from path import Path
from window import Window
from cmd_stream import CmdStream, Cmd
from style import *
from point import Point

class Poly(PointTaker):
    def __init__(self, *args):
        self.style = Style()
        self.close = True
        super(Poly, self).__init__(*args)

@combination(Close, Poly)
def fn(close, poly):
    poly.close = close

@combination(Color, Poly)
@combination(StrokeStyle, Poly)
@combination(Style, Poly)
def fn(child, poly):
    poly.style.take(child)

@combination(Poly, Path)
def fn(poly, path):
    if len(poly) > 0:
        points = iter(poly)
        path.cmd_stream(Cmd(Cmd.move_to, points.next()))
        path.cmd_stream(Cmd(Cmd.line_to, point) for point in points)
        if poly.close:
            path.cmd_stream(Cmd(Cmd.close_path))

@combination(Poly, CmdStream)
def fn(poly, cmd_stream):
    cmd_stream.take(Path(poly), poly.style)

@combination(Poly, Window, 'Poly')
def fn(poly, window):
    window.take(CmdStream(poly))

class Rectangle(PointTaker):
    def __init__(self, corner1, corner2, *args):
        self.style = Style()
        self.corner1 = Point(corner1)
        self.corner2 = Point(corner2)
        super(Rectangle, self).__init__(*args)

@combination(Color, Rectangle)
@combination(StrokeStyle, Rectangle)
@combination(Style, Rectangle)
def fn(child, rectangle):
    rectangle.style.take(child)

@combination(Rectangle, Window, 'Rectangle')
def fn(rectangle, window):
    c1 = rectangle.corner1
    c2 = rectangle.corner2
    p = Poly(c1, Point(x=c1.x, y=c2.y), c2, Point(x=c2.x, y=c1.y))
    p.style = rectangle.style
    window(p)
