# Copyright (C) 2021-2022 Matteo Franchin
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
import ctypes
import inspect
import re
import itertools

os.environ['PYOPENGL_PLATFORM'] = 'egl'
import OpenGL
import OpenGL.EGL as EGL
import OpenGL.GL as GL

from pyrtist.lib2d import BBox, View, Point

__all__ = ('FragmentWindow', 'get_line')


def get_line():
    '''Return the line number of the caller in the source.

    This is similar to what __LINE__ does in C.
    Note: the implementation assumes the caller is outside this file.
    '''
    frame = inspect.currentframe()
    if frame is not None:
        prev_filename = inspect.getframeinfo(frame).filename
        frame = frame.f_back
        while (frame is not None and
               inspect.getframeinfo(frame).filename == prev_filename):
            frame = frame.f_back
    return (inspect.getframeinfo(frame).lineno
            if frame is not None else None)

def init_egl(width, height):
    prev_display = os.environ.pop('DISPLAY', None)
    dpy = EGL.eglGetDisplay(EGL.EGL_DEFAULT_DISPLAY)
    if prev_display is not None:
        os.environ['DISPLAY'] = prev_display

    major = ctypes.c_long()
    minor = ctypes.c_long()
    EGL.eglInitialize(dpy, major, minor)

    attrs = EGL.arrays.GLintArray.asArray([
        EGL.EGL_SURFACE_TYPE, EGL.EGL_PBUFFER_BIT,
        EGL.EGL_BLUE_SIZE, 8,
        EGL.EGL_RED_SIZE, 8,
        EGL.EGL_GREEN_SIZE, 8,
        EGL.EGL_DEPTH_SIZE, 24,
        EGL.EGL_COLOR_BUFFER_TYPE, EGL.EGL_RGB_BUFFER,
        EGL.EGL_RENDERABLE_TYPE, EGL.EGL_OPENGL_BIT,
        EGL.EGL_CONFORMANT, EGL.EGL_OPENGL_BIT,
        EGL.EGL_NONE
    ])

    configs = (EGL.EGLConfig*1)()
    num_configs = ctypes.c_long()
    EGL.eglChooseConfig(dpy, attrs, configs, 1, num_configs)

    EGL.eglBindAPI(EGL.EGL_OPENGL_API)

    attrs = [EGL.EGL_WIDTH, width,
             EGL.EGL_HEIGHT, height,
             EGL.EGL_NONE]
    surface = EGL.eglCreatePbufferSurface(dpy, configs[0], attrs)

    attrs = [EGL.EGL_CONTEXT_MAJOR_VERSION, 4,
             EGL.EGL_CONTEXT_MINOR_VERSION, 0,
             EGL.EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL.EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
             EGL.EGL_NONE]
    attrs = [EGL.EGL_NONE]
    ctx = EGL.eglCreateContext(dpy, configs[0], EGL.EGL_NO_CONTEXT, attrs)

    EGL.eglMakeCurrent(dpy, surface, surface, ctx)

    return dpy

def finish_egl(dpy):
    EGL.eglMakeCurrent(dpy, EGL.EGL_NO_SURFACE, EGL.EGL_NO_SURFACE, EGL.EGL_NO_CONTEXT)

def info_to_str(info):
    '''Transform the output of glGetShaderInfoLog and friends to a string.'''
    if isinstance(info, bytes):
        return info.decode('utf-8')
    return info


