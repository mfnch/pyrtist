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

'''
Utilities to pose 3D meshes and manipulate their bones.
'''

__all__ = ('BoneView', 'Bone', 'Constraints')


import math
import logging
import numpy as np

from pyrtist.lib2d import Point, Tri
from . import Matrix3, Point3


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


# Posing can be done in two ways, depending whether any of the reference points
# was dragged by the user.
#
# 1. When there are no dragged point, the posing is done in one single step (in
#    the enum below we call it UNIQUE_POSE), where all constraints are treated
#    equally and are applied in one go.
#
# 2. When there are one or more dragged point, the posing is done in two posing
#    steps (in the enum we call them INITIAL_POSE, FINAL_POSE), where:
#
#   - in the first initial pose we use the old values of all the dragged
#     reference points. This way we get the skeleton in an initial pose, which
#     is obtained applying all the constraints,
#
#   - in the second final pose we only apply constraints corresponding to
#     reference points which were dragged by the user (i.e. those that have
#     p.gui.old_value attribute). In this case, we automatically move all the
#     reference points that were not dragged by the user. To give an example of
#     what is achieved, when the user moves the elbow reference point all the
#     reference points which are children of the elbow node are automatically
#     re-posed. The user doesn't need to adjust the hand and each of the
#     reference points of the fingers as these are moved automatically in the
#     second posing phase.
(INITIAL_POSE,
 FINAL_POSE,
 UNIQUE_POSE) = range(3)


class Constraints(object):
    def __init__(self, views=None, **kwargs):
        if views is None:
            views = {}
        views.update(kwargs)
        self._views = views
        self._constraints = {}
        self._do_two_poses = False
        if len(views) == 0:
            raise ValueError('Constraints() object must be given at least one '
                             'view')
        if None in views:
            raise ValueError('Invalid view')

    def add(self, bone_name, dst_view_points, axis=None, cnst_type=None,
            forward=True, back=True, repose_children=True):
        # Resolve the views in `dst_view_points' from their names.
        resolved_dst_view_points = []
        two_poses_constraint = False
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
                # This is a dragged point, put it at the beginning of the list,
                # so that the dragged point acts as the primary view point.
                resolved_dst_view_points.insert(0, item)

                # Apply this constraint after all the others, so that undragged
                # bones which are children of the dragged bones are reposed
                # automatically. In other words, this ensures that dragging a
                # bone drags all its children as if they were rigidly connected
                # to it.
                two_poses_constraint = True
            else:
                resolved_dst_view_points.append(item)

        if axis is not None:
            cnst = RotationConstraint(bone_name, resolved_dst_view_points,
                                      axis, forward, back)
        else:
            if len(resolved_dst_view_points) < 2:
                raise ValueError('You should provide either the rotation axis '
                                 'or two view points from two different views')
            if cnst_type in (None, 'rotation'):
                cnst_bld = PrefRotationConstraint
            elif cnst_type == 'translation':
                cnst_bld = PrefTranslationConstraint
            else:
                raise ValueError("Unknown constraint type `{}'"
                                 .format(cnst_type))
            cnst = cnst_bld(bone_name, resolved_dst_view_points, forward, back)
        self._constraints[bone_name] = cnst

        if repose_children and two_poses_constraint:
            self._do_two_poses = True
            cnst.enable_two_poses()

    def _repose_all(self, bone, parent_matrix, pose_type):
        cnst = self._constraints.get(bone.name)
        if cnst is not None and cnst.is_applicable(pose_type):
            bone.matrix = cnst.apply_forward(parent_matrix, bone, pose_type)

        abs_matrix = np.dot(parent_matrix, bone.matrix)
        for child in bone.children:
            self._repose_all(child, abs_matrix, pose_type)

    def apply_forward(self, root_bone):
        '''Compute the mesh pose in `root_bone` from the constraints and the
        current values of the associated view points.

        This method computed the `root_bone` matrices to reflect the values of
        the view variables (`BoneView.variables`).
        '''

        pose_types = ((INITIAL_POSE, FINAL_POSE) if self._do_two_poses
                      else (UNIQUE_POSE,))
        for pose_type in pose_types:
            parent_matrix = np.identity(4, dtype=np.float64)
            self._repose_all(root_bone, parent_matrix, pose_type)

    def apply_backward(self, root_bone):
        '''Compute the position of view points from the given pose `root_bone`.

        Return a list of tuples (view, point_name) for each updated point.
        '''
        bones = root_bone.get_bones()
        mxs = root_bone.build_matrices()
        moved_view_points = []
        for name, cnst in self._constraints.items():
            if not cnst.back:
                continue
            bone = bones[name]
            pts = cnst.apply_backward(mxs[name], bone)
            moved_view_points.extend(pts)
        return moved_view_points


