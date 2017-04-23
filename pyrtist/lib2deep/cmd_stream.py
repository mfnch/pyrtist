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

from ..lib2d.base import combination
from ..lib2d import cmd_stream as twod
from ..lib2d import Window, Scalar

from .core_types import Point3, Matrix3, Z, DeepMatrix


class Cmd(twod.CmdBase):
    # Drawing commands.
    draw_cmd_names = ('on_step', 'on_plane', 'on_sphere', 'on_cylinder',
                      'on_circular', 'on_crescent')
    # All commands.
    names = draw_cmd_names + \
      ('set_bbox', 'depth_new', 'merge', 'sculpt',
       'image_new', 'image_draw', 'draw_mesh', 'transfer')
Cmd.register_commands()


class DeepCmdArgFilter(twod.CmdArgFilter):
    @classmethod
    def from_matrix(cls, mx):
        if not isinstance(mx, DeepMatrix):
            mx = DeepMatrix(mx)
        mx2 = mx.get_xy()
        # Remember: the determinant tells us how volumes are transformed.
        scale_factor = abs(mx.det3())**(1.0/3.0)
        filter_point = lambda p: mx2.apply(p)
        filter_scalar = lambda s: Scalar(scale_factor*s)

        mx3 = mx.get_matrix3()
        filters = {Point3: lambda p: mx.apply(p),
                   Z: lambda z: Z(mx.apply_z(z)),
                   Matrix3: lambda mx: mx3.get_product(mx)}
        return cls(filter_scalar, filter_point, filters)

    def __init__(self, filter_scalar, filter_point, filters=None):
        super(DeepCmdArgFilter, self).__init__(filter_scalar, filter_point)
        self.filters = filters or {}

    def __call__(self, arg):
        flt_arg = self.filters.get(arg.__class__)
        if flt_arg is not None:
            return flt_arg(arg)
        if isinstance(arg, Window):
            w = Window()
            w.cmd_stream.add(arg.cmd_stream, self)
            return w
        return super(DeepCmdArgFilter, self).__call__(arg)


class CmdStream(twod.CmdStreamBase):
    pass


@combination(Cmd, CmdStream)
def fn(cmd, cmd_stream):
    cmd_stream.cmds.append(cmd)

@combination(type(None), CmdStream)
def fn(none, cmd_stream):
    # Just ignore None objects.
    pass

@combination(CmdStream, CmdStream)
def fn(child, parent):
    parent.add(child)