class ProgramHandler:
    shader_type_from_string = {'fragment': GL.GL_FRAGMENT_SHADER,
                               'vertex': GL.GL_VERTEX_SHADER}

    def __init__(self, *shader_infos):
        self._shader_infos = shader_infos
        self._program = None
        self._shaders = []

    def compile(self):
        self._program = program = GL.glCreateProgram()

        shaders = self._shaders
        assert len(shaders) == 0
        for shader_source_list in self._shader_infos:
            shader_type_str = shader_source_list.shader_type
            shader_type = self.shader_type_from_string[shader_type_str]
            shader_source = shader_source_list.get_sources()

            shader = GL.glCreateShader(shader_type)
            shaders.append(shader)
            GL.glShaderSource(shader, shader_source)
            GL.glCompileShader(shader)
            compile_status = GL.glGetShaderiv(shader, GL.GL_COMPILE_STATUS)
            if compile_status != GL.GL_TRUE:
                idx_map = shader_source_list.get_file_index_map()
                info = info_to_str(GL.glGetShaderInfoLog(shader))
                msg = ('GL {} shader compilation failure:\n{}\n{}'
                       .format(shader_type_str, info, idx_map))
                raise RuntimeError(msg)
            GL.glAttachShader(program, shader)

        GL.glLinkProgram(program)

        GL.glValidateProgram(program)
        validate_status = GL.glGetProgramiv(program, GL.GL_VALIDATE_STATUS)
        if validate_status != GL.GL_TRUE:
            info = info_to_str(GL.glGetProgramInfoLog(program))
            raise RuntimeError('GL program validation failure:\n{}'.format(info))

        link_status = GL.glGetProgramiv(program, GL.GL_LINK_STATUS)
        if link_status != GL.GL_TRUE:
            info = info_to_str(GL.glGetProgramInfoLog(program))
            raise RuntimeError('GL program link failure:\n{}'.format(info))

        while len(shaders) > 0:
            GL.glDeleteShader(shaders.pop())

        return program

    def __del__(self):
        self.clear()

    def clear(self):
        while len(self._shaders) > 0:
            GL.glDeleteShader(self._shaders.pop())
        if self._program is not None:
            GL.glDeleteProgram(self._program)
            self._program = None


class ShaderSourceList:
    def __init__(self, shader_type=None, version=None):
        self.shader_type = shader_type
        self.version = version
        self.sources = []

    def add_source(self, source):
        self.sources.append(source)

    def get_sources(self):
        ret = [s.get_content(file_index=i)
               for i, s in enumerate(self.sources)]
        if self.version is not None:
            ret[0] = '#version {}\n'.format(self.version) + ret[0]
        return ret

    def get_file_index_map(self):
        ret = 'Files correpsonding to indices:'
        for i, source in enumerate(self.sources):
            ret += '\n  {} --> {}'.format(i, source.get_desc())
        return ret


class ShaderSource:
    def __init__(self, content=None, line=None,
                 file_name=None, file_index=None, desc=None):
        self.content = content
        self.line = line
        self.file_name = file_name
        self.file_index = None
        self.desc = desc

    def get_content(self, file_index=None):
        content = self.content
        if content is None:
            return None
        if self.line is not None:
            if file_index is None:
                prepend = '#line {}\n'.format(self.line - 1)
            else:
                prepend = '#line {} {}\n'.format(self.line - 1, file_index)
                self.file_index = file_index
            content = prepend + content
        return content

    def get_desc(self):
        desc = self.desc
        if desc is None:
            return self.file_name or '?'
        if self.file_name is None:
            return desc
        return '{} ({})'.format(desc, file_name)


