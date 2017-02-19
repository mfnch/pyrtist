'''
Utilities to load, display and pose 3D meshes.
'''

import math
import numpy as np

from pyrtist.lib2d import Point, Tri
from pyrtist.lib3d import Matrix3, Point3


def _extract_point_and_angle(view_point):
    if isinstance(view_point, Tri):
        angle = (view_point.ip - view_point.p).angle()
        return (view_point.p, angle)
    else:
        return (Point(view_point), 0.0)


class BoneView(object):
    def __init__(self, transform_matrix=None, proj_matrix=None,
                 variables=None):
        if transform_matrix is None:
            transform_matrix = np.zeros((3, 4), dtype=np.float64)
            transform_matrix[:3, :3] = np.identity(3, dtype=np.float64)
        elif isinstance(transform_matrix, Matrix3):
            transform_matrix = transform_matrix.to_np44()
        if proj_matrix is None:
            proj_matrix = np.array([[1.0, 0.0, 0.0, 0.0],
                                    [0.0, 1.0, 0.0, 0.0]])
        if variables is None:
            variables = {}
        self.variables = variables
        self.transform_matrix = transform_matrix
        self.proj_matrix = proj_matrix

    @property
    def view_matrix(self):
        return np.dot(self.proj_matrix, self.transform_matrix)


class Constraints(object):
    def __init__(self, views=None, **kwargs):
        if views is None:
            views = {}
        views.update(kwargs)
        self._views = views
        self._constraints = {}
        if len(views) == 0:
            raise ValueError('Constraints() object must be given at least one '
                             'view')
        if None in views:
            raise ValueError('Invalid view')

    def add(self, bone_name, dst_view_points, axis=None, move=False):
        # Resolve the views in `dst_view_points' from their names.
        resolved_dst_view_points = []
        for i, dst_view_point in enumerate(dst_view_points):
            if '/' not in dst_view_point:
                raise ValueError("Invalid format for destination view point "
                                 "'{}' should be 'view_name/point_name'"
                                 .format(dst_view_point))
            view_name, point_name = \
              (s.strip() for s in dst_view_point.split('/', 1))
            view = self._views.get(view_name, None)
            if view_name not in self._views:
                raise ValueError("Invalid view name given in '{}'"
                                 .format(dst_view_point))
            if point_name not in view.variables:
                raise ValueError("Cannot find point name '{}' in variables "
                                 "for view '{}'".format(point_name, view_name))

            item = (view, point_name)
            gui_attrs = getattr(view.variables[point_name], 'gui', None)
            if gui_attrs is not None and gui_attrs.old_value is not None:
                # This is a dragged point, put it at the beginning of the list.
                resolved_dst_view_points.insert(0, item)
            else:
                resolved_dst_view_points.append(item)

        if axis is not None:
            cnst = RotationConstraint(bone_name, resolved_dst_view_points,
                                      axis, move)
        else:
            if len(resolved_dst_view_points) < 2:
                raise ValueError('You should provide either the rotation axis '
                                 'or two view points from two different views')
            cnst = PreferentialConstraint(bone_name, resolved_dst_view_points,
                                          move)
        self._constraints[bone_name] = cnst

    def _repose_all(self, bone, parent_matrix):
        constraint = self._constraints.get(bone.name)
        if constraint is not None:
            bone.matrix = constraint.apply(parent_matrix, bone)

        abs_matrix = np.dot(parent_matrix, bone.matrix)
        for child in bone.children:
            self._repose_all(child, abs_matrix)

    def apply_forward(self, root_bone):
        '''Compute the mesh pose in `root_bone` from the constraints and the
        current values of the associated view points.

        This method computed the `root_bone` matrices to reflect the values of
        the view variables (`BoneView.variables`).
        '''
        self._repose_all(root_bone, np.identity(4, dtype=np.float64))

    def apply_backward(self, root_bone):
        '''Compute the position of view points from the given pose `root_bone`.

        Return a list of tuples (view, point_name) for each updated point.
        '''
        bones = root_bone.get_bones()
        mxs = root_bone.build_matrices()
        moved_view_points = []
        for name, constraint in self._constraints.items():
            if not constraint.move:
                continue
            bone = bones[name]
            bone_end_pos = bone.get_end_pos()
            for view, point_name in constraint.dst_view_points:
                mx = np.dot(view.view_matrix, mxs[name])
                new_end_pos = np.dot(mx, bone_end_pos)[:2]
                view.variables[point_name] = new_end_pos
                moved_view_points.append((view, point_name))
        return moved_view_points


class GenericConstraint(object):
    def __init__(self, bone_name, dst_view_points, move):
        self.bone_name = bone_name
        self.dst_view_points = dst_view_points
        self.move = move


