__all__ = ('Circle',)

import math

from .core_types import Point, Through
from .primitive import Primitive
from .base import combination, RejectError
from .cmd_stream import CmdStream, Cmd
from .window import Window

class Circle(Primitive):
    def __init__(self, *args):
        super(Circle, self).__init__()
        self.center = None
        self.radii = []
        self.take(*args)

    def get_radii(self):
        if len(self.radii) == 2:
            return self.radii
        rx = self.radii[0]
        return (rx, rx)

    def build_path(self):
        if len(self.radii) == 0 or self.center is None:
            raise ValueError('Circle is missing the center')

        rx, ry = self.get_radii()
        one_zero = self.center + Point.vx(rx)
        zero_one = self.center + Point.vy(ry)
        return [Cmd(Cmd.move_to, one_zero),
                Cmd(Cmd.ext_arc_to, self.center, one_zero, zero_one,
                    0.0, 2.0*math.pi, 1)]

@combination(int, Circle)
@combination(float, Circle)
def fn(scalar, circle):
    if len(circle.radii) >= 2:
        raise RejectError()
    circle.radii.append(float(scalar))

@combination(tuple, Circle)
@combination(Point, Circle)
def point_at_circle(tp, circle):
    circle.center = Point(tp)

@combination(Through, Circle)
def tuple_at_circle(tp, circle):
    if len(tp) != 3:
        raise ValueError('Through object should contain exactly 3 points')
    # Assume the tuple contains 3 points and find the Circle that passes
    # through them.
    p21 = 2.0*(tp[1] - tp[0])
    p31 = 2.0*(tp[2] - tp[0])
    sq1 = tp[0].norm2()
    sq21 = tp[1].norm2() - sq1
    sq31 = tp[2].norm2() - sq1
    det = p21.x*p31.y - p21.y*p31.x
    if det == 0.0:
        raise ValueError('Cannot find the circle passing through '
                         'the three points')
    circle.center = Point(p31.y*sq21 - p21.y*sq31,
                          p21.x*sq31 - p31.x*sq21) / det
    r = (tp[0] - circle.center).norm()
    circle.radii = [r, r]

@combination(Circle, Window, 'Circle')
def fn(circle, window):
    window.take(CmdStream(circle))
