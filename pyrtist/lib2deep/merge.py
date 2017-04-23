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

'''Merge object: allows merging surfaces.'''

__all__ = ('Merge',)

from ..lib2d.base import combination
from ..lib2d import Window
from .primitive import Primitive
from .cmd_stream import Cmd


class Merge(Primitive):
    def __init__(self, *args):
        super(Merge, self).__init__()
        self.primitives = []
        self.take(*args)

    def get_window(self, *args):
        w = Window()
        for p in self.primitives:
            w.take(p.get_window())
        return w

    def build_cmds(self):
        cmds = []
        w = ([Cmd(Cmd.image_draw, self.window)]
             if self.window is not None else None)
        for p in self.primitives:
            do_merge = (len(cmds) > 0)
            cmds.extend(p.build_cmds())
            if do_merge:
                cmds.append(Cmd(Cmd.merge))
        return cmds

@combination(Primitive, Merge)
def primitive_at_merge(primitive, merge):
    merge.primitives.append(primitive)
