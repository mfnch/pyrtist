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
Load and manipulation of 3D meshes.
'''

__all__ = ('Mesh', 'ObjMesh')


import os

from .. import deepsurface


class Mesh(object):
    @staticmethod
    def new_collada(*args, **kwargs):
        try:
            from .collada_mesh import ColladaMesh
        except ImportError:
            raise ImportError('Please install python-collada to '
                              'load meshes using the Collada file format')
        return ColladaMesh(*args, **kwargs)

    @staticmethod
    def new_obj(cls, file_name):
        return ObjMesh(file_name)

    @classmethod
    def load(cls, file_name, file_format=None):
        '''Load a mesh from file and return the corresponding Mesh object.'''
        if file_format is None:
            file_format = os.path.splitext(file_name)[-1]
        load_methods = {'.dae': cls.new_collada,
                        'collada': cls.new_collada,
                        '.obj': cls.new_obj}
        load_method = load_methods.get(file_format.lower())
        if load_method is None:
            raise ValueError('You must provide the mesh file format')
        return load_method(file_name=file_name)

    def __init__(self):
        pass


class ObjMesh(Mesh):
    def __init__(self, file_name=None):
        super(ObjMesh, self).__init__()
        self.file_name = file_name

    def _build_raw_mesh(self):
        return deepsurface.Mesh(self.file_name)
