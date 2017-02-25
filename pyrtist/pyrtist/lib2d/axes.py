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

__all__ = ('Axes',)

from .base import Taker, combination
from .core_types import Point, Scale, Matrix


class Axes(Taker):
    '''Object describing a reference system with an origin and - not
    necessarily orthogonal - axes.
    '''

    @staticmethod
    def vx(origin, delta_x=1.0):
        return Axes(origin, origin + Point.vx(delta_x))

    @staticmethod
    def vy(origin, delta_y=1.0):
        return Axes(origin, origin + Point.vy(delta_y))

    def __init__(self, *args):
        self.points = []
        super(Axes, self).__init__(*args)

    @property
    def origin(self):
        if len(self.points) >= 1:
            return self.points[0]
        return Point()

    @property
    def one_zero(self):
        if len(self.points) >= 2:
            return self.points[1]
        return self.origin + Point.vx()

    @property
    def zero_one(self):
        if len(self.points) >= 3:
            return self.points[2]
        if len(self.points) <= 1:
            return self.origin + Point.vy()
        origin = self.origin
        zero_one = origin + (self.one_zero - origin).ort()
        return zero_one

    def copy(self):
        return Axes(*self.get_points())

    def scale(self, scale):
        '''Scale the object keeping the same origin.'''
        scale = Scale(scale)
        origin = self.origin
        one_zero = origin + (self.one_zero - origin)*scale.x
        zero_one = origin + (self.zero_one - origin)*scale.y
        self.points = [origin, one_zero, zero_one]

    def __repr__(self):
        return ('Axes({}, {}, {})'
                .format(self.origin, self.one_zero, self.zero_one))

    def __mul__(self, scalar):
        axes = self.copy()
        return axes.scale(scalar)

    def get_matrix(self):
        '''Get the matrix for this Axes object.

        The matrix transforms the coordinates in the reference frame associated
        to the Axes object to real coordinates.

        Example:
            Axes((10, 10), (15, 15)).get_matrix().apply((1, 0))
              -> Point(15, 15)
        '''
        origin = self.origin
        v01 = self.zero_one - origin
        v10 = self.one_zero - origin
        return Matrix(((v10.x, v01.x, origin.x),
                       (v10.y, v01.y, origin.y)))

@combination(tuple, Axes)
@combination(Point, Axes)
def point_at_axes(point, axes):
    if len(axes.points) >= 3:
        raise TypeError('Too many arguments to {}'
                        .format(self.__class__.__name__))
    axes.points.append(point)

@combination(int, Axes)
@combination(float, Axes)
@combination(Scale, Axes)
def scalar_at_axes(scale, axes):
    axes.scale(scale)
