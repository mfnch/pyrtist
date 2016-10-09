'''Plane object: allows drawing on a plane.'''

__all__ = ('Plane',)

from ..lib2d import BBox, Point, Rectangle
from ..lib2d.base import combination
from .core_types import Point3
from .cmd_stream import CmdStream, Cmd
from .primitive import Primitive


class Plane(Primitive):
    def __init__(self, *args):
        self.points = []
        super(Plane, self).__init__(*args)

    def get_profile(self, *args):
        bb = BBox(p.xy for p in self.points)
        if not bb:
            raise ValueError('Plane needs a Window object')
        r = Rectangle(*bb)
        r.take(*args)
        return r

    def get_points(self):
        points = self.points
        n = len(points)
        if n > 2:
            return points
        if n == 0:
            return (Point3(), Point3(1), Point3(0, 1))
        if n == 1:
            p = points[0]
            return (p, Point3(1, 0, p.z), Point3(0, 1, p.z))
        assert n == 2
        p = points[0] + Point3((points[1] - points[0]).xy.ort())
        return (points[0], points[1], p)

    def build_shape_cmd(self):
        points = self.get_points()
        normal = (points[1] - points[0]).vec_prod(points[2] - points[0])
        if abs(normal.normalized().dot(Point3(0, 0, 1))) < 1e-4:
            raise ValueError('Plane is ortogonal to plane of view')
        return [Cmd(Cmd.on_plane, *points)]

@combination(Point, Plane)
def point_at_plane(point, plane):
    plane.take(Point3(point))

@combination(Point3, Plane)
def point3_at_plane(point, plane):
    if len(plane.points) >= 3:
        raise ValueError('Plane can pass through 3 points at maximum')
    plane.points.append(point)
