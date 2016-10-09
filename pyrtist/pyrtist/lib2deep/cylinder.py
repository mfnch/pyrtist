'''Cylinder object: allows drawing on top of a cylindrical surface.'''

__all__ = ('Cylinder',)

from ..lib2d.base import combination, RejectError
from ..lib2d import Point, Radii, Poly
from .core_types import Point3
from .cmd_stream import Cmd, CmdStream
from .primitive import Primitive


class Cylinder(Primitive):
    def __init__(self, *args):
        self.points = []
        self.radii = []
        super(Cylinder, self).__init__(*args)

    def get_profile(self, *args):
        p1, p2 = tuple(p.xy for p in self.get_points())
        (rs, _), (re, _) = self.get_radii()
        ort = (p2 - p1).ort().normalized()
        return Poly(p1 + rs*ort, p2 + re*ort, p2 - re*ort, p1 - rs*ort, *args)

    def get_points(self):
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
        return (p1, p2)

    def get_radii(self):
        if len(self.radii) < 1:
            raise ValueError('Cylinder needs at least one radius')
        if len(self.radii) > 2:
            raise ValueError('Cylinder takes at most two radii')

        ret = []
        for radii in self.radii:
            radii.check(1, 2)
            if len(radii.args) == 1:
                ret.append(Radii(radii.args[0], radii.args[0]))
            else:
                ret.append(radii)

        if len(ret) == 1:
            ret.append(ret[0])
        return (ret[0].args, ret[1].args)

    def build_shape_cmd(self):
        start_point, end_point = self.get_points()
        (rs_xy, rs_z), (re_xy, re_z) = self.get_radii()

        dp = (start_point.xy - end_point.xy).ort().normalized()
        start_edge = start_point + Point3(rs_xy*dp, rs_z)
        end_edge = end_point + Point3(re_xy*dp, re_z)
        return [Cmd(Cmd.on_cylinder, start_point, start_edge,
                    end_point, end_edge)]

@combination(Point, Cylinder)
@combination(Point3, Cylinder)
def point3_at_cylinder(p, cylinder):
    cylinder.points.append(p)

@combination(int, Cylinder)
@combination(float, Cylinder)
def scalar_at_cylinder(scalar, cylinder):
    cylinder.take(Radii(scalar))

@combination(Radii, Cylinder)
def radii_at_cylinder(radii, cylinder):
    if len(cylinder.radii) >= 2:
        raise RejectError('Too many radii given to Cylinder')
    cylinder.radii.append(radii)
