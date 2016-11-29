'''3D types for Pyrtist.
'''

__all__ = ('Point3', 'Matrix3')

import numbers
from ..lib2d import Point


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


class Matrix3(object):
    size = (3, 4)

    identity = [[1.0, 0.0, 0.0, 0.0],
                [0.0, 1.0, 0.0, 0.0],
                [0.0, 0.0, 1.0, 0.0]]

    @classmethod
    def rotation_euler(cls, phi, theta, psi):
        '''Return a rotation from the corresponding Euler angles.'''
        cphi, sphi = (math.cos(phi), math.sin(phi))
        cth, sth = (math.cos(theta), math.sin(theta))
        cpsi, spsi = (math.cos(psi), math.sin(psi))
        return cls(
          [[cpsi*cphi - cth*sphi*spsi, cpsi*sphi + cth*cphi*spsi, spsi*sth],
           [-spsi*cphi - cth*sphi*cpsi, cth*cphi*cpsi - spsi*sphi, cpsi*sth],
           [sth*sphi, -sth*cphi, cth]]
        )

    def __init__(self, value=None):
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

    def __mul__(self, b):
        if isinstance(b, Point3):
            return self.apply(b)
        if isinstance(b, tuple) and len(b) == 3:
            return self.apply(Point(b))

        ret = self.copy()
        if isinstance(b, numbers.Number):
            ret.scale(b)
        else:
            ret.multiply(b)
        return ret

    def __rmul__(self, b):
        if isinstance(b, numbers.Number):
            return self.__mul__(b)
        raise NotImplementedError()

    def multiply(self, b):
        '''Multiply this matrix with another one.'''
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
        self.value = value

    def scale(self, s):
        '''Scale the matrix by the given factor (in-place).'''
        m, n = self.size
        for i in range(m):
            for j in range(n):
                self.value[i][j] *= s

    def translate(self, p):
        '''Translate the matrix by the given Point value (in-place).'''
        for i, x in enumerate(p):
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
