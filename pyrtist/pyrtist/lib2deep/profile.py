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

    def check(self):
        '''Raise an exception if the object is not valid.'''
        if not self.is_valid():
            raise ValueError('Profile object is not valid')

    def change_axes(self, axes, zero_offset=False):
        '''Express the profile using different axes.

        If the profile does not have an axis, this method just adds it
        without affecting the points.
        '''
        dy = (Point.vy(self.offset) if zero_offset else Point())
        if self.axes is None:
            ps = [p + dy for p in self.points]
        else:
            mx_self = self.axes.get_matrix().get_inverse()
            ps = [mx_self.apply(p) + dy for p in self.points]
            ps.sort(key=lambda p: p.x)
            mx_new = axes.get_matrix()
            ps = tuple(mx_new.apply(p) for p in ps)
        return (Profile(axes, *ps) if zero_offset
                else Profile(self.offset, axes, *ps))

    def get_function(self):
        '''Get a function for the profile.'''
        self.check()

        mx_self = self.axes.get_matrix().get_inverse()
        ps = [mx_self.apply(p) for p in self.points]
        ps.sort(key=lambda p: p.x)
        min_x = ps[0].x
        max_x = ps[-1].x
        last = [ps[0], ps[1]]

        def fn(x):
            p_lhs, p_rhs = last
            if p_lhs.x <= x <= p_rhs.x:
                pass
            elif min_x <= x <= max_x:
                lhs = 0
                rhs = len(ps) - 1
                while lhs + 1 < rhs:
                    ctr = (lhs + rhs)//2
                    if x < ps[ctr].x:
                        rhs = ctr
                    else:
                        lhs = ctr
                p_lhs, p_rhs = last[0:2] = ps[lhs:lhs+2]
            else:
                return 0.0
            return (p_lhs.y*(p_rhs.x - x) +
                    p_rhs.y*(x - p_lhs.x))/(p_rhs.x - p_lhs.x)

        return fn
