# Copyright (C) 2021 Matteo Franchin
#
# This file is part of Pyrtist.
#   Pyrtist is free software: you can redistribute it and/or modify it
#   under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, either version 2.1 of the License, or
#   (at your option) any later version.
#
#   Pyrtist is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

'''
Module that allows using OpenGL to draw in Pyrtist (experimental)

This module exposes the FragmentWindow window that allows using a fragment
shader to draw graphics. Use as follows:

  from pyrtist.opengl import *

  p1 = (0, 0)
  p2 = (1, 1)
  w = FragmentWindow(p1, p2)

  w << """
  #version 150 core
  in vec3 frag_pos;
  out vec4 out_color;
  void main() {
    out_color = vec4(frag_pos.x, frag_pos.y, 0.0, 1.0);
  }
  """

  w.draw(gui)

p1 and p2 are the points in the bounding box. frag_pos will be the position
coordinates relative to these points.
'''

from .fragment_window import FragmentWindow
from ..lib2d import gui
