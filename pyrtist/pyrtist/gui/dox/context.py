# Copyright (C) 2012 Matteo Franchin
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

class Context(object):
  '''A class used to identify contexts in a Box source file.
  The idea is that each item (DoxBlock) in a Box source file is associated a
  context. Contexs can be changed by blocks themselves. For example, a section
  block like ``///Section: Library'' changes to a context with a different
  value of 'section'. The main use of the context is to classify/organize
  blocks into sections.'''

  def __init__(self, values=None):
    self.values = dict(values if values else ())

  def create_context(self, **kwargs):
    '''Create a new context copying it from this one and overwriting it with
    the given ``key=value'' arguments.

    Example: context.create_context(section='newsection')
    '''
    return Context(values=kwargs)

  def get(self, *args, **kwargs):
    '''Get a value from the context. Same interface as ``dict.get''.'''
    return self.values.get(*args, **kwargs)