class RotationConstraint(GenericConstraint):
    def __init__(self, bone_name, dst_view_points, rotation_axis, move):
        super(RotationConstraint, self).__init__(bone_name, dst_view_points, move)
        rpn = Point3(rotation_axis)
        self.rotation_axis = np.array([rpn.x, rpn.y, rpn.z])

    def apply(self, parent_matrix, bone):
        # The first point is the one used to calculate the constraints. The
        # other points are never used as input of the posing, only as output
        # (in case the user wants to see where the bone end is from other
        # views).
        child_matrix = bone.matrix
        desired_view, point_name = self.dst_view_points[0]
        desired_bone_end_position, bone_axis_rotation = \
          _extract_point_and_angle(desired_view.variables[point_name])

        # We get rid of the rotation part of `matrix', as we are going to
        # recalculate it anyway and we want to do this in a well known
        # reference system.
        mx = child_matrix.copy()
        mx[:3, :3] = np.identity(3, dtype=np.float64)
        abs_matrix = np.dot(parent_matrix, mx)

        # full_proj projects the relative bone coordinates to screen
        # coordinates.
        full_proj = np.dot(desired_view.view_matrix, abs_matrix)

        # We need to repose the bone so that it leans towards the direction
        # desired_bone_end_position. We end up having to solve an equation
        # like: lhs_mx * (x, y, z) = rhs, so we compute the Left-Hand-Side
        # matrix, invert it and apply to rhs. (x, y, z) is the new bone end
        # position.
        lhs_mx = np.zeros((3, 3))
        lhs_mx[:2, :] = full_proj[:, :3]
        lhs_mx[2, :] = self.rotation_axis
        p = desired_bone_end_position
        rhs = np.array([p.x - full_proj[0, 3], p.y - full_proj[1, 3], 0.0])
        new_bone_dir = np.zeros((3,))
        new_bone_dir = np.dot(np.linalg.inv(lhs_mx), rhs)

        # From the new bone direction, recompute the bone rotation matrix.
        bone_end_pos = Point3(*bone.get_end_pos()[:3])
        mx = (Matrix3.rotation_by_example(bone_end_pos, Point3(*new_bone_dir)) *
              Matrix3.rotation(bone_axis_rotation, axis=bone_end_pos))
        child_matrix[:3, :3] = mx.to_np33()
        return child_matrix


class PreferentialConstraint(GenericConstraint):
    '''Every element of self.dst_view_points provides 2 constraints on the 3
    coordinates of the 3D point we are trying to determine, r, the end bone
    position. Consequently, every such element identifies a line in 3D space
    which - in theory - should contain r. However, two lines in 3D space are
    not guaranteed to have an intersection. We must therefore adopt a strategy
    to identify r, even when there are no intersections between the lines (e.g.
    due to calculation errors). `PreferentialConstraint` adopts the strategy of
    partitioning the list of constraints in one primary constraint and other
    many secondary constraints. It assumes r lies inside the line identified by
    the primary constraint. r is then determined by finding the point in this
    line which minimises the distance to all the lines identified by the
    secondary constraints.
    '''

    def __init__(self, bone_name, dst_view_points, move):
        super(PreferentialConstraint, self).__init__(bone_name,
                                                     dst_view_points, move)

    def apply(self, parent_matrix, bone):
        assert len(self.dst_view_points) >= 2, \
          'PreferentialConstraint requires at least two view points'

        mx = bone.matrix.copy()
        mx[:3, :3] = np.identity(3, dtype=np.float64)
        abs_matrix = np.dot(parent_matrix, mx)

        # Rotation around the bone axis.
        bone_axis_rotation = 0.0

        # For every constraint we determine the 3D line it corresponds to.
        # (see comment in the class' docstring.) The line is identified by
        # one of its points, line_memb, and the line direction, line_dir.
        # These values as stored as tuples in the `lines` list below.
        lines = []
        for view, point_name in self.dst_view_points:
            # full_proj below is a 2x4 matrix.
            full_proj = np.dot(view.view_matrix, abs_matrix)

            # We now build a 3x3 matrix from full_proj.
            # The third row is obtained from the vector product of the top
            # two rows and is thus guaranteed to be invertible, as long as the
            # scalar product of the top two rows is nonzero.
            lhs_mx = np.zeros((3, 3))
            lhs_mx[:2, :] = full_proj[:, :3]
            line_dir = np.cross(lhs_mx[0], lhs_mx[1])
            line_dir_norm = np.linalg.norm(line_dir)
            if not line_dir_norm > 0.0:
                raise ValueError('Degenerate view for {}'.format(point_name))
            line_dir /= line_dir_norm
            lhs_mx[2] = line_dir

            p, angle = _extract_point_and_angle(view.variables[point_name])
            bone_axis_rotation += angle
            rhs = np.array([p.x - full_proj[0, 3], p.y - full_proj[1, 3], 0.0])
            lhs_mx_inv = np.linalg.inv(lhs_mx)
            line_memb = np.dot(lhs_mx_inv, rhs)

            lines.append((line_memb, line_dir))

        # We assume the first view point is the primary one.
        primary = lines[0]
        secondaries = lines[1:]

        # The point we are trying to determine can be therefore expressed as:
        #
        #   primary[0] + alpha*primary[1]
        #
        # For a suitable alpha. Below we determine this alpha.
        dividend = 0.0
        divisor = 0.0
        for secondary in secondaries:
            delta_memb = secondary[0] - primary[0]
            ort = primary[1] - np.dot(primary[1], secondary[1])*secondary[1]
            dividend += np.dot(delta_memb, ort)
            divisor += np.dot(ort, ort)
        alpha = dividend / divisor

        # We can now compute the new bone direction, r, and from this the bone
        # rotation matrix.
        r = Point3(*(primary[0] + alpha*primary[1]))
        bone_end_pos = Point3(*bone.get_end_pos()[:3])
        mx = (Matrix3.rotation_by_example(bone_end_pos, r) *
              Matrix3.rotation(bone_axis_rotation, axis=bone_end_pos))
        bone.matrix[:3, :3] = mx.to_np33()
        return bone.matrix


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

    def __str__(self):
        return self.to_str()

    def to_str(self, indent='', keyword=''):
        lines = ['{}{}: {} children'
                 .format(indent, self.name, len(self.children))]
        indent = indent + '  '
        for child in self.children:
            lines.append(child.to_str(indent, keyword))
        return '\n'.join(line for line in lines if keyword in line)

    def get_end_pos(self):
        '''Return the end bone position in the bone's own reference system.'''
        pos = None
        for child in self.children:
            t = child.matrix[:, 3]
            if pos is None:
                pos = t
            else:
                pos = pos + t
        return pos

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
