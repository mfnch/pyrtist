'''Lath object: allows drawing objects with cylindrical symmetry.'''

__all__ = ('Lathe',)

from ..lib2d.base import Taker, combination
from ..lib2d import Point, Radii, Window, Poly, Color, Axes
from .core_types import Point3, Z
from .profile import Profile
from .cmd_stream import Cmd, CmdStream
from .deep_window import DeepWindow
from .primitive import Primitive


class Lathe(Primitive):
    def __init__(self, *args):
        self.radii = None
        self.points = []
        self.profile = None
        super(Lathe, self).__init__(*args)

    def get_profile(self, *args):
        p1, p2, p3 = (p.xy for p in self.get_points())
        p4 = p2 + p3 - p1
        return Poly(p1, p2, p4, p3, *args)

    def get_points(self):
        # TODO: this can be shared with Cylinder.
        if len(self.points) != 2:
            raise TypeError('Cylinder requires exactly 2 points ({} given)'
                            .format(len(self.points)))
        # Get the z component, if the user specified it, otherwise use 0.
        old_p1, old_p2 = self.points
        z = getattr(old_p1, 'z', getattr(old_p2, 'z', 0.0))

        # Ensure the points are Point3 instances with the right z.
        p1 = Point3(0, 0, z)
        p1.set(old_p1)
        p2 = Point3(0, 0, z)
        p2.set(old_p2)
        if p1.z != p2.z:
            raise ValueError('Cylinder points should have the same coordinate')

        axis = (p2 - p1).xy
        r, rz = self.get_radii()
        p3 = p1 + Point3(axis.ort().normalized()*r, rz)
        return (p1, p2, p3)

    def get_radii(self):
        if self.radii is None:
            return (1.0, 1.0)
        self.radii.check(0, 2)
        return (type(self.radii) + (1.0, 1.0))[:2]

@combination(Radii, Lathe)
def radii_at_lathe(radii, lathe):
    if self.radii is not None:
        raise TypeError('Lathe already got a radius')
    lathe.radii = radii

@combination(int, Lathe)
@combination(float, Lathe)
def point3_at_lathe(scalar, lathe):
    lathe.take(Radii(scalar))

@combination(Point, Lathe)
@combination(Point3, Lathe)
def point3_at_lathe(p, lathe):
    lathe.points.append(p)

@combination(Profile, Lathe)
def profile_at_lathe(profile, lathe):
    if lathe.profile is not None:
        raise TypeError('Lathe already got a Profile')
    lathe.profile = profile

@combination(Lathe, CmdStream)
def lathe_at_cmd_stream(lathe, cmd_stream):
    p1, p2, p3 = lathe.get_points()
    fn = lathe.profile.change_axes(Axes(p1.xy, p2.xy, p3.xy)).get_function()
    cmd_stream.take(Cmd(Cmd.src_draw, lathe.get_window()),
                    Cmd(Cmd.on_lathe, p1, p2, p3, fn),
                    Cmd(Cmd.transfer))


@combination(Lathe, DeepWindow)
def lathe_at_deep_window(lathe, deep_window):
    deep_window.take(CmdStream(lathe))
