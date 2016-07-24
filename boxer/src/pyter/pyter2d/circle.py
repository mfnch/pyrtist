import math

from base import Taker, combination, RejectException
from point import Point, Px, Py
from cmd_stream import CmdStream, Cmd
from path import Path
from style import Style, Stroke
from window import Window

class Circle(Taker):
    def __init__(self, *args):
        self.style = Style()
        self.center = None
        self.radii = []
        super(Circle, self).__init__(*args)

@combination(int, Circle)
@combination(float, Circle)
def fn(scalar, circle):
    if len(circle.radii) >= 2:
        raise RejectException()
    circle.radii.append(float(scalar))

@combination(Point, Circle)
def fn(point, circle):
    circle.center = point

@combination(tuple, Circle)
def fn(tp, circle):
    if len(tp) != 2:
        raise RejectException()
    return circle.take(Point(tp))

@combination(Circle, Path)
def fn(circle, path):
    if len(circle.radii) == 0 or circle.center is None:
        raise RejectException()

    if len(circle.radii) == 2:
        rx, ry = circle.radii
    else:
        rx = ry = circle.radii[0]

    one_zero = circle.center + Px(rx)
    zero_one = circle.center + Py(ry)
    path.cmd_stream(Cmd(Cmd.move_to, one_zero),
                    Cmd(Cmd.ext_arc_to, circle.center, one_zero, zero_one,
                        0.0, 2.0*math.pi))

@combination(Circle, Window, 'Circle')
def fn(circle, window):
    window.take(Stroke(Path(circle), circle.style))
