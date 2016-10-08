'''Sphere object: allows drawing on top of a sphere surface.'''

__all__ = ('Sphere',)

from ..lib2d import Window, Circle, Point, Color
from ..lib2d.base import Taker, combination, RejectError
from .core_types import Point3, Z
from .cmd_stream import Cmd, CmdStream
from .deep_window import DeepWindow
from .primitive import Primitive


class Sphere(Primitive):
    def __init__(self, *args):
        self.center = None
        self.radii = []
        super(Sphere, self).__init__(*args)

    def get_profile(self, *args):
        extra_args = tuple(self.radii[:2] if len(self.radii) > 2
                           else self.radii) + args
        return Circle(self.center.xy, *extra_args)

@combination(Point3, Sphere)
def center_at_sphere(center, sphere):
    if sphere.center is not None:
        raise ValueError('Sphere already has a center')
    sphere.center = center

@combination(int, Sphere)
@combination(float, Sphere)
def scalar_at_sphere(scalar, circle):
    if len(circle.radii) >= 3:
        raise RejectError()
    circle.radii.append(float(scalar))

@combination(Sphere, CmdStream)
def sphere_at_cmd_stream(sphere, cmd_stream):
    center, radii = (sphere.center, sphere.radii)
    if len(radii) == 0 or center is None:
        detail = ('center' if center is None else 'radius')
        raise ValueError('Sphere without {} cannot be passed to CmdStream'
                         .format(detail))

    if len(radii) == 1:
        rx = ry = rz = radii[0]
    elif len(radii) == 3:
        rx, ry, rz = radii
    elif len(radii) == 2:
        rx = ry = radii[0]
        rz = radii[1]

    center_2d = Point(center.x, center.y)
    one_zero = center_2d + Point.vx(rx)
    zero_one = center_2d + Point.vy(ry)
    cmd_stream.take(Cmd(Cmd.src_draw, sphere.get_window()),
                    Cmd(Cmd.on_sphere, center_2d, one_zero, zero_one,
                        Z(center.z), Z(rz)),
                    Cmd(Cmd.transfer))
