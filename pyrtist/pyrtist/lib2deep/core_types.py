'''Core types for lib2deep.

This module defines 3D Point and other types commonly used in the library.
'''

__all__ = ('Point3', 'DeepMatrix')

import numbers
import math

from ..lib2d import Point, Matrix


class Z(float):
    'Used to mark Z coordinates in commands.'


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


class DeepMatrix(Matrix):
    '''Extension of lib2d.Matrix which adds translation and scaling along z.'''

    z_identity = [1.0, 0.0]

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

    def scale(self, s):
        '''Scale by the given factor (in-place).'''
        super(DeepMatrix, self).scale(s)
        self.z_value[0] *= s
        self.z_value[1] *= s

    def translate(self, p):
        '''Translate by the given Point3 value (in-place).'''
        p = (p if isinstance(p, Point3) else Point3(p))
        super(DeepMatrix, self).translate(p.get_xy())
        self.z_value += p.z

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
