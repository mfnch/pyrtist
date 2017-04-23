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

'''Definition of Pattern, the paint to use for draw and fill operations.'''

__all__ = ('Pattern', 'Extend')

from .base import combination
from .core_types import create_enum
from .cmd_stream import Cmd, CmdStream


class Pattern(object):
    '''The source to use when filling a shape or drawing a line.  `Color` and
    `Gradient` are derived classes.  This means that whenever a Pattern can be
    provided, a `Color` (or `Gradient`) can also be provided.
    '''

    def _get_cmds(self):
        # This method should be overloaded by the inheriting classes and should
        # return a list of commands necessary to set up the specific pattern.
        return []


@combination(Pattern, CmdStream)
def pattern_at_cmd_stream(pattern, cmd_stream):
    cmd_stream.take(*pattern._get_cmds())


doc = '''Extend mode to be used for drawing outside the pattern area.

Available options are:
- Extend.none: pixels outside of the source pattern are fully transparent,
- Extend.repeat: the pattern is tiled by repeating,
- Extend.reflect: the pattern is tiled by reflecting at the edges,
- Extend.pad: pixels outside of the pattern copy the closest pixel from the
  source.
'''
Extend = create_enum('Extend', doc,
                     'unset', 'none', 'repeat', 'reflect', 'pad')

@combination(Extend, CmdStream)
def extend_at_cmd_stream(extend, cmd_stream):
    if extend is not Extend.unset:
        cmd_stream.take(Cmd(Cmd.pattern_set_extend, extend.name))
