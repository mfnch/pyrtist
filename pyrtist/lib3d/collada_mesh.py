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

import os
import logging
import collections
import numpy as np

import collada

from .. import deepsurface
from .mesh import Mesh
from .bone import Bone

# Use the optimised version of ColladaMesh._pose_calc, if available.
try:
    from .perf_critical import _pose_calc_opt, _poly_calc_opt
except:
    _pose_calc_opt = None
    _poly_calc_opt = None

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
        self._textures = None

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

    @property
    def textures(self):
        if self._textures is None:
            self._textures = self._build_textures()
        return self._textures

    def _build_textures(self):
        try:
            import PIL.Image as pil
        except ImportError:
            pil = None
            logging.warn("No PIL module found. Without PIL textures won't be "
                         "loaded properly.")

        parent_dir = self.collada_node.filename
        if parent_dir:
            parent_dir = os.path.dirname(parent_dir)




        idx = 0


        images = collections.OrderedDict()
        for image in self.collada_node.images:
            image_obj = None
            if pil is not None:
                image_path = image.path
                if not os.path.exists(image_path):
                    file_prefix = 'file://'
                    if image_path.lower().startswith(file_prefix):
                        image_path = image_path[len(file_prefix):]
                    if parent_dir:
                        image_path = os.path.join(parent_dir, image_path)
                pil_image = pil.open(image_path)
                pil_image.load()
                image_obj = self._adjust_texture(pil_image)

                if False:
                    import cairo
                    idx += 1
                    tmp = cairo.ImageSurface(cairo.FORMAT_ARGB32,
                                             pil_image.size[0],
                                             pil_image.size[1])
                    tmp_data = memoryview(tmp)
                    img_data = memoryview(image_obj)
                    tmp_data[:] = img_data[:]
                    tmp.write_to_png('/tmp/{}.png'.format(idx))

            images[image.id] = image_obj

        return images

    def _adjust_texture(self, pil_image):
        ib = deepsurface.ImageBuffer(pil_image.size[0], pil_image.size[1])
        ib.set_from_string(pil_image.tobytes(), pil_image.size[0],
                           pil_image.mode)
        return ib

    def _build_posed_vertices(self, skel, pose, dtype=np.float32):
        controllers = self.collada_node.controllers
        bind_matrices = skel.build_matrices()
        pose_matrices = pose.build_matrices()
        final_matrices = \
          {name: np.dot(pose_matrices[name], np.linalg.inv(mx))[:3]
           for name, mx in bind_matrices.items()}

        # Compute total number of vertices.
        num_vertices = sum(len(prim.vertex)
                           for controller in controllers
                           for prim in controller.geometry.primitives)

        # Create array of output vertices.
        out_vertices = np.zeros((num_vertices, 3), dtype)
        out_idx = 0

        calc = _pose_calc_opt or self._pose_calc

        for controller in controllers:
            # Re-index matrices.
            joint_names = controller.sourcebyid[controller.joint_source]
            joint_matrices = np.zeros((len(joint_names), 3, 4), dtype)
            for idx, name in enumerate(joint_names):
                joint_matrices[idx] = final_matrices[name]

            # Use matrices to re-pose mesh.
            bind_matrix = controller.bind_shape_matrix.T
            weights = controller.weights.data.reshape(-1).astype(dtype)
            for prim in controller.geometry.primitives:
                ext_vertices = extend_point_array(prim.vertex).dot(bind_matrix)
                out_idx = calc(out_vertices, out_idx, ext_vertices,
                               weights, joint_matrices, controller.index)

        return out_vertices.tolist()

    @staticmethod
    def _pose_calc(out_vertices, out_idx,
                   vertices, weights, matrices, indices):
        '''Internal computation used by _build_posed_vertices.

        This is performance critical. An optimised implementation of this
        is provided in the pyrtist.lib3d.perf_critical module.
        '''
        for i, vertex in enumerate(vertices):
            out_vertex = out_vertices[out_idx]
            out_idx += 1
            for midx, widx in indices[i]:
                out_vertex += weights[widx] * np.dot(matrices[midx], vertex)
        return out_idx

    def _build_polygons(self):
        ret = []
        start_v_idx, start_t_idx, start_n_idx = (1, 1, 1)
        materials = self.collada_node.materials
        calc = _poly_calc_opt or self._poly_calc

        for geom_idx, geom in enumerate(self.collada_node.geometries):
            for prim in geom.primitives:
                if isinstance(prim, collada.polylist.Polylist):
                    ps = calc(prim.vertex_index,
                              prim.texcoord_indexset[0],
                              prim.normal_index,
                              start_v_idx, start_t_idx, start_n_idx,
                              prim.polyindex)

                    material = prim.material
                    if material is None and geom_idx < len(materials):
                        material = materials[geom_idx]
                    ret.append((ps, material))
                    start_v_idx += len(prim.vertex)
                    start_t_idx += len(prim.texcoordset[0])
                    start_n_idx += len(prim.normal)
        return ret

    @staticmethod
    def _poly_calc(v, t, n, sv, st, sn, indices):
        return [[(v[idx] + sv, t[idx] + st, n[idx] + sn)
                 for idx in range(start, end)]
                for start, end in indices]

    def _build_vertices(self):
        vertices = []
        for geom in self.collada_node.geometries:
            for prim in geom.primitives:
                if isinstance(prim, collada.polylist.Polylist):
                    vertices.extend(prim.vertex)
        return vertices

    def _build_tex_coords(self):
        tex_coords = []
        for geom in self.collada_node.geometries:
            for prim in geom.primitives:
                if isinstance(prim, collada.polylist.Polylist):
                    tex_coords.extend(prim.texcoordset[0].tolist())
        return tex_coords

    def _build_raw_mesh(self, default_color=(1.0, 1.0, 1.0, 1.0)):
        if self.pose is not None:
            vertices = self._build_posed_vertices(self.origin_pose, self.pose)
        else:
            vertices = self._build_vertices()
        tex_coords = self._build_tex_coords()

        if self.polygons is None:
            self.polygons = self._build_polygons()

        raw_mesh = deepsurface.Mesh()
        if not raw_mesh.add_vertices(vertices):
            logging.warn('ColladaMesh: failed adding vertices')
        if not raw_mesh.add_tex_coords(tex_coords):
            logging.warn('ColladaMesh: failed adding texture coordinates')

        for polys, material in self.polygons:
            # Just look at the diffuse part.
            diffuse_mat = default_color
            if material:
                if material.effect.shadingtype != 'phong':
                    logging.warn('Material shadingtype {} is not supported'
                                 .format(material.effect.shadingtype))
                diffuse_mat = material.effect.diffuse

            if isinstance(diffuse_mat, collada.material.Map):
                surface = diffuse_mat.sampler.surface
                assert surface.format == 'A8R8G8B8'
                diffuse_mat = self.textures[surface.image.id]
            elif not isinstance(diffuse_mat, tuple):
                loging.warn('Unknown diffuse material')
                diffuse_mat = default_color

            if not raw_mesh.add_polygons(polys, diffuse_mat):
                logging.warn('ColladaMesh: failed adding polygons')
        return raw_mesh
