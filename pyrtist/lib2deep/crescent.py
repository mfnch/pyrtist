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

'''Crescent object: allows drawing a crescent-shaped cylindrical object.'''

__all__ = ('Crescent',)

from ..lib2d import Point, Window, Arc, Through, Close, Fill, Radii
from ..lib2d.base import combination
from .core_types import Point3
from .cmd_stream import Cmd
from .primitive import Primitive, InvalidPrimitiveError


class Crescent(Primitive):
    def __init__(self, *args):
        self.points = []
        self.scale_z = 1.0
        self.z = None
        self.radius_z = None
        super(Crescent, self).__init__(*args)

    def get_points(self):
        if len(self.points) != 4:
            raise InvalidPrimitiveError('Crescent needs exactly 4 points')
        return self.points

    def get_radius_z(self):
        p1, p2, p3, p4 = self.get_points()
        arc1 = Arc(Through(p1, p2, p3))
        arc2 = Arc(Through(p3, p4, p1))
        radius = (arc1[0.5] - arc2[0.5]).norm()*0.5
        return radius

    def get_profile(self, *args):
        p1, p2, p3, p4 = self.get_points()
        return Fill(Arc(Through(p1, p2, p3), Close.no),
                    Arc(Through(p3, p4, p1), Close.no), *self.extra_args)

    def build_depth_cmd(self):
        p1, p2, p3, p4 = self.get_points()
        origin = Point3((p1 + p3)*0.5, self.z or 0.0)
        one_zero = Point3(p3, origin.z + self.get_radius_z()*self.scale_z)
        zero_one = origin + Point3((one_zero - origin).xy.ort())
        return [Cmd(Cmd.on_crescent, origin, one_zero, zero_one,
                    Point3(p2), Point3(p4))]


@combination(Point, Crescent)
def point_at_crescent(point, crescent):
    crescent.points.append(point)

@combination(Point3, Crescent)
def point3_at_crescent(point3, crescent):
    if crescent.z is None:
        crescent.z = point3.z
    else:
        if point3.z != crescent.z:
            raise ValueError('All Point3 given to Crescent should have '
                             'the same z')
    crescent.points.append(point3.xy)

@combination(int, Crescent)
@combination(float, Crescent)
def scalar_at_crescent(scalar, crescent):
    crescent.scale_z = scalar

@combination(Radii, Crescent)
def radii_at_crescent(radii, crescent):
    radii.check(1, 1)
    radius, = radii
    if radius <= 0.0:
        raise ValueError('Crescent only takes positive radius. '
                         'Use a negative scale to invert the depth along z.')
    crescent.radius_z = radius
