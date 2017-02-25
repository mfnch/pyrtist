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

'''Sculpt object: allows sculpting one object on top of another one.'''

__all__ = ('Sculpt',)

from ..lib2d import Window
from ..lib2d.base import combination
from .primitive import Primitive
from .cmd_stream import Cmd
from .merge import Merge


class Sculpt(Primitive):
    def __init__(self, *args):
        super(Sculpt, self).__init__()
        self.primitives = []
        self.take(*args)

    def get_window(self, *args):
        return Window()

    def build_cmds(self):
        cmds = []
        if len(self.primitives) < 1:
            return [Cmd(Cmd.image_new), Cmd(Cmd.depth_new)]

        operands = [self.primitives[0]]
        if len(self.primitives) == 2:
            operands.append(self.primitives[1])
        elif len(self.primitives) > 2:
            operands.append(Merge(*self.primitives[1:]))

        for operand in operands:
            cmds.extend(operand.build_cmds())

        if len(operands) == 2:
            cmds.append(Cmd(Cmd.sculpt))
        return cmds

@combination(Primitive, Sculpt)
def primitive_at_sculpt(primitive, sculpt):
    sculpt.primitives.append(primitive)
