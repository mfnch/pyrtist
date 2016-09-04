'''Core types for lib2deep.

This module defines 3D Point and other types commonly used in the library.
'''

__all__ = ('Point3',)

import numbers
import math

from ..lib2d import Point


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
        if len(args) == 0:
            self.x = self.y = self.z = 0.0
        else:
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
                raise TypeError('Unknown type of first argument of Point3()')
            if len(xyz) == 3:
                pass
            elif len(xyz) > 3:
                raise TypeError('Too many arguments to Point3()')
            else:
                xyz = (xyz + (0.0, 0.0, 0.0))[:3]
            self.x = float(xyz[0])
            self.y = float(xyz[1])
            self.z = float(xyz[2])

    def __repr__(self):
        return 'Point3({}, {}, {})'.format(self.x, self.y, self.z)

    def __iter__(self):
        return iter((self.x, self.y, self.z))

    def __add__(self, value):
        return Point3(self.x + value.x, self.y + value.y, self.z + value.z)

    def __sub__(self, value):
        return Point3(self.x - value.x, self.y - value.y, self.z - value.z)

    def copy(self):
        'Return a copy of the point.'
        return Point3(self.x, self.y, self.z)

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
