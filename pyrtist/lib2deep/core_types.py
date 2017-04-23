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

'''Core types for lib2deep.

This module defines 3D Point and other types commonly used in the library.
'''

__all__ = ('Point3', 'Matrix3', 'DeepMatrix')

import numbers
import math

from ..lib2d import Point, Matrix
from ..lib3d import Point3, Matrix3


class Z(float):
    'Used to mark Z coordinates in commands.'


class DeepMatrix(Matrix):
    '''Extension of lib2d.Matrix which adds translation and scaling along z.'''

    z_identity = [1.0, 0.0]

    @classmethod
    def diag(cls, x, y, z):
        return cls([[x, 0.0, 0.0], [0.0, y, 0.0]], [z, 0.0])

    @classmethod
    def translation(cls, t):
        return cls([[1.0, 0.0, t.x],
                    [0.0, 1.0, t.y]], [1.0, t.z])

    def __init__(self, xy_value=None, z_value=None):
        super(DeepMatrix, self).__init__()
        self.set(xy_value, z_value)

    def set(self, xy_value=None, z_value=None):
        if isinstance(xy_value, DeepMatrix):
            if z_value is None:
                z_value = xy_value.z_value
            xy_value = xy_value.value
        elif isinstance(xy_value, Matrix):
            if z_value is None:
                z_value = (abs(xy_value.det())**0.5, 0.0)
        super(DeepMatrix, self).set(xy_value)
        self.z_value = list(z_value or DeepMatrix.z_identity)

    def __repr__(self):
        return 'DeepMatrix({}, {})'.format(repr(self.value), repr(self.z_value))

    def __mul__(self, b):
        if isinstance(b, Point3):
            return self.apply(b)
        if isinstance(b, tuple) and len(b) == 3:
            return self.apply(Point3(b))
        return super(DeepMatrix, self).__mul__(b)

    def multiply(self, b):
        if isinstance(b, DeepMatrix):
            a31, a32 = ab = self.z_value
            b31, b32 = b.z_value
            ab[0] = a31*b31
            ab[1] = a31*b32 + a32
        super(DeepMatrix, self).multiply(b)

    def copy(self):
        '''Return a copy of the matrix.'''
        return DeepMatrix(self.value, self.z_value)

    def get_xy(self):
        '''Return the components of the DeepMatrix along the x and y
        directions.
        '''
        return Matrix(self.value)

    def get_matrix3(self):
        '''Return an equivalent Matrix3 object.'''
        v0, v1 = self.value
        z = self.z_value
        return Matrix3([[v0[0], v0[1],  0.0, v0[2]],
                        [v1[0], v1[1],  0.0, v1[2]],
                        [  0.0,   0.0, z[0],  z[1]]])

    def scale(self, s):
        '''Scale by the given factor (in-place).'''
        super(DeepMatrix, self).scale(s)
        self.z_value[0] *= s
        self.z_value[1] *= s

    def translate(self, p):
        '''Translate by the given Point3 value (in-place).'''
        p = (p if isinstance(p, Point3) else Point3(p))
        super(DeepMatrix, self).translate(p.get_xy())
        self.z_value[1] += p.z

    def apply(self, p):
        '''Apply the DeepMatrix to a Point3.'''
        p = (p if isinstance(p, Point3) else Point3(p))
        ret_xy = super(DeepMatrix, self).apply(p.get_xy())
        ret_z = self.apply_z(p.z)
        return Point3(ret_xy, ret_z)

    def apply_z(self, z):
        '''Apply the DeepMatrix to a Z coordinate.'''
        a31, a32 = self.z_value
        return a31*z + a32

    def invert(self):
        '''Invert the DeepMatrix.'''
        if self.z_value[0] == 0.0:
            raise ValueError('DeepMatrix is singular in z: cannot invert it')
        z_inv = 1.0/self.z_value[0]
        z_value = [z_inv, z_inv*self.z_value[1]]
        super(DeepMatrix, self).invert()
        self.z_value = z_value

    def det3(self):
        return super(DeepMatrix, self).det()*self.z_value[0]
