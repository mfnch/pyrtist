# Copyright (C) 2010 by Matteo Franchin (fnch@users.sourceforge.net)
#
# This file is part of Boxer.
#
#   Boxer is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Boxer is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Boxer.  If not, see <http://www.gnu.org/licenses/>.

# Here we provide a Python interface to call Box and get back a view
# (an image) of the picture.

class PixSize:
  def __init__(self, value):
    x, y = value
    self.x = x
    self.y = y
    self.xy = value

class BoxWindow:
  def __init__(self, screen_pix_size, buf_pix_size=None):
    self.screen_pix_size = PixSize(screen_pix_size)

    if bux_pix_size != None:
      buf_pix_size = PixSize(buf_pix_size)
    else:
      f = 1.5
      x = int(self.screen_pix_size.x*f)
      y = int(self.screen_pix_size.x*f)
      buf_pix_size = PixSize((x, y))

    self.buf_pix_size = buf_pix_size

  def draw(self, script, view_size=None):
    pass

