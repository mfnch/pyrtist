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

__all__ = ('normal', 'triangle', 'line', 'ruler')

from .. import *

normal = Window()
normal.take(
  Poly(Point(0, -1), Point(0.5, 0), Point(0, 1), Point(2, 0)),
  Hot('head', Point(2, 0)), Hot('tail', Point(1.8, 0)), Hot('join', Point(0.5, 0))
)

triangle = Window()
triangle.take(
  Poly(Point(0, -1), Point(0, 1), Point(2, 0)),
  Hot('head', Point(2, 0)), Hot('tail', Point(1.8, 0)), Hot('join', Point(0, 0))
)

line = Window()
line.take(
  Line(0.2, Point(0, -1), Point(2, 0), Point(0, 1)),
  Hot('head', Point(2, 0)), Hot('tail', Point(1.8, 0)), Hot('join', Point(1.8, 0))
)

ruler = Window()
ruler.take(
  Poly(Point(0, -1), Point(0, 1), Point(2, 0)),
  Line(0.2, Point(2, -1), Point(2, 1)),
  Hot('head', Point(2, 0)), Hot('tail', Point(1.8, 0)), Hot('join', Point(0, 0))
)
