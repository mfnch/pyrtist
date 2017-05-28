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

__all__ = ('Skeleton',)

import numpy as np

from ..lib2d import Color
from .core_types import Matrix3, Point3
from .sphere import Sphere
from .cylinder import Cylinder


class Skeleton(object):
    def __init__(self, root_bone, joint_color=None, bone_color=None,
                 radii_ratio=0.3):
        self.root_bone = root_bone
        self.all_bone_names = root_bone.get_bones().keys()
        self.hidden_bone_names = set()
        self.joint_color = joint_color or Color.red
        self.bone_color = bone_color or Color.blue
        self.radii_ratio = radii_ratio

    def hide_bones(self, *names):
        for name in names:
            if name not in self.all_bone_names:
                print('hide_bones: bone {} not found'.format(name))
            self.hidden_bone_names.add(name)

    def _draw_bone(self, dw, bone, parent_matrix, radius=0.15):
        abs_matrix = np.dot(parent_matrix, bone.matrix)
        if bone.name not in self.hidden_bone_names:
            mx = abs_matrix[:3, :]
            origin = np.array([0.0, 0.0, 0.0, 1.0])
            start = Point3(*np.dot(mx, origin))

            end_pos = bone.get_end_pos()
            if end_pos is not None:
                end = Point3(*np.dot(mx, end_pos))
                end.z = start.z
                bone_length = (end - start).norm()
                radius = min(0.15, 0.25*bone_length)
                r = 0.75*radius
                dw.take(Cylinder(start, end, r, r*self.radii_ratio,
                                 self.bone_color))

            dw.take(Sphere(start, radius, self.joint_color))

        for child in bone.children:
            self._draw_bone(dw, child, abs_matrix, radius)

    def draw(self, dw, parent_matrix):
        self._draw_bone(dw, self.root_bone, parent_matrix)
