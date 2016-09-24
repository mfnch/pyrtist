'''Fundamental types for the pyrtist 2D graphic library.

This module defines Point, Matrix, and other types commonly used in the
library.
'''

__all__ = ('Scalar', 'Point', 'Px', 'Py', 'Pang', 'PointTaker',
           'Matrix', 'Close', 'Container', 'Radii', 'Tri', 'View')

import math
import numbers
import copy

from base import *


class Scalar(float):
    '''Used to identify scalars that need to be transformed (such as line
    widths) in a CmdStream.
    '''


class Point(object):
    '''Point with 2 components.

    A Point() can be created in one of the following ways:
    - Point(), Point(x) or Point(x, y). Use the provided argument to set the
      x and y components. Missing components are set to zero.
    - Point(Point(...)) to copy the point given as first argument.
    - Point(tuple) to set from a tuple.
    '''

    @staticmethod
    def vx(delta_x=1.0):
        '''Return a vector with the given x component and zero y component.'''
        return Point(delta_x)

    @staticmethod
    def vy(delta_y=1.0):
        '''Return a vector with the given y component and zero x component.'''
        return Point(0.0, delta_y)

    @staticmethod
    def vangle(angle):
        '''Return a unit vector forming the specified angle with the x axis.'''
        return Point(math.cos(angle), math.sin(angle))

    @staticmethod
    def sum(points, default=None):
        return sum(points, default or Point())

    def __init__(self, *args, **kwargs):
        if len(args) == 0:
            self.x = self.y = 0.0
        else:
            arg0 = args[0]
            if isinstance(arg0, numbers.Number):
                xy = args
            elif isinstance(arg0, Point):
                xy = (arg0.x, arg0.y) + args[1:]
            elif isinstance(arg0, tuple):
                xy = arg0 + args[1:]
            else:
                raise TypeError('Unknown type of first argument of Point()')
            if len(xy) == 2:
                self.x = float(xy[0])
                self.y = float(xy[1])
            elif len(xy) > 2:
                raise TypeError('Too many arguments to Point()')
            else:
                assert len(args) == 1
                self.x = xy[0]
                self.y = 0.0
        # The code below is there for compatibility reasons, but we should get
        # rid of it eventually.
        if 'x' in kwargs:
            self.x = kwargs['x']
        if 'y' in kwargs:
            self.y = kwargs['y']

    def __repr__(self):
        return 'Point({x}, {y})'.format(x=self.x, y=self.y)

    def __iter__(self):
        return iter((self.x, self.y))

    def __add__(self, value):
        return Point(x=self.x + value.x, y=self.y + value.y)

    def __sub__(self, value):
        return Point(x=self.x - value.x, y=self.y - value.y)

    def __mul__(self, value):
        if isinstance(value, numbers.Number):
            return Point(x=self.x*value, y=self.y*value)
        else:
            return float(self.x*value.x + self.y*value.y)

    def __rmul__(self, value):
        return self.__mul__(value)

    def __div__(self, value):
        return Point(x=self.x/value, y=self.y/value)

    def copy(self):
        return Point(x=self.x, y=self.y)

    def dot(self, p):
        '''Return the scalar product with p.'''
        return self.x*p.x + self.y*p.y

    def norm2(self):
        return self.x*self.x + self.y*self.y

    def norm(self):
        return math.sqrt(self.norm2())

    def normalize(self):
        '''Normalized this vector.'''
        n = self.norm()
        if n != 0.0:
            self.x /= n
            self.y /= n

    def normalized(self):
        '''Return a normalized copy of this vector.'''
        p = self.copy()
        p.normalize()
        return p

    def ort(self):
        '''Return the ortogonal vector, rotated by 90 degrees anticlockwise.'''
        return Point(-self.y, self.x)

    def mirror(self, axes):
        '''Mirror the point with respect to the x axis of the given Axes()
        object.
        '''
        d = self - axes.origin
        u10 = (axes.one_zero - axes.origin).normalized()
        d_ort = u10*u10.dot(d)
        return axes.origin - d + d_ort*2.0

    def mirror_x(self, p):
        '''Mirror the point with respect to the x axis.'''
        return Point(self.x, 2.0*p.y - self.y)

    def mirror_y(self, p):
        '''Mirror the point with respect to the x axis.'''
        return Point(2.0*p.x - self.x, self.y)


def Px(value):
    return Point.vx(value)

def Py(value):
    return Point.vy(value)

def Pang(angle):
    'Return a Point of unit norm forming the specified angle with the x axis.'
    return Point(x=math.cos(angle), y=math.sin(angle))


class PointTaker(Taker):
    def __init__(self, *args):
        self.points = []
        super(PointTaker, self).__init__(*args)

    def __iter__(self):
        return iter(self.points)

    def __len__(self):
        return len(self.points)

    def __getitem__(self, index):
        return self.points[index]

@combination(Point, PointTaker)
def fn(point, point_taker):
    point_taker.points.append(point)

@combination(tuple, PointTaker)
def fn(tp, point_taker):
    if len(tp) != 2:
        raise RejectException()
    point_taker.take(Point(tp))


