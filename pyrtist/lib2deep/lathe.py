# Copyright (C) 2017 by Matteo Franchin (fnch@users.sf.net)
#
# This file is part of Pyrtist.
#   Pyrtist is free software: you can redistribute it and/or modify it
#   under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Pyrtist is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

'''Lath object: allows drawing objects with cylindrical symmetry.'''

__all__ = ('Lathe',)

from ..lib2d.base import combination
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
        v = p3 - p1
        return Poly(p1 + v, p2 + v, p2 - v, p1 - v, *args)

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
            raise ValueError('Lathe points should have the same z coordinate')

        axis = (p2 - p1).xy
        r, rz = self.get_radii()
        p3 = p1 + Point3(axis.ort().normalized()*r, rz)
        return (p1, p2, p3)

    def get_radii(self):
        if self.radii is None:
            return (1.0, 1.0)
        self.radii.check(0, 2)
        return (tuple(self.radii) + (1.0, 1.0))[:2]

    def build_depth_cmd(self):
        p1, p2, p3 = self.get_points()
        fn = self.profile.change_axes(Axes(p1.xy, p2.xy, p3.xy)).get_function()
        return [Cmd(Cmd.on_circular, p1, p2, p3, fn)]

@combination(Radii, Lathe)
def radii_at_lathe(radii, lathe):
    if lathe.radii is not None:
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
