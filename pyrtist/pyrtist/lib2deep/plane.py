'''Plane object: allows drawing on a plane.'''

__all__ = ('Plane',)

from ..lib2d import Window, BBox, Point, Rectangle
from ..lib2d.base import Taker, combination
from .core_types import Point3
from .cmd_stream import CmdStream, Cmd
from .deep_window import DeepWindow

class Plane(Taker):
    def __init__(self, *args):
        self.window = None
        self.points = []
        super(Plane, self).__init__(*args)

    def get_window(self):
        if self.window is not None:
            return self.window
        w = Window()
        bb = BBox(p.xy for p in self.points)
        if not bb:
            raise ValueError('Plane needs a Window object')
        w.take(Rectangle(*bb))
        return w

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


@combination(Window, Plane)
def window_at_plane(window, plane):
    plane.window = window

@combination(Point, Plane)
def point_at_plane(point, plane):
    plane.take(Point3(point))

@combination(Point3, Plane)
def point3_at_plane(point, plane):
    if len(plane.points) >= 3:
        raise ValueError('Plane can pass through 3 points at maximum')
    plane.points.append(point)

@combination(Plane, CmdStream)
def plane_at_cmd_stream(plane, cmd_stream):
    points = plane.get_points()

    normal = (points[1] - points[0]).vec_prod(points[2] - points[0])
    if abs(normal.normalized().dot(Point3(0, 0, 1))) < 1e-4:
        raise ValueError('Plane is ortogonal to plane of view')

    cmd_stream.take(Cmd(Cmd.src_draw, plane.get_window()),
                    Cmd(Cmd.on_plane, *points),
                    Cmd(Cmd.transfer))

@combination(Plane, DeepWindow)
def plane_at_deep_window(plane, deep_window):
    deep_window.take(CmdStream(plane))