class Matrix(object):
    identity = [[1.0, 0.0, 0.0],
                [0.0, 1.0, 0.0]]

    def __init__(self, value=None):
        if value is None:
            value = Matrix.identity
        elif isinstance(value, Matrix):
            value = value.value
        self.value = [list(value[0]), list(value[1])]

    def __repr__(self):
        return 'Matrix({})'.format(repr(self.value))

    def __mul__(self, b):
        if isinstance(b, Point):
            return self.apply(b)
        if isinstance(b, tuple) and len(b) == 2:
            return self.apply(Point(b))

        ret = self.copy()
        if isinstance(b, numbers.Number):
            ret.scale(b)
        else:
            ret.multiply(b)
        return ret

    def get_entries(self):
        '''Get the matrix entries as a tuple of 6 scalars.'''
        return tuple(self.value[0] + self.value[1])

    def multiply(self, b):
        (a11, a12, a13), (a21, a22, a23) = ab = self.value
        (b11, b12, b13), (b21, b22, b23) = b.value
        ab[0][0] = a11*b11 + a12*b21; ab[0][1] = a11*b12 + a12*b22
        ab[1][0] = a21*b11 + a22*b21; ab[1][1] = a21*b12 + a22*b22
        ab[0][2] = a13 + a11*b13 + a12*b23
        ab[1][2] = a23 + a21*b13 + a22*b23

    def __rmul__(self, b):
        if isinstance(b, numbers.Number):
            return self.__mul__(b)
        raise NotImplementedError()

    def copy(self):
        '''Return a copy of the matrix.'''
        return Matrix(value=self.value)

    def scale(self, s):
        '''Scale the matrix by the given factor (in-place).'''
        v = self.value
        v[0][0] *= s; v[0][1] *= s; v[0][2] *= s
        v[1][0] *= s; v[1][1] *= s; v[1][2] *= s

    def translate(self, p):
        '''Translate the matrix by the given Point value (in-place).'''
        self.value[0][2] += p.x
        self.value[1][2] += p.y

    def apply(self, p):
        '''Apply the matrix to a Point.'''
        if not isinstance(p, Point):
            p = Point(p)
        (a11, a12, a13), (a21, a22, a23) = self.value
        return Point(a11*p.x + a12*p.y + a13,
                     a21*p.x + a22*p.y + a23)

    def det(self):
        '''Return the determinant of the matrix.'''
        m = self.value
        return m[0][0]*m[1][1] - m[0][1]*m[1][0]

    def invert(self):
        '''Invert the matrix in place.'''
        (m11, m12, m13), (m21, m22, m23) = m = self.value
        det = m11*m22 - m12*m21
        if det == 0.0:
            raise ValueError('The matrix is singular: cannot invert it')

        m[0][0] = new11 =  m22/det; m[0][1] = new12 = -m12/det
        m[1][0] = new21 = -m21/det; m[1][1] = new22 =  m11/det
        m[0][2] = -new11*m13 - new12*m23
        m[1][2] = -new21*m13 - new22*m23

    def get_inverse(self):
        '''Return the inverse of the matrix.'''
        ret = self.copy()
        ret.invert()
        return ret


Close = enum('Close', 'Whether to close a path',
             no=False, yes=True)


class Container(object):
    def __init__(self, *args):
        self.args = args

    def __repr__(self):
        return '{name}({args})'.format(name=self.__class__.__name__,
                                       args=', '.join(map(repr, self.args)))

    def __iter__(self):
        return iter(self.args)

    def copy(self):
        return self.__class__(*self.args)

    def check(self, min_args, max_args):
        if len(self.args) < min_args:
            raise TypeError('{} object needs at least {} arguments'
                            .format(self.__class__.__name__, min_args))
        if len(self.args) > max_args:
            raise TypeError('{} object takes at most {} arguments'
                            .format(self.__class__.__name__, max_args))


class Radii(Container):
    '''Container which groups one or more radii (e.g. the x, y radii of
    an ellipsoid.
    '''

class Tri(Container):
    '''Container which groups up to 3 points used to define a Bezier curve.'''

    def __init__(self, *args):
        n = len(args)
        if n == 1:
            self.args = (None, args[0], None)
        elif n == 2:
            self.args = (args[0], args[1], None)
        elif n == 3:
            self.args = args
        else:
            raise TypeError('Tri takes at most 3 points')

    @property
    def p(self):
        return self.args[1]

    @property
    def ip(self):
        p_in = self.args[0]
        if p_in is not None:
            return p_in
        p_out = self.args[2]
        if p_out is not None:
            return 2.0*self.args[1] - p_out
        return self.args[1]

    @property
    def op(self):
        p_out = self.args[2]
        if p_out is not None:
            return p_out
        p_in = self.args[0]
        if p_in is not None:
            return 2.0*self.args[1] - p_in
        return self.args[1]


class View(object):
    '''Object used to pass information to the GUI.'''

    def __init__(self, bbox, origin, size):
        self.bbox = bbox
        self.origin = origin
        self.size = size

    def __repr__(self):
        b1 = self.bbox.min_point
        b2 = self.bbox.max_point
        bbox_args = ', '.join(map(str, (2, b1.x, b1.y, b2.x, b2.y)))
        other_args = ', '.join(map(str, (self.origin.x, self.origin.y,
                                         self.size.x, self.size.y)))
        return '{}\n{}\n'.format(bbox_args, other_args)
