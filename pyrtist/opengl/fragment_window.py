# Copyright (C) 2021 Matteo Franchin
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

os.environ['PYOPENGL_PLATFORM'] = 'egl'
import OpenGL
import OpenGL.EGL as EGL
import OpenGL.GL as GL

from pyrtist.lib2d import BBox, View

__all__ = ('FragmentWindow', 'get_line')


def get_line():
    '''Return the line number of the caller in the source.

    This is similar to what __LINE__ does in C.
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

def vertex_shader(*source):
    return (GL.GL_VERTEX_SHADER, source)

def fragment_shader(*source):
    return (GL.GL_FRAGMENT_SHADER, source)


class ProgramHandler:
    def __init__(self, *shader_infos):
        self.shader_infos = shader_infos
        self.program = None
        self.shaders = []

    def compile(self):
        self.program = program = GL.glCreateProgram()

        shaders = self.shaders
        assert len(shaders) == 0
        for shader_type, shader_source in self.shader_infos:
            shader = GL.glCreateShader(shader_type)
            shaders.append(shader)
            GL.glShaderSource(shader, shader_source)
            GL.glCompileShader(shader)
            compile_status = GL.glGetShaderiv(shader, GL.GL_COMPILE_STATUS)
            if compile_status != GL.GL_TRUE:
                info = GL.glGetShaderInfoLog(shader).decode('utf-8')
                raise RuntimeError('GL shader compilation failure:\n{}'.format(info))
            GL.glAttachShader(program, shader)

        GL.glLinkProgram(program)

        GL.glValidateProgram(program)
        validate_status = GL.glGetProgramiv(program, GL.GL_VALIDATE_STATUS)
        if validate_status != GL.GL_TRUE:
            info = GL.glGetProgramInfoLog(program).decode('utf-8')
            raise RuntimeError('GL program validation failure:\n{}'.format(info))

        link_status = GL.glGetProgramiv(program, GL.GL_LINK_STATUS)
        if link_status != GL.GL_TRUE:
            info = GL.glGetProgramInfoLog(program).decode('utf-8')
            raise RuntimeError('GL program link failure:\n{}'.format(info))

        while len(shaders) > 0:
            GL.glDeleteShader(shaders.pop())

        return program

    def __del__(self):
        self.clear()

    def clear(self):
        while len(self.shaders) > 0:
            GL.glDeleteShader(self.shaders.pop())
        if self.program is not None:
            GL.glDeleteProgram(self.program)


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

    def __init__(self, p1, p2, position_name='position'):
        assert(len(p1) >= 2)
        assert(len(p2) >= 2)
        self.min_point = tuple(min(p1[i], p2[i]) for i in range(2))
        self.max_point = tuple(max(p1[i], p2[i]) for i in range(2))
        self.frag_defines = {}
        self.vert_defines = {'FRAG_POSITION': position_name}
        self.frag_source = None
        self.frag_source_line = None
        self.vert_source = self.default_vertex_shader
        self.program_handler = None

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
        self.frag_source_line = line
        self.frag_source = source

    def __lshift__(self, op):
        self.set_source(op)

    def get_vert_sources(self):
        definitions = ['#define {} {}'.format(k, v)
                       for k, v in self.vert_defines.items()]
        defs_source = ('#version 150 core\n' + '\n'.join(definitions) + '\n')
        return vertex_shader(defs_source, self.vert_source)

    def get_frag_sources(self):
        extra_lines = ['#version 150 core']

        extra_lines.extend('#define {} {}'.format(k, v)
                           for k, v in self.frag_defines.items())

        line = self.frag_source_line
        if line is not None:
            extra_lines.append('#line {}'.format(line - 1))

        defs_source = ('\n'.join(extra_lines) + '\n')
        return fragment_shader(defs_source, self.frag_source)

    def draw_with_transform(self, transform):
        self.program_handler = ProgramHandler(self.get_vert_sources(),
                                              self.get_frag_sources())
        program = self.program_handler.compile()

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

    def draw(self, gui):
        if gui._size is None:
            return

        pix_width, pix_height = gui._size

        transform = [1.0, 0.0, 0.0, 0.0,
                     0.0,-1.0, 0.0, 0.0,
                     0.0, 0.0, 1.0, 0.0,
                     0.0, 0.0, 0.0, 1.0]

        if gui._full_view:
            win_aspect = pix_width / pix_height
            mn, mx = self.min_point, self.max_point
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
            ox, oy, sx, sy = gui._zoom_window
            origin = (ox, oy)
            size = (sx, sy)

            view = View(BBox(self.min_point, self.max_point), origin, size)

        transform[0] = 0.5 * size[0]
        transform[3] = origin[0] + transform[0]
        transform[5] = -0.5 * size[1]
        transform[7] = origin[1] - transform[5]

        # Add defines for reference points.
        ref_points = {}
        gui.update_vars(ref_points)
        ref_points.pop('gui')
        for name, rp in ref_points.items():
            self.frag_defines[name] = 'vec2({}, {})'.format(rp[0], rp[1])

        dpy = init_egl(pix_width, pix_height)
        self.draw_with_transform(transform)
        data = GL.glReadPixels(0, 0, pix_width, pix_height, GL.GL_RGB, GL.GL_UNSIGNED_BYTE)
        finish_egl(dpy)

        # This is a bit of a hack for now.
        gui._tx_cmd_image_info(view)
        args = (pix_width * 3, pix_width, pix_height, data)
        gui._gui_tx_pipe.send(('image_data', args))