class FragmentWindow:
    default_vertex_shader = '''
in vec2 vert_pos;

out vec2 FRAG_POSITION;

uniform mat4 transform;

void main() {
  gl_Position = vec4(vert_pos, 0.0, 1.0);
  FRAG_POSITION = (transform * vec4(vert_pos, 0.0, 1.0)).xy;
}
'''

    vec_re = re.compile('[biud]?vec([2-4])')
    mat_re = re.compile('[d]?mat([2-4])(x[2-4])?')

    def __init__(self, p1, p2, position_name='position'):
        assert(len(p1) >= 2)
        assert(len(p2) >= 2)
        self._min_point = tuple(min(p1[i], p2[i]) for i in range(2))
        self._max_point = tuple(max(p1[i], p2[i]) for i in range(2))
        self._frag_defines = {}
        self._vert_defines = {'FRAG_POSITION': position_name}
        self._frag_includes = []
        self._frag_source = None
        self._vert_source = \
            ShaderSource(content=self.default_vertex_shader, line=1,
                         desc='Pyrtist default vertex shader')

    def include(self, source_file_name):
        '''Include GLSL files from source.

        Args:
          source_file_name: full path to the source file.
        '''
        with open(source_file_name, 'r') as f:
            content = f.read()
        src = ShaderSource(content=content, file_name=source_file_name, line=1)
        self._frag_includes.append(src)

    @classmethod
    def _stringify(cls, v, t):
        if t is None:
            return str(v)

        match = cls.vec_re.fullmatch(t)
        if match is not None:
            n = int(match.group(1))
            return '{}({})'.format(t, ', '.join(str(v[i]) for i in range(n)))

        match = cls.mat_re.fullmatch(t)
        if match is not None:
            n = int(match.group(1))
            g2 = match.group(2)
            m = (n if g2 is None else int(g2[1:]))
            cols = tuple('vec{}({})'.format(m, ', '.join(str(v[j][i])
                                                         for j in range(m)))
                         for i in range(n))
            return '{}({})'.format(t, ', '.join(cols))

        raise ValueError('Invalid GLSL type {} for define'.format(t))

    def define(self, name, value, type=None):
        '''Define a macro to use in the GLSL source.

        Args:
          name:
            Name of the macro being defined.
          value:
            Value of the macro.
          type:
            GLSL type of the value. This can be any vector or matrix type.
            If this argument is provided, value can be a Python type (e.g.
            a tuple) and it is converted automatically to the desired type.

        Return:
          The final string value assigned to the macro.
        '''
        value_as_str = self._stringify(value, type)
        self._frag_defines[name] = value_as_str
        return value_as_str

    def set_vars(self, variables, **kwargs):
        '''Set variables from the given dictionary.

        Note that variables of unhandled type are simply ignored.

        Args:
          variables:
            Dictionary of variables to set. Only variables of type
            pyrtist.lib2d.Point are currently handled. All other values
            in the dictionary are ignored.
          kwargs:
            Other variables to set from keyword arguments.
        '''

        # Add defines for reference points.
        for name, value in itertools.chain(variables.items(), kwargs):
            if isinstance(value, Point):
                self._frag_defines[name] = \
                    'vec2({}, {})'.format(value[0], value[1])

    def set_source(self, source, line=True):
        '''Set the source of the fragment shader.

        If this function is called more than once, the last call
        overrides previous calls.

        Args:
          source:
            Source of the shader.
          line:
            An integer indicating the position of the source in the file.
            This is used to generate a #line directive in GLSL so that messages
            from the GLSL compiler are reported with useful line numbers.
            This can also be set to None to disable generation of the #line
            directive. Alternatively, if this is set to True, the directive
            uses the line number where this method is called in Python.
            For example:

              # The following usage works well, as the string containing the
              # GLSL source lies inside the same line where the set_source
              # method is called.
              w.set_source(""" ... """)

              # The usage below does not work.
              source = """ ... """
              w.set_source(source)

              # Do this instead.
              line, source = get_line(), """ ... """
              w.set_source(source, line=line)
        '''
        if line is True:
            line = get_line()
        if line not in (None, False):
            assert isinstance(line, int), \
                'line argument must be one of: None, boolean or int'
        else:
            line = None
        self._frag_source = ShaderSource(content=source, line=line,
                                         desc='Main fragment shader source')

    def __lshift__(self, op):
        self.set_source(op)

    def get_vert_sources(self):
        sources = ShaderSourceList(shader_type='vertex', version='150 core')
        definitions = ['#define {} {}'.format(k, v)
                       for k, v in self._vert_defines.items()]
        defs_source = ('\n'.join(definitions) + '\n')
        src = ShaderSource(content=defs_source,
                           desc='Vertex shader preprocessor definitions')
        sources.add_source(src)
        sources.add_source(self._vert_source)
        return sources

    def get_frag_sources(self):
        sources = ShaderSourceList(shader_type='fragment', version='150 core')

        # First: add one source containing #define directives for refpoints.
        defs_lines = ['#define {} {}'.format(k, v)
                      for k, v in self._frag_defines.items()]
        src = ShaderSource(content='\n'.join(defs_lines) + '\n',
                           desc='Pyrtist internal preprocessor definitions')
        sources.add_source(src)

        # Next: add all the included sources.
        for src in self._frag_includes:
            sources.add_source(src)

        # Finally: add the main source.
        sources.add_source(self._frag_source)
        return sources

    def draw_with_transform(self, transform):
        program_handler = ProgramHandler(self.get_vert_sources(),
                                         self.get_frag_sources())
        program = program_handler.compile()

        GL.glClearColor(0.0, 0.0, 0.0, 0.0)
        GL.glClearDepth(1.0)
        GL.glDepthFunc(GL.GL_LESS)
        GL.glEnable(GL.GL_DEPTH_TEST)
        GL.glShadeModel(GL.GL_SMOOTH)

        GL.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT)

        GL.glUseProgram(program)

        do_transpose = OpenGL.GL.GL_TRUE
        uniform_loc = GL.glGetUniformLocation(program, 'transform')
        GL.glUniformMatrix4fv(uniform_loc, 1, do_transpose, transform)

        GL.glBegin(GL.GL_QUADS)
        GL.glVertex2f(-1.0,  1.0)
        GL.glVertex2f( 1.0,  1.0)
        GL.glVertex2f( 1.0, -1.0)
        GL.glVertex2f(-1.0, -1.0)
        GL.glEnd()

    def _draw_view(self, ref_points, size_in_pixels,
                   origin=None, size=None, pixel_format='RGB'):
        pix_width, pix_height = size_in_pixels
        gl_format_from_string = {'RGB': GL.GL_RGB,
                                 'RGBA': GL.GL_RGBA}
        gl_format = gl_format_from_string.get(pixel_format)
        if gl_format is None:
            raise RuntimeError('Invalid image format {!r}'
                               .format(pixel_format))

        transform = [1.0, 0.0, 0.0, 0.0,
                     0.0,-1.0, 0.0, 0.0,
                     0.0, 0.0, 1.0, 0.0,
                     0.0, 0.0, 0.0, 1.0]

        if origin is None:
            assert size is None, 'origin argument is missing'
            win_aspect = pix_width / pix_height
            mn, mx = self._min_point, self._max_point
            bb_size = (mx[0] - mn[0], mx[1] - mn[1])
            bb_aspect = bb_size[0] / bb_size[1]
            bb_center = (0.5 * (mn[0] + mx[0]), 0.5 * (mn[1] + mx[1]))
            if win_aspect >= bb_aspect:
                s = win_aspect / bb_aspect
                origin = ((mn[0] - bb_center[0]) * s + bb_center[0], mn[1])
                size = (bb_size[0] * s, bb_size[1])
            else:
                s = bb_aspect / win_aspect
                origin = (mn[0], (mn[1] - bb_center[1]) * s + bb_center[1])
                size = (bb_size[0], bb_size[1] * s)

            view = View(BBox(mn, mx), origin, size)
        else:
            assert size is not None, 'size argument is missing'
            view = View(BBox(self._min_point, self._max_point), origin, size)

        transform[0] = 0.5 * size[0]
        transform[3] = origin[0] + transform[0]
        transform[5] = -0.5 * size[1]
        transform[7] = origin[1] - transform[5]

        # Add defines for reference points.
        for name, rp in ref_points.items():
            self._frag_defines[name] = 'vec2({}, {})'.format(rp[0], rp[1])

        dpy = init_egl(pix_width, pix_height)
        self.draw_with_transform(transform)
        data = GL.glReadPixels(0, 0, pix_width, pix_height, gl_format,
                               GL.GL_UNSIGNED_BYTE)
        finish_egl(dpy)

        return (view, data)

    def draw(self, gui):
        if gui._size is None:
            return
        pix_width, pix_height = gui._size

        ref_points = {}
        gui.update_vars(ref_points)
        ref_points.pop('gui')

        if gui._full_view:
            view, data = self._draw_view(ref_points, gui._size)
        else:
            ox, oy, sx, sy = gui._zoom_window
            origin = (ox, oy)
            size = (sx, sy)
            view, data = self._draw_view(ref_points, gui._size, origin, size)

        # This is a bit of a hack for now.
        gui._tx_cmd_image_info(view)
        args = (pix_width * 3, pix_width, pix_height, data)
        gui._gui_tx_pipe.send(('image_data', args))

    def save(self, file_name, image_format=None, width=None, height=None,
             pixel_format='RGBA', **image_format_params):
        '''Save the shader output to an image file.

        Args:
          file_name:
            Name of the image file to save to.
          image_format:
            Image format to use (e.g. '.png'.) If None, the image format is
            deduced from the extension of the file name.
          width:
            Width of the rendered output
          height:
            Height of the rendered output
          pixel_format:
            Format of the rendered output
          image_format_params:
            Parameters passed to the image writer. See PIL.Image.Image.save
        '''

        mn, mx = self._min_point, self._max_point
        size = (mx[0] - mn[0], mx[1] - mn[1])

        no_width = width is None
        no_height = height is None
        if no_width and no_height:
            width = 800
            no_width = False

        if no_width:
            width = size[0] * height / size[1]
        elif no_height:
            height = size[1] * width / size[0]

        size_in_pixels = (int(width), int(height))
        _, data = self._draw_view({}, size_in_pixels,
                                  pixel_format=pixel_format,
                                  origin=mn, size=size)
        from PIL import Image
        image = Image.frombytes(pixel_format, size_in_pixels, data,
                                decoder_name='raw')
        image.save(file_name, image_format, **image_format_params)
