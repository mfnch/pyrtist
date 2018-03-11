# Copyright (C) 2018 Matteo Franchin
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

__all__ = ('Drawable2',)

from .core_types import Taker, combination
from .window import Window


class Drawable2(Taker):
  defaults = {}

  def __init__(self, **kwargs):
    super(Drawable2, self).__init__()
    for name, value in self.defaults.items():
      setattr(self, name, kwargs.pop(name, value))
    if len(kwargs):
      raise ValueError('Invalid keyword argument(s): {}.'
                       .format(', '.join(kwargs)))

  def draw(self, target):
    '''Override this to implement drawing.'''
    raise ValueError('Object cannot be drawn')


@combination(Drawable2, Window)
def drawable2_at_window(drawable2, window):
  drawable2.draw(window)
