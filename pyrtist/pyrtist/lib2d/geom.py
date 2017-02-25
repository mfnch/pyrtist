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

from .core_types import Point

def intersection(segment1, segment2):
    '''Find the intersection point between the two segment identified by the
    two pairs of points segment1 and segment2.
    '''
    a1 = segment1[0]
    b1 = segment2[0]
    a12 = segment1[1] - a1
    b12 = segment2[1] - b1
    ob12 = Point(b12.y, -b12.x)
    denom = ob12.dot(a12)
    if denom == 0.0:
        raise ValueError("Intersection not found: segments are parallel")
    return a1 + a12 * (ob12.dot(b1 - a1)/denom)