class GenericConstraint(object):
    def __init__(self, bone_name, dst_view_points, forward, back):
        self.bone_name = bone_name
        self.dst_view_points = dst_view_points
        self.forward = forward
        self.back = back
        self.do_two_poses = False

    def enable_two_poses(self, value=True):
        '''Set the range of applicability of this constraint.

        When the two poses mode is enabled, the constraint is applied twice
        during the initial phase, where the old value of the reference point
        associated to the constraint is used (.gui.old_value), and during the
        final phase, where the ordinary value is used.
        '''
        self.do_two_poses = value

    def is_applicable(self, pose_type):
        '''Whether the constraint is applicable to this pose type.'''
        if not self.forward:
            return False
        if self.do_two_poses:
            return True
        return pose_type in (UNIQUE_POSE, INITIAL_POSE)

    def _extract_point_and_angle(self, view, name, pose_type):
        # Get the "new" and "old" values of the reference point.
        new_view_point = view.variables[name]
        gui_attrs = getattr(new_view_point, 'gui', None)
        if gui_attrs is not None and gui_attrs.old_value is not None:
            old_view_point = gui_attrs.old_value
        else:
            old_view_point = None

        view_point = new_view_point
        if pose_type == INITIAL_POSE and old_view_point:
            # Look at the old values of the reference point.
            view_point = old_view_point

        angle = None
        if isinstance(view_point, Tri):
            if pose_type == FINAL_POSE and old_view_point:
                dp_old = old_view_point.ip - old_view_point.p
                tri_was_just_created = (dp_old.norm() == 0.0)
                if tri_was_just_created:
                    # This Tri reference point was just created. We ignore its
                    # value so that apply_backward() can set the initial
                    # rotation angle. The user will have a chance of changing
                    # the reference point and the associated angle in future
                    # drag actions.
                    return (Point(view_point.p), angle)
            dp = view_point.ip - view_point.p
            view_point = view_point.p
            if dp.norm2() > 0.0:
                angle = dp.angle()
        return (Point(view_point), angle)


class RotationConstraint(GenericConstraint):
    def __init__(self, bone_name, dst_view_points, rotation_axis,
                 forward, back):
        super(RotationConstraint, self).__init__(bone_name, dst_view_points,
                                                 forward, back)
        rpn = Point3(rotation_axis)
        self.rotation_axis = np.array([rpn.x, rpn.y, rpn.z])

    def apply_forward(self, parent_matrix, bone, pose_type):
        # The first point is the one used to calculate the constraints. The
        # other points are never used as input of the posing, only as output
        # (in case the user wants to see where the bone end is from other
        # views).
        child_matrix = bone.matrix
        desired_view, point_name = self.dst_view_points[0]
        desired_bone_end_position, bone_axis_rotation = \
          self._extract_point_and_angle(desired_view, point_name, pose_type)

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
        bone_end_pos = Point3.from_np(bone.get_end_pos())
        mx = (Matrix3.rotation_by_example(bone_end_pos, Point3(*new_bone_dir)) *
              Matrix3.rotation(bone_axis_rotation, axis=bone_end_pos))
        child_matrix[:3, :3] = mx.to_np33()
        return child_matrix


class PreferentialConstraint(GenericConstraint):
    '''This class provides an algorithm to determine the coordinates of a 3D
    point from the 2D projections of this point on two or more views.
    Constraints inheriting from this class, use this algorithm to map the 2D
    reference points for the same joint in different views into one single
    set of 3D coordinates.

    DETAILS: Every element of self.dst_view_points provides 2 constraints on
    the 3 coordinates of the 3D point we are trying to determine, r.
    Consequently, every such element identifies a line in 3D space which - in
    theory - should contain r. However, two lines in 3D space are not
    guaranteed to have an intersection. We must therefore adopt a strategy to
    identify r, even when there are no intersections between the lines (e.g.
    due to calculation errors). `PreferentialConstraint` adopts the strategy of
    partitioning the list of constraints in one primary constraint and other
    many secondary constraints. It assumes r lies inside the line identified by
    the primary constraint. r is then determined by finding the point in this
    line which minimises the distance to all the lines identified by the
    secondary constraints.
    '''

    def _calc_point3_and_angles(self, bone_matrix, pose_type):
        assert len(self.dst_view_points) >= 2, \
          'PreferentialConstraint requires at least two view points'

        # Rotation around the bone axis.
        self_rotation_angles = []

        # For every constraint we determine the 3D line it corresponds to.
        # (see comment in the class' docstring.) The line is identified by
        # one of its points, line_memb, and the line direction, line_dir.
        # These values as stored as tuples in the `lines` list below.
        lines = []
        for view, point_name in self.dst_view_points:
            # full_proj below is a 2x4 matrix.
            full_proj = np.dot(view.view_matrix, bone_matrix)

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

            p, angle = \
              self._extract_point_and_angle(view, point_name, pose_type)
            if angle is not None:
                self_rotation_angles.append(angle)

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
        r = Point3.from_np(primary[0] + alpha*primary[1])
        return (r, self_rotation_angles)

    def _calc_refpoints(self, bone_matrix, point3, angle):
        moved_view_points = []
        for view, point_name in self.dst_view_points:
            mx = np.dot(view.view_matrix, bone_matrix)
            new_value = Point(*np.dot(mx, point3)[:2])
            old_value = view.variables[point_name]
            if isinstance(old_value, Tri) and old_value.ip is not None:
                # If we are dealing with a Tri, then new_value must be a
                # Tri as well.
                r = (old_value.p - old_value.ip).norm()
                p = new_value
                new_value = Tri(p + r*Point.vangle(angle), p)

            view.variables[point_name] = new_value
            moved_view_points.append((view, point_name))
        return moved_view_points


