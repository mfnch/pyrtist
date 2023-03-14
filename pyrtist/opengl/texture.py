# Copyright (C) 2023 Matteo Franchin
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

import OpenGL
import OpenGL.GL as GL
import PIL.Image
import numpy as np
import enum

from .common import *

__all__ = ('Texture', 'Wrap', 'Filter', 'GLTexture')


class Wrap(enum.Enum):
    CLAMP_TO_EDGE = GL.GL_CLAMP_TO_EDGE
    MIRRORED_REPEAT = GL.GL_MIRRORED_REPEAT
    REPEAT = GL.GL_REPEAT


class Filter(enum.Enum):
    # NAME = (GL value, use_mipmap)
    LINEAR = (GL.GL_LINEAR, False)
    NEAREST = (GL.GL_NEAREST, False)
    NEAREST_MIPMAP_NEAREST = (GL.GL_NEAREST_MIPMAP_NEAREST, True)
    LINEAR_MIPMAP_NEAREST = (GL.GL_LINEAR_MIPMAP_NEAREST, True)
    NEAREST_MIPMAP_LINEAR = (GL.GL_NEAREST_MIPMAP_LINEAR, True)
    LINEAR_MIPMAP_LINEAR = (GL.GL_LINEAR_MIPMAP_LINEAR, True)


class Texture:
    Wrap = Wrap
    Filter = Filter

    def __init__(self):
        self._texture = None
        self._content = None
        self._size = None
        self._wrap_s = Wrap.CLAMP_TO_EDGE
        self._wrap_t = Wrap.CLAMP_TO_EDGE
        self._min_filter = Filter.LINEAR
        self._mag_filter = Filter.LINEAR
        self._impl = None                  # Associated GLTexture object.

    def set(self, content=None, size=None, wrap_x=None, wrap_y=None,
            min_filter=None, mag_filter=None):
        '''
        Set image attributes.
        Attributes given as None are not set. Default values above are those
        created at construction of this object.

        Args:
          content : default=None
            Numpy array containing the image data
          size : tuple[int, int], default=None
            Size of the texture
          wrap_x : Wrap, default=Wrap.CLAMP_TO_EDGE
            Wrapping along the x direction fo the texture.
          wrap_y : Wrap, default=Wrap.CLAMP_TO_EDGE
            Wrapping along the y direction fo the texture.
          min_filter : Filter, default=Filter.LINEAR
            Minification filter.
          mag_filter : Filter, default=Filter.LINEAR
            Magnification filter.
        '''
        set_optional(self, _content=content, _size=size,
                     _wrap_s=wrap_x, _wrap_t=wrap_y,
                     _min_filter=min_filter, _mag_filter=mag_filter)

    def set_content_from_file(self, file_name):
        with PIL.Image.open(file_name) as image:
            flipped = image.transpose(PIL.Image.FLIP_TOP_BOTTOM).convert(mode='RGBA')
            self.set(content=np.frombuffer(flipped.tobytes(), np.uint8),
                     size=image.size)


class GLTexture:
    def __init__(self, texture=None):
        self._gl_name = None
        self._texture = texture

    def __del__(self):
        self.clear()

    def clear(self):
        if self._gl_name is not None:
            GL.glDeleteTextures(self._gl_name)

    def gen(self):
        if self._gl_name is not None:
            return self._gl_name

        t = self._texture
        if t._size is None:
            raise GLError('Texture size not provided')
        if t._mag_filter.value[1]:
            raise GLError(f'Invalid magnification filter: {t._mag_filter}')

        gl_name = GL.glGenTextures(1)
        self._gl_name = gl_name

        target = GL.GL_TEXTURE_2D
        use_mipmaps = t._min_filter.value[1]

        GL.glBindTexture(target, gl_name)
        GL.glTexParameteri(target, GL.GL_TEXTURE_WRAP_S, t._wrap_s.value)
        GL.glTexParameteri(target, GL.GL_TEXTURE_WRAP_T, t._wrap_t.value)
        GL.glTexParameteri(target, GL.GL_TEXTURE_MIN_FILTER, t._min_filter.value[0])
        GL.glTexParameteri(target, GL.GL_TEXTURE_MAG_FILTER, t._mag_filter.value[0])

        internal_format = GL.GL_RGBA
        data_format = GL.GL_RGBA
        data_type = GL.GL_UNSIGNED_BYTE
        data = t._content
        GL.glTexImage2D(target, 0, internal_format, t._size[0], t._size[1], 0,
                        data_format, data_type, data)

        if use_mipmaps and t._content is not None:
            GL.glGenerateMipmap(target)
        else:
            GL.glTexParameteri(target, GL.GL_TEXTURE_BASE_LEVEL, 0)
            GL.glTexParameteri(target, GL.GL_TEXTURE_MAX_LEVEL, 0)

        return (target, gl_name)
