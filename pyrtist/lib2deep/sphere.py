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

'''Sphere object: allows drawing on top of a sphere surface.'''

__all__ = ('Sphere',)

from ..lib2d import Circle, Point
from ..lib2d.base import combination, RejectError
from .core_types import Point3, Z
from .cmd_stream import Cmd, CmdStream
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

    def build_depth_cmd(self):
        center, radii = (self.center, self.radii)
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
        return [Cmd(Cmd.on_sphere, center_2d, one_zero, zero_one,
                    Z(center.z), Z(center.z + rz))]

@combination(Point, Sphere)
@combination(Point3, Sphere)
def center_at_sphere(center, sphere):
    if sphere.center is not None:
        raise ValueError('Sphere already has a center')
    sphere.center = Point3(center)

@combination(int, Sphere)
@combination(float, Sphere)
def scalar_at_sphere(scalar, circle):
    if len(circle.radii) >= 3:
        raise RejectError()
    circle.radii.append(float(scalar))
