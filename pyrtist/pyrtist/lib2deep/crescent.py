'''Crescent object: allows drawing a crescent-shaped cylindrical object.'''

__all__ = ('Crescent',)

from ..lib2d import Point, Window, Arc, Through, Close, Fill
from ..lib2d.base import combination
from .core_types import Point3
from .cmd_stream import Cmd
from .primitive import Primitive, InvalidPrimitiveError


class Crescent(Primitive):
    def __init__(self, *args):
        self.points = []
        super(Crescent, self).__init__(*args)

    def get_points(self):
        if len(self.points) != 4:
            raise InvalidPrimitiveError('Crescent needs exactly 4 points')
        return self.points

    def get_profile(self, *args):
        p1, p2, p3, p4 = tuple(p.xy for p in self.get_points())
        return Fill(Arc(Through(p1, p2, p3), Close.no),
                    Arc(Through(p3, p4, p1), Close.no), *self.extra_args)

    def build_depth_cmd(self):
        p1, p2, p3, p4 = self.get_points()
        origin = Point3(((p1 + p3)*0.5).xy, p1.z)
        one_zero = p3
        zero_one = origin + Point3((one_zero - origin).xy.ort())
        return [Cmd(Cmd.on_crescent, origin, one_zero, zero_one, p2, p4)]


@combination(Point, Crescent)
@combination(Point3, Crescent)
def point_at_crescent(point, crescent):
    crescent.points.append(Point3(point))
