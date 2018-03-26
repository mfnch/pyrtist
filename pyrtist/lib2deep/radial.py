# Copyright (C) 2017 Matteo Franchin
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

'''Lath object: allows drawing objects with cylindrical symmetry.'''

__all__ = ('Radial',)

import types

from ..lib2d import combination, Point, Radii, Window, Circle, Color, Axes
from ..lib2d import Drawable
from .core_types import Point3, Z
from .profile import Profile
from .cmd_stream import Cmd, CmdStream
from .deep_window import DeepWindow
from .primitive import Primitive


class Radial(Primitive):
    def __init__(self, *args):
        super(Radial, self).__init__()
        self.center = None
        self.radii = None
        self.profile = None
        self.take(*args)
        self.check()

    def check(self, quiet=False):
        '''Check that this object is well formed.'''
        ok = (isinstance(self.radii, Radii) and
              isinstance(self.center, Point3) and
              isinstance(self.profile, types.FunctionType))
        if quiet or ok:
            return ok
        detail = ('center' if self.center is None else 'radius')
        raise ValueError('Radial without {} cannot be passed to CmdStream'
                         .format(detail))

    def get_profile(self, *args):
        return Circle(self.center.xy, self.get_radii()[0], *args)

    def get_radii(self):
        if self.radii is None:
            return (1.0, 1.0)
        self.radii.check(0, 2)
        return (tuple(self.radii) + (1.0, 1.0))[:2]

    def build_depth_cmd(self):
        p1 = self.center
        r, rz = self.get_radii()
        p3 = Point3(p1.x + r, p1.y, p1.z + rz)
        p2 = Point3(p1.x, p1.y + r, p1.z + rz)
        return [Cmd(Cmd.on_radial, p1, p2, p3, self.profile)]

@combination(Radii, Radial)
def radii_at_radial(radii, radial):
    assert radial.radii is None, 'Radii already provided'
    radial.radii = radii

@combination(Point, Radial)
@combination(Point3, Radial)
def point3_at_radial(p, radial):
    assert radial.center is None, 'Center already provided'
    radial.center = Point3(p)

@combination(types.FunctionType, Radial)
def profile_at_radial(fn, radial):
    assert radial.profile is None, 'Profile already provided'
    radial.profile = fn
