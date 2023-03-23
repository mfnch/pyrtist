# Copyright (C) 2021-2023 Matteo Franchin
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
import PIL.Image

from pyrtist.lib2d import BBox, View, Point
from .texture import Texture, GLTexture
from .common import GLError

__all__ = ('FragmentWindow', 'get_line')


def get_line():
    '''Return the line number of the caller of this function.

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
        EGL.EGL_ALPHA_SIZE, 8,
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


class UniformState:
    def __init__(self, reserved_names={}):
        self._next_texture_unit = 0
        self._reserved_names = reserved_names
        self._handlers = {
            int: self._set_uniform_int,
            float: self._set_uniform_float,
            tuple: self._set_uniform_tuple,
            list: self._set_uniform_tuple,
            Texture: self._set_uniform_texture
        }

    def reserve_texture_unit(self):
        ret = self._next_texture_unit
        self._next_texture_unit += 1
        return ret

    def set_uniforms(self, program, uniforms):
        not_found = set()
        for name, value in uniforms.items():
            if name in self._reserved_names:
                raise GLError(f'Uniform name "{name}" is reserved. '
                              'Please choose a different name for this uniform')
            location = GL.glGetUniformLocation(program, name)
            if location == -1:
                not_found.add(name)
                continue
            setter = self._handlers.get(type(value), None)
            if setter is None:
                # Try matching each type, in case value's type is a derived type.
                for handler_type, handler in self._handlers.items():
                    if isinstance(value, handler_type):
                        setter = handler
                        break
                if setter is None:
                    raise GLError(f'Invalid value for set_uniform: {value}')
            setter(location, value)
        return not_found

    def _set_uniform_int(self, location, value):
        GL.glUniform1i(location, value)

    def _set_uniform_float(self, location, value):
        GL.glUniform1f(location, value)

    def _set_uniform_tuple(self, location, value):
        n = len(value)
        if n < 1:
            raise GLError('Empty tuple/list to set_uniform')
        types = set(type(v) for vi in value)
        if float in types:
            setters = (GL.glUniform1f, GL.glUniform2f, GL.glUniform3f, GL.glUniform4f)
        elif int in types:
            setters = (GL.glUniform1i, GL.glUniform2i, GL.glUniform3i, GL.glUniform4i)
        else:
            raise GLError(f'Invalid value for set_uniform: {value}')
        if n > len(setters):
            raise GLError(f'Too many componets in tuple/list for set_uniform: {value}')
        setters[n - 1](location, *value)

    def _set_uniform_texture(self, location, value):
        texture_unit = self.reserve_texture_unit()
        GL.glUniform1i(location, texture_unit)
        GL.glActiveTexture(GL.GL_TEXTURE0 + texture_unit)
        if value._impl is None:
            value._impl = GLTexture(value)
        target, gl_name = value._impl.gen()
        GL.glBindTexture(target, gl_name)


class FragmentWindow:
    '''
    Window that uses a fragment shader (in OpenGL's GLSL language) for drawing.

    This window aims to provide functionality similar to what provided by
    shadertoy.com
    '''

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

    # Ids for special macros.
    (MIN_POINT,
     MAX_POINT,
     CENTER_POINT,
     BB_SIZE,
     RESOLUTION,
     PIXEL_SIZE)  = range(6)

    def __init__(self, p1, p2, position_name='position',
                 pixel_size='pixel_size', resolution='resolution',
                 min_point=None, max_point=None, center_point=None,
                 bb_size=None, allow_unset_uniforms=True,
                 version='150 core'):
        '''
        Create a new FragmentWindow.

        Args:
          p1 : tuple[float, float]
            One corner of the window's bounding box
          p2 : tuple[float, float]
            Another corner of the window's bounding box
          position_name : str, default='position'
            Name of the fragment position in the GLSL source
          pixel_size : str, default='pixel_size'
            Macro defining a vec2 with the size of a pixel in Pyrtist
            coordinates, i.e. the same used for `p1` and `p2`. See Note
          resolution : str, default='resolution'
            Macro defining a vec2 with the width and height of the window in
            pixels. See Note
          min_point : tuple[float, float], default=None
            The corner of the bounding box having minimum coordinates. See Note
          max_point : tuple[float, float], default=None
            The corner of the bounding box having maximum coordinates. See Note
          center_point : tuple[float, float], default=None
            Mid-point between `min_point` and `max_point`. See Note
          bb_size : tuple[float, float], default=None
            Size computed as `max_point` - `min_point`. See Note
          allow_unset_uniforms : bool, default=True
            Whether to tolerate failures setting uniforms specified via the
            set_uniforms method. Note that the shader compiler may optimize
            away variables unused in the shader, leading to unexpected errors.
            For this reason, `allow_unset_uniform` is set to True by default.
          version : str, default '150 core'
            GLSL version to use in the top #version line in the fragment shader

        Note:
          `pixel_size`, `resolution`, `min_point`, `max_point`, `center_point`,
          `bb_size` are names of special macros that should be defined in the
          shader. They can be set to None to indicate that they are not used in
          the shader and therefore should not be defined.
        '''
        assert(len(p1) >= 2)
        assert(len(p2) >= 2)
        self._min_point = tuple(min(p1[i], p2[i]) for i in range(2))
        self._max_point = tuple(max(p1[i], p2[i]) for i in range(2))
        self._frag_defines = {}
        self._vert_defines = {'FRAG_POSITION': position_name}
        self._uniforms = {}
        self._allow_unset_uniforms = allow_unset_uniforms
        self._frag_includes = []
        self._frag_specials = {
            self.MIN_POINT: min_point,
            self.MAX_POINT: max_point,
            self.CENTER_POINT: center_point,
            self.BB_SIZE: bb_size,
            self.RESOLUTION: resolution,
            self.PIXEL_SIZE: pixel_size,
        }
        self._frag_version = version
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

    def set_uniforms(self, **kwargs):
        '''Set the value of a uniform in the shader.

        Attributes given as None are not set. Default values above are those
        created at construction of this object.

        Args:
          kwargs : dict[str, object]
            Dictionary of {name: value} pairs where `name` is the name of the
            uniform variable in the shader and `value` is the Python value to
            set. At the moment, the Python object is used to deduce the type
            of the uniform. For example:

              w.set_uniform(count=1)   # ends up calling glUniform1i
              w.set_uniform(x=1.234)   # calls glUniform1f
              w.set_uniform(xy=(1,1)   # calls glUniform2i
              w.set_uniform(xy=(1,2.0) # calls glUniform2f

            This method can also be used to set textures. For example:

              tex = Texture()
              tex.set_content_from_file('texture.png')
              tex.set(wrap_x=Texture.Wrap.CLAMP_TO_EDGE,
                      wrap_y=Texture.Wrap.CLAMP_TO_EDGE,
                      mag_filter=Texture.Filter.LINEAR,
                      min_filter=Texture.Filter.NEAREST)
              w.set_uniforms(image=tex)

            The shader could declare `image` as follows:

              uniform sampler2D image;

            Note that the image is always converted to a RGBA texture and uses
            RGBA as the internal format.
        '''
        self._uniforms.update(kwargs)

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
        sources = ShaderSourceList(shader_type='fragment',
                                   version=self._frag_version)

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

        reserved_names = {'transform'}
        uniform_state = UniformState(reserved_names)
        unset_uniforms = uniform_state.set_uniforms(program, self._uniforms)
        if len(unset_uniforms) > 0 and not self._allow_unset_uniforms:
            raise GLError(f'Uniforms not found: {", ".join(unset_uniforms)}')

        do_transpose = OpenGL.GL.GL_TRUE
        uniform_loc = GL.glGetUniformLocation(program, 'transform')
        GL.glUniformMatrix4fv(uniform_loc, 1, do_transpose, transform)

        # TODO: avoid immediate mode.
        GL.glBegin(GL.GL_QUADS)
        GL.glVertex2f(-1.0,  1.0)
        GL.glVertex2f( 1.0,  1.0)
        GL.glVertex2f( 1.0, -1.0)
        GL.glVertex2f(-1.0, -1.0)
        GL.glEnd()

    def _set_specials(self, size_in_pixels, view_size):
        max_p = self._max_point
        min_p = self._min_point
        center = tuple((max_p[i] + min_p[i]) * 0.5 for i in range(2))
        bb_size = tuple(max_p[i] - min_p[i] for i in range(2))
        pix_size = tuple(view_size[i] / size_in_pixels[i] for i in range(2))
        special_values = {
            self.RESOLUTION: 'vec3({}, {}, 1.0)'.format(*size_in_pixels),
            self.MIN_POINT: 'vec2({}, {})'.format(*min_p),
            self.MAX_POINT: 'vec2({}, {})'.format(*max_p),
            self.CENTER_POINT: 'vec2({}, {})'.format(*center),
            self.BB_SIZE: 'vec2({}, {})'.format(*bb_size),
            self.PIXEL_SIZE: 'vec2({}, {})'.format(*pix_size),
        }
        for internal_name, name in self._frag_specials.items():
            if name is not None:
                value = special_values.get(internal_name)
                if value is not None:
                    self._frag_defines[name] = value

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
            if isinstance(rp, Point):
                self._frag_defines[name] = 'vec2({}, {})'.format(rp[0], rp[1])
        self._set_specials(size_in_pixels, size)

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
        image = PIL.Image.frombytes(pixel_format, size_in_pixels, data,
                                    decoder_name='raw')
        image.save(file_name, image_format, **image_format_params)
