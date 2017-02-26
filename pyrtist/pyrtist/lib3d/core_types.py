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

'''3D types for Pyrtist.
'''

__all__ = ('Point3', 'Matrix3')

import math
import numbers
import numpy as np

from ..lib2d import Point, GenericMatrix


class Point3(object):
    '''Point with 3 components.

    Point3 can be initialised in one of the following ways:
    - Point3(), Point(x), Point(x, y), Point(x, y, z). Use the given arguments
      to set x, y and z components. Missing components are set to zero.
    - Point3(Point(x, y)) or Point3(Point(x, y), z) to set x, y components from
      a 2D point.
    - Point3(Point3(...)) to create a copy of the first argument.
    - Point3(tuple, ...) to set the first components from the tuple and the
      others from the remaining arguments.
    '''

    def __init__(self, *args):
        self.x = 0.0
        self.y = 0.0
        self.z = 0.0
        self.set(*args)

    def set(self, *args):
        if len(args) < 1:
            return
        arg0 = args[0]
        if isinstance(arg0, numbers.Number):
            xyz = args
        elif isinstance(arg0, Point):
            xyz = (arg0.x, arg0.y) + args[1:]
        elif isinstance(arg0, Point3):
            xyz = (arg0.x, arg0.y, arg0.z) + args[1:]
        elif isinstance(arg0, tuple):
            xyz = arg0 + args[1:]
        else:
            raise TypeError('Invalid type for setting Point3')
        if len(xyz) > 3:
            raise TypeError('Too many arguments for setting Point3')
        for i, value in enumerate(xyz):
            setattr(self, 'xyz'[i], value)

    def __repr__(self):
        return 'Point3({}, {}, {})'.format(self.x, self.y, self.z)

    def __iter__(self):
        return iter((self.x, self.y, self.z))

    def __neg__(self):
        return type(self)(*tuple(-x for x in self))

    def __pos__(self):
        return self.copy()

    def __add__(self, value):
        return Point3(self.x + value.x, self.y + value.y, self.z + value.z)

    def __sub__(self, value):
        return Point3(self.x - value.x, self.y - value.y, self.z - value.z)

    def __mul__(self, value):
        if isinstance(value, numbers.Number):
            # Scalar by vector.
            return Point3(self.x*value, self.y*value, self.z*value)
        else:
            # Vector product.
            return self.dot(value)

    def __rmul__(self, value):
        return self.__mul__(value)

    def __div__(self, value):
        return self.__class__(self.x/value, self.y/value, self.z/value)

    def copy(self):
        'Return a copy of the point.'
        return Point3(self.x, self.y, self.z)

    @property
    def xy(self):
        return self.get_xy()

    def get_xy(self):
        'Return the x and y components as a Point object.'
        return Point(self.x, self.y)

    def dot(self, p):
        'Return the scalar product with another 3D point.'
        return self.x*p.x + self.y*p.y + self.z*p.z

    def cross(self, p):
        'Return the cross product with another 3D point.'
        return self.__class__(self.y*p.z - self.z*p.y,
                              self.z*p.x - self.x*p.z,
                              self.x*p.y - self.y*p.x)

    def norm2(self):
        'Return the square of the norm of the point.'
        return self.x*self.x + self.y*self.y + self.z*self.z

    def norm(self):
        'Return the norm of the point.'
        return math.sqrt(self.norm2())

    def normalize(self):
        'Normalize the point.'
        n = self.norm()
        if n != 0.0:
            self.x /= n
            self.y /= n
            self.z /= n

    def normalized(self):
        'Return a normalized copy of this point.'
        p = self.copy()
        p.normalize()
        return p

    def vec_prod(self, p):
        '''Vector product.'''
        # Vector product.
        ax, ay, az = self
        bx, by, bz = p
        return Point3(ay*bz - az*by, az*bx - ax*bz, ax*by - ay*bx)


