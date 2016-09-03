__all__ = ('Axes',)

from .core_types import Point, Matrix


class Axes(object):
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
        if len(args) > 3:
            raise TypeError('Too many arguments to {}'
                            .format(self.__class__.__name__))
        points = map(Point, args)
        if len(points) == 0:
            points = (Point(), Point.vx(), Point.vy())
        elif len(points) == 1:
            origin = points[0]
            points = (origin, origin + Point.vx(), origin + Point.vy())
        elif len(points) == 2:
            origin, one_zero = points
            zero_one = origin + (one_zero - origin).ort()
            points = (origin, one_zero, zero_one)
        self.origin, self.one_zero, self.zero_one = points

    def copy(self):
        return Axes(self.origin, self.one_zero, self.zero_one)

    def scale(self, scalar):
        '''Scale the object keeping the same origin.'''
        origin = self.origin
        self.one_zero = origin + (self.one_zero - origin)*scalar
        self.zero_one = origin + (self.zero_one - origin)*scalar

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
