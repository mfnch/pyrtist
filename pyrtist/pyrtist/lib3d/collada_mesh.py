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

import logging
import numpy as np

import collada

from .. import deepsurface
from .mesh import Mesh
from .bone import Bone


def extend_point_array(point3_array):
    '''Given a numpy array of 3D points, extend it to an array of 4D points,
    all identical to the original points, but with a 1.0 as their 4th
    coordinate.'''
    assert(len(point3_array.shape) == 2 and point3_array.shape[1] == 3)
    point4_array = np.zeros((len(point3_array), 4), point3_array.dtype)
    point4_array[:, :3] = point3_array
    point4_array[:, 3] = 1.0
    return point4_array


class ColladaMesh(Mesh):
    def __init__(self, matrix=None, collada_node=None, file_name=None):
        super(ColladaMesh, self).__init__()
        self.matrix = matrix
        self.origin_pose = None
        self.pose = None
        self.collada_node = None
        self.polygons = None

        if collada_node is not None:
            self.collada_node = collada_node
            if file_name is None:
                logging.warn('ColladaMesh: file_name is ignored when the node '
                             'is provided')
        elif file_name is not None:
            self.collada_node = collada.Collada(file_name)
        else:
            raise TypeError('ColladaMesh needs one of its key arguments '
                            'collada_node or file_name')

    def _build_posed_vertices(self, skel, pose):
        bind_matrices = skel.build_matrices()
        inv_bind_matrices = {k: np.linalg.inv(mx)
                             for k, mx in bind_matrices.items()}
        pose_matrices = pose.build_matrices()
        out_vertex = np.array([0.0, 0.0, 0.0])
        out_vertices = []

        for controller in self.collada_node.controllers:
            # Pre-compute matrices.
            joint_names = list(controller.sourcebyid[controller.joint_source])
            joint_matrices = []
            for i in range(controller.max_joint_index + 1):
                name = joint_names[i]
                mx44 = np.dot(pose_matrices[name], inv_bind_matrices[name])
                joint_matrices.append(mx44[:3])

            # Use matrices to re-pose mesh.
            bind_matrix = controller.bind_shape_matrix.T
            for prim in controller.geometry.primitives:
                ext_vertices = extend_point_array(prim.vertex).dot(bind_matrix)
                for i, ext_vertex in enumerate(ext_vertices):
                    out_vertex = 0.0
                    for midx, widx in controller.index[i]:
                        weight = controller.weights[widx]
                        mx = joint_matrices[midx]
                        out_vertex += weight*np.dot(mx, ext_vertex)
                    out_vertices.append(out_vertex.tolist())

        return out_vertices

    def _build_polygons(self):
        ret = []
        start_idx = 1
        for geom in self.collada_node.geometries:
            for prim in geom.primitives:
                if isinstance(prim, collada.polylist.Polylist):
                    ps = [[(prim.vertex_index[idx] + start_idx,
                            prim.texcoord_indexset[0][idx] + start_idx,
                            prim.normal_index[idx] + start_idx)
                            for idx in range(start, end)]
                          for start, end in prim.polyindex]
                    ret.append(ps)
                    start_idx += len(prim.vertex)
        return ret

    def _build_vertices(self):
        vertices = []
        for geom in self.collada_node.geometries:
            for prim in geom.primitives:
                if isinstance(prim, collada.polylist.Polylist):
                    vertices.extend(prim.vertex)
        return vertices

    def _build_raw_mesh(self):
        if self.pose is not None:
            vertices = self._build_posed_vertices(self.origin_pose, self.pose)
        else:
            vertices = self._build_vertices()

        if self.polygons is None:
            self.polygons = self._build_polygons()

        raw_mesh = deepsurface.Mesh()
        raw_mesh.add_vertices(vertices)
        for polys in self.polygons:
            raw_mesh.add_polygons(polys)
        return raw_mesh
