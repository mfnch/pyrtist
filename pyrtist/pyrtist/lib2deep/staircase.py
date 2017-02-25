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

'''Staircase object: allow drawing on a staircase (piecewise planar surface).
'''

__all__ = ('Staircase',)

from ..lib2d import Window, Point, Axes
from ..lib2d.base import combination, RejectError
from .core_types import Z
from .profile import Profile
from .cmd_stream import Cmd, CmdStream
from .primitive import Primitive


class Staircase(Primitive):
    def __init__(self, *args):
        self.axes = None
        self.profile = None
        super(Staircase, self).__init__(*args)

    def get_profile(self, *args):
        w = Window()
        w.take(*args)
        return w

    def build_depth_cmd(self):
        if not self.profile.is_valid():
            raise ValueError('Invalid Profile object')

        # Create a list of points in coordinate system of the profile object.
        profile = self.profile
        mx_transf = profile.axes.get_matrix().get_inverse()
        ps_transf = [mx_transf.apply(p) for p in profile.points]
        ps_transf.sort(key=lambda p: p.x)

        # Now, for every point in the profile object, find its projection into
        # the x-axis of the staircase reference system. Also find the z offset
        # for the same point.
        mx_real = (self.axes or profile.axes).get_matrix()
        proj_and_z_offs = [(mx_real.apply(Point(p_transf.x, 0)),
                            p_transf.y)
                           for p_transf in ps_transf]
        cmds = []
        for i in range(1, len(proj_and_z_offs)):
            start, z_start = proj_and_z_offs[i - 1]
            end, z_end = proj_and_z_offs[i]
            cmds.append(Cmd(Cmd.on_step, start, Z(z_start), end, Z(z_end)))
        return cmds

@combination(Profile, Staircase)
def profile_at_staircase(profile, staircase):
    if staircase.profile is not None:
        raise RejectError('Staircase already got a Profile')
    staircase.profile = profile

@combination(Axes, Staircase)
def window_at_staircase(axes, staircase):
    if staircase.axes is not None:
        raise RejectError('Staircase already got an Axes')
    staircase.axes = axes
