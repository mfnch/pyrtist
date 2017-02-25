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

__all__ = ('Primitive', 'InvalidPrimitiveError')

from ..lib2d import Window, Color
from ..lib2d.base import Taker, combination
from .cmd_stream import Cmd, CmdStream
from .deep_window import DeepWindow


class InvalidPrimitiveError(Exception):
    '''Error raised when the primitive is incomplete or ill defined.'''


class Primitive(Taker):
    def __init__(self, *args):
        self.window = None
        self.extra_args = []
        super(Primitive, self).__init__(*args)

    def get_window(self):
        '''Get the Window object associated to this primitive or build a
        default profile if none was provided by the user.
        '''
        if self.window is not None:
            return self.window
        w = Window()
        w.take(self.get_profile(*self.extra_args))
        return w

    def build_depth_cmd(self):
        '''Return a list of commands that can be used to construct the depth
        buffer for this primitive.
        '''
        return []

    def build_image_cmds(self):
        '''Return a list of commands which can be used to construct the image
        buffer for this primitive.
        '''
        return [Cmd(Cmd.image_draw, self.get_window())]

    def build_cmds(self):
        '''Return a list of commands which can be used to construct the image
        and depth buffers for this primitive.
        '''
        cmds = [Cmd(Cmd.image_new)]
        cmds.extend(self.build_image_cmds())
        cmds.append(Cmd(Cmd.depth_new))
        cmds.extend(self.build_depth_cmd())
        return cmds

    def get_profile(self, *extra_args):
        raise NotImplementedError('Primitive profile not implemented')

@combination(Window, Primitive)
def window_at_primitive(window, primitive):
    if primitive.window is not None:
        raise TypeError('{} takes only one Window'
                        .format(window.__class__.__name__))
    primitive.window = window

@combination(Color, Primitive)
def color_at_primitive(extra_arg, primitive):
    primitive.extra_args.append(extra_arg)

@combination(Primitive, DeepWindow)
def primitive_at_deep_window(primitive, deep_window):
    deep_window.take(CmdStream(primitive))

@combination(Primitive, CmdStream)
def primitive_at_cmd_stream(primitive, cmd_stream):
    cmds = primitive.build_cmds()
    cmds.append(Cmd(Cmd.transfer))
    cmd_stream.take(*cmds)
