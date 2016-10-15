__all__ = ('Arc',)

#import math

from .core_types import Point, Close, Through
from .primitive import Primitive
from .base import combination
from .circle import Circle
from .cmd_stream import Cmd


class Arc(Primitive):
    def __init__(self, *args):
        super(Arc, self).__init__()
        self.circle = Circle()
        self.angle_begin = None
        self.angle_end = None
        self.close = Close.yes
        self.take(*args)

    def build_path(self):
        radius = self.circle.radii[0]
        center = self.circle.center
        start = center + radius*Point.vangle(self.angle_begin)
        one_zero = center + Point(radius, 0.0)
        zero_one = center + Point(0.0, radius)
        cmds = [Cmd(Cmd.move_to, start),
                Cmd(Cmd.ext_arc_to, center, one_zero, zero_one,
                    self.angle_begin, self.angle_end)]
        if self.close == Close.yes:
            cmds.append(Cmd(Cmd.close_path))
        return cmds


@combination(Close, Arc)
def close_at_arc(close, arc):
    arc.close = close

@combination(Through, Arc)
def point_at_arc(through, arc):
    arc.circle.take(through)
    center = arc.circle.center
    arc.angle_begin = (through[0] - center).angle()
    arc.angle_end = (through[2] - center).angle()
