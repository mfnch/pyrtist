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

__all__ = ('DeepSurface',)

from .. import deepsurface


class ImageBufferWrapper(deepsurface.ImageBuffer):
    def __init__(self, *args):
        super(ImageBufferWrapper, self).__init__(*args)
        self.cairo_surface = None


class DeepSurface(object):
    def __init__(self, width, height, num_reusable_buffers=5):
        self.width = width
        self.height = height

        self.reusable_image_buffers = []
        self.reusable_depth_buffers = []
        self.num_reusable_buffers = num_reusable_buffers
        self.creation_limit = 20

        self.image_buffer = self.take_image_buffer()
        self.depth_buffer = self.take_depth_buffer()

    def take_image_buffer(self, clear=True):
        if len(self.reusable_image_buffers) > 0:
            ret = self.reusable_image_buffers.pop()
        else:
            if self.creation_limit < 1:
                print('Refuse to create buffer')
                return None
            self.creation_limit -= 1
            ret = deepsurface.ImageBuffer(self.width, self.height)
        if clear:
            ret.clear()
        return ret

    def take_depth_buffer(self, clear=True):
        if len(self.reusable_depth_buffers) > 0:
            ret = self.reusable_depth_buffers.pop()
        else:
            if self.creation_limit < 1:
                print('Refuse to create buffer')
                return None
            self.creation_limit -= 1
            ret = deepsurface.DepthBuffer(self.width, self.height)
        if clear:
            ret.clear()
        return ret

    def give_depth_buffer(self, depth_buffer):
        if len(self.reusable_depth_buffers) < self.num_reusable_buffers:
            self.reusable_depth_buffers.append(depth_buffer)

    def give_image_buffer(self, image_buffer):
        if len(self.reusable_image_buffers) < self.num_reusable_buffers:
            self.reusable_image_buffers.append(image_buffer)

    def get_width(self):
        return self.width

    def get_height(self):
        return self.height

    def get_dst_image_buffer(self):
        return self.image_buffer

    def get_dst_depth_buffer(self):
        return self.depth_buffer

    def transfer(self, depth_buffer, image_buffer):
        deepsurface.transfer(depth_buffer, image_buffer,
                             self.depth_buffer, self.image_buffer)

    def save_to_files(self, image_file_name, normals_file_name):
        self.image_buffer.save_to_file(image_file_name)
        self.depth_buffer.save_to_file(normals_file_name)