class Matrix3(GenericMatrix):
    size = (3, 4)

    identity = [[1.0, 0.0, 0.0, 0.0],
                [0.0, 1.0, 0.0, 0.0],
                [0.0, 0.0, 1.0, 0.0]]

    @classmethod
    def translation(cls, t):
        '''Return a new translation matrix for the given translation vector.'''
        t = Point3(t)
        return cls([[1.0, 0.0, 0.0, t.x],
                    [0.0, 1.0, 0.0, t.y],
                    [0.0, 0.0, 1.0, t.z]])

    @classmethod
    def rotation_euler(cls, phi, theta, psi):
        '''Return a rotation from the corresponding Euler angles.'''
        cphi, sphi = (math.cos(phi), math.sin(phi))
        cth, sth = (math.cos(theta), math.sin(theta))
        cpsi, spsi = (math.cos(psi), math.sin(psi))
        return cls(
          [[cpsi*cphi - cth*sphi*spsi, cpsi*sphi + cth*cphi*spsi, spsi*sth, 0.0],
           [-spsi*cphi - cth*sphi*cpsi, cth*cphi*cpsi - spsi*sphi, cpsi*sth, 0.0],
           [sth*sphi, -sth*cphi, cth, 0.0]]
        )

    @classmethod
    def rotation(cls, angle, axis='x'):
        '''Construct a rotation around a given axis. If the rotation axis is
        one of the principal axes (x, y or z), then axis can be specified as
        a string 'x', 'y' or 'z'. A Point3 can be used for any other arbitrary
        rotation axis. If axis is not given, then 'x' is assumed. angle is the
        angle of rotation in radians.
        '''

        # Check for rotations around the x, y, z axes.
        idx_rot = {'x': 0, 'y': 1, 'z': 2}.get(axis)
        if idx_rot is not None:
            zero, one, two = [(i + idx_rot) % 3 for i in range(3)]
            c, s = (math.cos(angle), math.sin(angle))
            value = [[0.0]*4 for _ in range(3)]
            value[zero][zero] = 1.0
            value[one][one] = value[two][two] = c
            value[one][two] = -s
            value[two][one] = s
            return cls(value)

        # The axis is given as a 3D vector.
        axis = Point3(axis)
        axis_norm = axis.norm()
        if axis_norm <= 0.0:
            return cls(cls.identity)

        # Rotation around an arbitrary axis.
        ux, uy, uz = axis/axis_norm
        c = math.cos(angle)
        s = math.sin(angle)
        omc = 1.0 - c
        value = [[ux*ux*omc +    c, ux*uy*omc - uz*s, ux*uz*omc + uy*s, 0.0],
                 [uy*ux*omc + uz*s, uy*uy*omc +    c, uy*uz*omc - ux*s, 0.0],
                 [uz*ux*omc - uy*s, uz*uy*omc + ux*s, uz*uz*omc +    c, 0.0]]
        return cls(value)

    @classmethod
    def rotation_by_example(cls, v_in, v_out):
        '''Return the rotation which maps the Point3 object `v_in` to the
        Point3 object `v_out`.
        '''
        norms = v_in.norm()*v_out.norm()
        if norms > 0.0:
            axis = v_in.cross(v_out)
            sin_angle = axis.norm()/norms
            cos_angle = v_in.dot(v_out)/norms
            angle = math.atan2(sin_angle, cos_angle)
            return cls.rotation(angle, axis=axis)
        return cls(Matrix3.identity)

    def __init__(self, value=None):
        super(Matrix3, self).__init__()
        self.set(value)

    def set(self, value):
        '''Set the matrix to the given value.'''
        if value is None:
            value = self.identity
        elif isinstance(value, self.__class__):
            value = value.value
        self.value = [list(value[0]), list(value[1]), list(value[2])]

    def __repr__(self):
        return '{}({})'.format(self.__class__.__name__, repr(self.value))

    def __str__(self):
        return self.to_str()

    def to_str(self, fmt='8.3f', sep=' \t'):
        fmt = '{:' + fmt + '}'
        s = '\n'.join('\t' + sep.join(fmt.format(arg) for arg in args)
                      for args in self.value)
        return 'Matrix3(\n{}\n)'.format(s)

    def __mul__(self, b):
        if isinstance(b, Point3):
            return self.apply(b)
        if isinstance(b, tuple) and len(b) == 3:
            return self.apply(Point(b))

        if isinstance(b, numbers.Number):
            ret = self.copy()
            ret.scale(b)
        else:
            ret = self.get_product(b)
        return ret

    def __rmul__(self, b):
        if isinstance(b, numbers.Number):
            return self.__mul__(b)
        raise NotImplementedError()

    def to_np33(self, dtype=None):
        '''Convert to a 3x3 NumPy matrix, discarding the translation part.'''
        return self.to_np34(dtype=dtype)[:, :3]

    def to_np34(self, dtype=None):
        '''Convert to a 3x4 NumPy matrix.'''
        dtype = (dtype or np.float64)
        return np.array(self.value, dtype=dtype)

    def to_np44(self, dtype=None):
        '''Convert to a 4x4 NumPy matrix.'''
        dtype = (dtype or np.float64)
        mx = np.zeros((4, 4), dtype=dtype)
        mx[:3, :] = np.array(self.value, dtype=dtype)
        mx[3, 3] = 1.0
        return mx

    @classmethod
    def from_np(cls, np_array):
        '''Create a new Matrix3 from a 3x4 or 4x4 NumPy matrix.'''
        return cls(np_array[:3] if len(np_array) != 3 else np_array)

    def get_axis_and_angle(self):
        '''Return the axis and angle of rotation for the rotational part of
        this Matrix3 object. For the identity matrix, this function returns
        None for the axis and 0.0 for the angle.
        '''
        mx = self.to_np33()
        axis = np.array([mx[2, 1] - mx[1, 2],
                         mx[0, 2] - mx[2, 0],
                         mx[1, 0] - mx[0, 1]])
        axis_norm = np.linalg.norm(axis)
        if axis_norm == 0.0:
            return (None, 0.0)
        sin_angle = 0.5*axis_norm
        cos_angle = 0.5*(sum(mx[i, i] for i in range(3)) - 1.0)
        return (axis/axis_norm, math.atan2(sin_angle, cos_angle))

    def get_product(self, b):
        '''Return the result of right-multiplying this matrix by another one.

        This is similar to the multiply() method, except that the result is
        returned as a different matrix and the original matrix is left
        untouched.
        '''
        b = b.value
        a = self.value
        m, n = self.size
        assert min(m, n) == m
        value = [[sum(a[i][k]*b[k][j] for k in range(m))
                  for j in range(n)]
                 for i in range(m)]
        j = n - 1
        for i in range(m):
            value[i][j] += a[i][j]
        return self.__class__(value)

    def multiply(self, b):
        '''Right multiply this matrix with another matrix.'''
        print('multiply')
        self.value = self.get_product(b).value

    def scale(self, s):
        '''Scale the matrix by the given factor (in-place).'''
        m, n = self.size
        for i in range(m):
            for j in range(n):
                self.value[i][j] *= s

    def translate(self, p):
        '''Translate the matrix by the given Point value (in-place).'''
        for i, x in enumerate(Point3(p)):
            self.value[i][-1] += x

    def copy(self):
        '''Return a copy of the matrix.'''
        return self.__class__(value=self.value)

    def apply(self, p):
        '''Apply the matrix to a Point3.'''
        mx = self.value
        out = tuple(sum(mx[i][j]*x for j, x in enumerate(p))
                    for i in range(3))
        return Point3(out)
