# Copyright (C) 2013 by Matteo Franchin (fnch@users.sf.net)
#
# This file is part of Pyrtist.
#
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

"""
Renderer routines to draw the refpoints.
"""

from geom2 import Point


def cut_point(size, x, y):
  return (max(0, min(size[0]-1, x)), max(0, min(size[1]-1, y)))

def cut_square(size, x, y, dx, dy):
  if size == None:
    return (int(x), int(y), dx + 1, dy + 1)
  else:
    x = int(x)
    y = int(y)
    x0, y0 = cut_point(size, x, y)
    x1, y1 = cut_point(size, x+dx, y+dy)
    sx, sy = (x1 - x0, y1 - y0)
    if sx < 1 or sy < 1: return (x0, y0, -1, -1)
    return (x0, y0, sx+1, sy+1)

def draw_square(drawable, x, y, size, gc, kind=0):
  l0 = size
  dl0 = l0*2
  gc.foreground, gc.background = (gc.background, gc.foreground)
  drawable.draw_rectangle(gc, True, int(x - l0), int(y - l0), dl0, dl0)
  gc.foreground, gc.background = (gc.background, gc.foreground)

  l1 = l0 - 1
  dl1 = 2*l1
  drawable.draw_rectangle(gc, True, int(x - l1), int(y - l1), dl1, dl1)

def draw_circle(drawable, x, y, size, gc, kind=0):
  l0 = size
  dl0 = l0*2 + 1
  gc.foreground, gc.background = (gc.background, gc.foreground)
  drawable.draw_arc(gc, True, int(x - l0), int(y - l0), dl0, dl0, 64*0, 64*360)
  gc.foreground, gc.background = (gc.background, gc.foreground)

  l1 = l0 - 1
  dl1 = 2*l1 + 1
  drawable.draw_arc(gc, True, int(x - l1), int(y - l1), dl1, dl1, 64*0, 64*360)
