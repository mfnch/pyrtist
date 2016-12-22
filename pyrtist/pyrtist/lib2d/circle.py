__all__ = ('Circle', 'Circles')

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
def circle_at_window(circle, window):
    window.take(CmdStream(circle))


class Circles(Primitive):
    '''Convenience object that makes it easy to create a number of Circle that
    all share the same center or the same radius.
    '''

    def __init__(self, *args):
        super(Circles, self).__init__()
        self.circles = []
        self.take(*args)

    def __len__(self):
        return len(self.circles)

    def _get_memb(self, idx, memb_idx):
        memb = self.circles[idx][memb_idx]
        if memb is None:
            while idx >= 0:
                prev_memb = self.circles[idx][memb_idx]
                if prev_memb is not None:
                    memb = prev_memb
                    break
        assert memb is not None, 'Malformed Circles object'
        return memb

    def _get_tuples(self):
        out = []
        prev_center = None
        prev_radius = None
        for center, radius in self.circles:
            item = (center or prev_center, radius or prev_radius)
            prev_center, prev_radius = item
            out.append(item)
            assert prev_center is not None and prev_radius is not None, \
              'Malformed Circles object'
        return out

    def __iter__(self):
        return iter(Circle(center, radius)
                    for center, radius in self._get_tuples())

    def __getitem__(self, idx):
        return Circle(self._get_memb(idx, 0), self._get_memb(idx, 1))


@combination(int, Circles)
@combination(float, Circles)
@combination(Point, Circles)
def scalar_at_circle(attr, circles):
    attr_idx = (0 if isinstance(attr, Point) else 1)
    if len(circles.circles) > 0:
        last_circle = circles.circles[-1]
        if last_circle[attr_idx] is None:
            last_circle[attr_idx] = attr
            return
    circle = [None, None]
    circle[attr_idx] = attr
    circles.circles.append(circle)