class PrefRotationConstraint(PreferentialConstraint):
    '''Use the preferential view algorithm (see PreferentialConstraint) to find
    where the bone end-position should go. From this determine the bone matrix,
    also allowing the user to specify the rotation around the bone axis via a
    Tri point. This is done as follow: when the end bone is a Tri point, the
    in-point of the Tri is used to encode the angle of rotation around the bone
    axis (with something like `(tri.p - tri.ip).angle()`.
    '''

    def apply_forward(self, parent_matrix, bone, pose_type):
        bone_matrix = np.dot(parent_matrix, bone.matrix)
        r, angles = (super(PrefRotationConstraint, self)
                     ._calc_point3_and_angles(bone_matrix, pose_type))

        bone_end_pos_np = bone.get_end_pos()
        bone_end_pos = Point3.from_np(bone_end_pos_np)
        mx = np.dot(bone.matrix[:3, :3],
                    Matrix3.rotation_by_example(bone_end_pos, r).to_np33())
        if angles:
            # The user provided a custom rotation around the bone's axis.
            angle = sum(angles) / float(len(angles))
            self_mx = Matrix3.rotation(angle, bone_end_pos).to_np33()

            final_pos = np.dot(mx, bone_end_pos_np[:3])
            mx = Matrix3.rotation_by_example(bone_end_pos,
                                             Point3.from_np(final_pos))
            mx = np.dot(mx.to_np33(), self_mx)

        bone.matrix[:3, :3] = mx
        return bone.matrix

    def apply_backward(self, bone_matrix, bone):
        return self._calc_refpoints(bone_matrix, bone.get_end_pos(),
                                    bone.get_self_rotation())


class PrefTranslationConstraint(PreferentialConstraint):
    '''This constraint only affects the translation part of the bone matrix.
    It uses the preferential view algorithm (see PreferentialConstraint) to
    determine the position of the bone start point. This is typically used on
    the root node to translate the whole mesh.
    '''

    def apply_forward(self, parent_matrix, bone, pose_type):
        r, angles = (super(PrefTranslationConstraint, self)
                     ._calc_point3_and_angles(parent_matrix, pose_type))
        if angles:
            logging.warn("You gave angle to a translation constraint for "
                         "`{}', but translation constraints don't take angles"
                         .format(bone.name))
        bone.matrix[:3, 3] = r.to_np()
        return bone.matrix

    def apply_backward(self, bone_matrix, bone):
        bone_start = np.array([0.0, 0.0, 0.0, 1.0])
        return self._calc_refpoints(bone_matrix, bone_start, 0.0)


class Bone(object):
    @classmethod
    def from_collada_node(cls, node):
        '''Construct the mesh skeleton from a Collada node.'''
        bone = cls(node.id, node.transforms[0].matrix)
        for child in node.children:
            bone.children.append(cls.from_collada_node(child))
        return bone

    def __init__(self, name, matrix, children=(), end_pos=None):
        self.name = name
        self.end_pos = (end_pos.copy() if end_pos is not None else None)
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

    def set_end_pos(self, end_pos):
        '''Set the bone end position.'''
        self.end_pos = end_pos

    def get_end_pos(self):
        '''Return the end bone position in the bone's own reference system.'''
        if self.end_pos is not None:
            p = self.end_pos
            return np.array([p.x, p.y, p.z, 1.0])
        if len(self.children) == 0:
            return None
        ps = [child.matrix[:, 3] for child in self.children]
        return sum(ps[1:], ps[0]) / float(len(ps))

    def get_self_rotation(self):
        '''Get the angle of rotation around the bone itself.'''
        initial_pos = self.get_end_pos()[:3]
        final_pos = np.dot(self.matrix[:3, :3], initial_pos)
        mx = Matrix3.rotation_by_example(Point3.from_np(final_pos),
                                         Point3.from_np(initial_pos))
        self_rot_mx = np.dot(mx.to_np33(), self.matrix[:3, :3])
        axis, angle = Matrix3.np_get_axis_and_angle(self_rot_mx)
        if axis is None:
            return 0.0
        if np.dot(axis, initial_pos) >= 0.0:
            return angle
        else:
            return -angle

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
        self_copy = self.__class__(self.name, self.matrix.copy(),
                                   end_pos=self.end_pos)
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
