'''
Utilities to load, display and pose 3D meshes.
'''

import math
import numpy as np

from pyrtist.lib2d import Point
from pyrtist.lib3d import Matrix3, Point3


def rotate_y_axis(v):
    phi = math.atan2(v[0], v[1])
    theta = math.atan2(v[2], (v[0]**2 + v[1]**2)**0.5)
    return (Matrix3.rotation(-phi, axis='z') *
            Matrix3.rotation(theta, axis='x') *
            Matrix3.rotation(-phi, axis='y'))


class ReposeInfo(object):
    def __init__(self, desired_bone_end_position, rotation_plane_normal):
        rpn = Point3(rotation_plane_normal)
        self.rotation_plane_normal = np.array([rpn.x, rpn.y, rpn.z])
        self.desired_bone_end_position = Point(desired_bone_end_position)


class Bone(object):
    @classmethod
    def from_collada_node(cls, node):
        '''Construct the mesh skeleton from a Collada node.'''
        bone = cls(node.id, node.transforms[0].matrix)
        for child in node.children:
            bone.children.append(cls.from_collada_node(child))
        return bone

    def __init__(self, name, matrix, children=()):
        self.name = name
        self.matrix = matrix.copy()
        self.children = list(children)
        self.repose_info = None

    def __str__(self):
        return self.to_str()

    def to_str(self, indent='', keyword=''):
        lines = ['{}{}: {} children'
                 .format(indent, self.name, len(self.children))]
        indent = indent + '  '
        for child in self.children:
            lines.append(child.to_str(indent, keyword))
        return '\n'.join(line for line in lines if keyword in line)

    def get_bones(self, bones=None):
        '''Return all bones as a flat dictionary mapping the bone name to the
        Bone object.
        '''
        bones = bones or {}
        bones[self.name] = self
        for child in self.children:
            child.get_bones(bones)
        return bones

    def build_matrices(self, parent_matrix=None, matrices=None):
        '''Calculate the absolute bone matrices and return them as a dictionary
        mapping the name to the bone matrix.'''
        abs_matrix = (np.dot(parent_matrix, self.matrix)
                      if parent_matrix is not None else self.matrix)
        matrices = matrices or {}
        matrices[self.name] = abs_matrix
        for child in self.children:
            child.build_matrices(abs_matrix, matrices)
        return matrices

    def give_repose_constraint(self, *args):
        self.repose_info = ReposeInfo(*args)

    def repose_all(self, parent_matrix=None, proj=None):
        if parent_matrix is None:
            parent_matrix = np.zeros((3, 4), dtype=np.float32)
            parent_matrix[:3, :3] = np.identity(3, dtype=np.float32)

        if self.repose_info is not None:
            # We get rid of the rotation part of self.matrix, as we are going to
            # recalculate it anyway and we want to do this in a well known
            # reference system.
            mx = self.matrix.copy()
            mx[:3, :3] = np.identity(3, dtype=np.float32)
            abs_matrix = np.dot(parent_matrix, mx)

            # full_proj projects the relative bone coordinates to screen
            # coordinates.
            if proj is None:
                proj = np.array([[1.0, 0.0, 0.0, 0.0],
                                 [0.0, 1.0, 0.0, 0.0]])
            full_proj = np.dot(proj, abs_matrix)

            # We need to repose the bone so that it lean towards the direction
            # self.repose_info.desired_bone_end_position. We end up having to
            # solve an equation like: lhs_mx * (x, y, z) = rhs, so we compute
            # the Left-Hand-Side matrix, invert it and apply to rhs. (x, y, z)
            # is the new bone end position.
            lhs_mx = np.zeros((3, 3))
            lhs_mx[:2, :] = full_proj[:, :3]
            lhs_mx[2, :] = self.repose_info.rotation_plane_normal
            p = self.repose_info.desired_bone_end_position
            rhs = np.array([p.x - full_proj[0, 3], p.y - full_proj[1, 3], 0.0])
            new_bone_dir = np.zeros((4,))
            new_bone_dir[:3] = np.dot(np.linalg.inv(lhs_mx), rhs)
            new_bone_dir[3] = 1.0

            # From the new bone direction, recompute the bone rotation matrix.
            mx = rotate_y_axis(new_bone_dir)
            rot_mx = mx.to_np33()
            self.matrix[:3, :3] = rot_mx

        abs_matrix = np.dot(parent_matrix, self.matrix)
        for child in self.children:
            child.repose_all(abs_matrix, proj)

    def copy(self):
        '''Return a new independent copy of this object.'''
        self_copy = self.__class__(self.name, self.matrix.copy())
        for child in self.children:
            self_copy.children.append(child.copy())
        return self_copy

    def save_debug(self, file_name):
        '''Convert the object to string and save it to a file with the given
        file name.
        '''
        with open(file_name, 'w') as f:
            f.write(str(self))

    def ls(self, keyword):
        '''Print the bone names containing the given keyword.'''
        print('\n'.join(x for x in self.get_bones().keys() if keyword in x))
