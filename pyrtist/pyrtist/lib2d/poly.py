__all__ = ('Poly', 'Rectangle')

from .base import *
from .core_types import *
from .path import Path
from .window import Window
from .cmd_stream import CmdStream, Cmd
from .pattern import Pattern
from .style import *

class Poly(Taker):
    def __init__(self, *args):
        self.style = Style()
        self.close = True
        self.points = []
        self.margins = []
        self.num_margins = 0
        self.margin_left = 0.0
        self.margin_right = 0.0
        super(Poly, self).__init__(*args)

    def __len__(self):
        return len(self.points)

    def __iter__(self):
        return iter(self.points)

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

@combination(Pattern, Poly)
@combination(StrokeStyle, Poly)
@combination(Style, Poly)
def fn(child, poly):
    poly.style.take(child)

@combination(Poly, Path)
def fn(poly, path):
    cmd_stream = path.cmd_stream  # This is all we need from `path`.
    points = poly.points
    if len(points) >= 3:
        # Correct margins (this is quite tricky, but is needed to get Poly
        # behave how the user expects).
        margins = poly.margins
        margins_at_0 = ((poly.margin_left, margins[0][1])
                        if len(margins) > 1 else
                        (margins[0][0], poly.margin_right))

        vertex0 = points[-2]
        vertex1 = points[-1]
        prev_margin = margins[-2][1]
        point0 = vertex1 + (vertex0 - vertex1)*prev_margin

        cmd_stream(Cmd(Cmd.move_to, point0))

        for i, vertex2 in enumerate(poly.points):
             margin_left, margin_right = (margins[i] if i > 0
                                          else margins_at_0)
             degenerate_corner = (prev_margin == 0.0) or (margin_left == 0.0)

             segment_vector = vertex2 - vertex1
             point1 = vertex1 + margin_left*segment_vector
             point2 = vertex2 - margin_right*segment_vector

             if degenerate_corner:
                if prev_margin != 0.0:
                    cmd_stream.take(Cmd(Cmd.line_to, vertex1))
                cmd_stream.take(Cmd(Cmd.line_to, point2))
             else:
                cmd_stream.take(Cmd(Cmd.ext_joinarc_to,
                                    point0, vertex1, point1))
                cmd_stream.take(Cmd(Cmd.line_to, point2))

             prev_margin = margin_right
             point0 = point2
             vertex0 = vertex1
             vertex1 = vertex2

        # Close the polygon, if requested
        if poly.close:
            cmd_stream(Cmd(Cmd.close_path))

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

@combination(Pattern, Rectangle)
@combination(StrokeStyle, Rectangle)
@combination(Style, Rectangle)
def fn(child, rectangle):
    rectangle.style.take(child)

@combination(Rectangle, Window, 'Rectangle')
def fn(rectangle, window):
    c1 = rectangle.corner1
    c2 = rectangle.corner2
    p = Poly(c1, Point(c1.x, c2.y), c2, Point(c2.x, c1.y))
    p.style = rectangle.style
    window(p)
