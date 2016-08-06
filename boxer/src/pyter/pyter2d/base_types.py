'''Fundamental types for the pyrtist 2D graphic library.

This module defines Point, Matrix, and other types commonly used in the
library.
'''

__all__ = ('Scalar', 'Point', 'Px', 'Py', 'PointTaker', 'Matrix', 'Close')

import numbers
import copy

from base import *


class Scalar(float):
    '''Used to identify scalars that need to be transformed (such as line
    widths) in a CmdStream.
    '''


class Point(Taker):
    def __init__(self, value=None, x=0.0, y=0.0):
        self.x = x
        self.y = y
        super(Point, self).__init__()
        if value is not None:
            self.take(value)

    def __repr__(self):
        return 'Point(({x}, {y}))'.format(x=self.x, y=self.y)

    def __add__(self, value):
        return Point(x=self.x + value.x, y=self.y + value.y)

    def __sub__(self, value):
        return Point(x=self.x - value.x, y=self.y - value.y)

    def __mul__(self, value):
        if isinstance(value, numbers.Number):
            return Point(x=self.x*value, y=self.y*value)
        else:
            return float(self.x*value.x + self.y*value.y)

def Px(value):
    return Point(x=value)

def Py(value):
    return Point(y=value)

@combination(Point, Point)
def fn(child, parent):
    parent.x = child.x
    parent.y = child.y

@combination(tuple, Point)
def fn(tp, point):
    if len(tp) != 2:
        raise RejectException()
    x, y = tp
    point.x = float(x)
    point.y = float(y)


class PointTaker(Taker):
    def __init__(self, *args):
        self.points = []
        super(PointTaker, self).__init__(*args)

    def __iter__(self):
        return iter(self.points)

    def __len__(self):
        return len(self.points)

@combination(Point, PointTaker)
def fn(point, point_taker):
    point_taker.points.append(point)

@combination(tuple, PointTaker)
def fn(tp, point_taker):
    if len(tp) != 2:
        raise RejectException()
    point_taker.take(Point(tp))


Close = enum('Close', 'Whether to close a path',
             no=False, yes=True)


class Matrix(object):
    identity = [[1.0, 0.0, 0.0],
                [0.0, 1.0, 0.0]]

    def __init__(self, value=None):
        if value is None:
            value = Matrix.identity
        self.value = copy.deepcopy(value)

    def __repr__(self):
        return 'Matrix({})'.format(repr(self.value))

    def __mul__(self, b):
        if isinstance(b, Point):
            return self.apply(b)

        ret = self.copy()
        if isinstance(b, numbers.Number):
            ret.scale(b)
        else:
            ret.multiply(b)
        return ret

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
        'Return a copy of the matrix.'

        return Matrix(value=self.value)

    def scale(self, s):
        'Scale the matrix by the given factor (in-place).'

        v = self.value
        v[0][0] *= s; v[0][1] *= s; v[0][2] *= s
        v[1][0] *= s; v[1][1] *= s; v[1][2] *= s

    def translate(self, p):
        'Translate the matrix by the given Point value (in-place).'

        self.value[0][2] += p.x
        self.value[1][2] += p.y

    def apply(self, p):
        'Apply the matrix to a Point.'

        (a11, a12, a13), (a21, a22, a23) = self.value
        return Point(x=a11*p.x + a12*p.y + a13,
                     y=a21*p.x + a22*p.y + a23)

    def det(self):
        'Return the determinant of the matrix.'

        m = self.value
        return m[0][0]*m[1][1] - m[0][1]*m[1][0]

    def invert(self):
        'Invert the matrix in place.'

        (m11, m12, m13), (m21, m22, m23) = m = self.value
        det = m11*m22 - m12*m21
        if det == 0.0:
            raise ValueError('The matrix is singular: cannot invert it')

        m[0][0] = new11 =  m22/det; m[0][1] = new12 = -m12/det
        m[1][0] = new21 = -m21/det; m[1][1] = new22 =  m11/det
        m[0][2] = -new11*m13 - new12*m23
        m[1][2] = -new21*m13 - new22*m23

    def get_inverse(self):
        'Return the inverse of the matrix.'

        ret = self.copy()
        ret.invert()
        return ret
