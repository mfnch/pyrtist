__all__ = ('Profile',)

import numbers

from ..lib2d import Point, Matrix, Axes


class Profile(object):
    def __init__(self, *args):
        self.axes = None
        self.points = points = []
        offset = 0.0
        for arg in args:
            if isinstance(arg, Axes):
                if self.axes is not None:
                    raise TypeError('Duplicate Axes() object '
                                    'given to Profile()')
                self.axes = arg
            elif isinstance(arg, numbers.Number):
                offset += arg
            else:
                points.append(Point(arg))
        self.offset = offset

    def is_valid(self):
        '''Whether the object is well formed.'''
        return (self.axes is not None and len(self.points) >= 2)

    def change_axes(self, axes, zero_offset=False):
        '''Express the profile in a different axes.'''
        mx_self = self.axes.get_matrix().get_inverse()
        mx_new = axes.get_matrix()
        dy = (Point.vy(self.offset) if zero_offset else Point())
        ps = [mx_self.apply(p) + dy for p in self.points]
        ps.sort(key=lambda p: p.x)
        ps = tuple(mx_new.apply(p) for p in ps)
        return (Profile(axes, *ps) if zero_offset
                else Profile(self.offset, axes, *ps))
