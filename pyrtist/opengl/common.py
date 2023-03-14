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

__all__ = ('GLError', 'check_error', 'set_optional')

class GLError(Exception):
    pass


def check_error():
    gl_err = GL.glGetError()
    if gl_err != GL.GL_NO_ERROR:
        raise GLError(f'GL error {gl_err}')

def set_optional(obj, **kwargs):
    for name, value in kwargs.items():
        if not hasattr(obj, name) or value is not None:
            setattr(obj, name, value)
