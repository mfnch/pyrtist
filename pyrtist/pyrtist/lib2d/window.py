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

__all__ = ('Hot', 'Window')

import numbers

from .base import Taker, combination
from .cmd_stream import Cmd, CmdStream
from .style import Style, StrokeStyle, Color, Stroke, Fill
from .cairo_cmd_exec import CairoCmdExecutor
from .bbox import BBox
from .core_types import Point, View
from .pattern import Pattern


class Hot(object):
    def __init__(self, name, point):
        self.name = name
        self.point = point


class WindowBase(Taker):
    default_resolution = 10.0

    def __init__(self, cmd_stream, cmd_executor):
        self.cmd_stream = cmd_stream
        self.cmd_executor = cmd_executor
        self.hot_points = {}
        super(WindowBase, self).__init__()

    def __getitem__(self, name):
        return self.hot_points[name]

    def get(self, name, default=None):
        return self.hot_points.get(name, default)

    def _consume_cmds(self):
        cmd_exec = self.cmd_executor
        if cmd_exec is not None:
            self.cmd_stream = cmd_exec.execute(self.cmd_stream)

    def _check_for_save(self, *args):
        mode, size, resolution, executor, bg_color = args
        if self.cmd_executor is not None:
            if any(arg is not None for arg in args):
                raise ValueError('No arguments should be given when saving '
                                 'a window which has a target')
            self.cmd_executor.save(file_name)
            return

        if size is not None and resolution is not None:
            raise ValueError("Only one of `size' and `resolution' arguments "
                             "should be given to the save method")
        # Compute the bounding box.
        bbox = BBox(self)

        if size is None:
            if resolution is None:
                resolution = self.default_resolution
            resolution = (Point((resolution, resolution))
                          if isinstance(resolution, numbers.Number)
                          else Point(resolution))
            if resolution.x <= 0 or resolution.y <= 0:
                raise ValueError('Negative resolution')
            view = bbox.max_point - bbox.min_point
            size = (int(view.x*resolution.x), int(view.y*resolution.y))

        return (bbox, mode, size, executor)


class Window(WindowBase):
    '''Window is the object which receives the drawing commands.

    It can either send these commands to a drawer or record them and send them
    later via the Save method.
    '''

    default_executor = CairoCmdExecutor
    default_mode = 'rgb24'
    default_bg_color = Color.white

    def __init__(self, *args):
        '''Create a new Window.'''

        cmd_executor = None
        cmd_executors = [arg for arg in args
                         if isinstance(arg, CairoCmdExecutor)]
        if len(cmd_executors) == 1:
            cmd_executor = cmd_executors[0]
            args = tuple(arg for arg in args
                         if not isinstance(arg, CairoCmdExecutor))
        elif len(cmd_executors) > 1:
            raise ValueError('Window takes at most one command executor')

        super(Window, self).__init__(CmdStream(), cmd_executor)
        self.take(*args)

    def draw(self, mode=None, size=None, resolution=None, executor=None,
             bg_color=None):
        '''Draw the window to a Cairo surface.

        The usage of this method is similar to the usage of the save method,
        except that this method does not save the image to a file. Instead, it
        draws the image to a new Cairo surface and then returns it.
        '''
        bbox, mode, size, executor = \
          self._check_for_save(mode, size, resolution, executor, bg_color)
        if bg_color is None:
            bg_color = (self.default_bg_color if mode == 'rgb24'
                        else Color(1, 1, 1, 0))
        ex = executor or self.default_executor
        mode = mode or self.default_mode
        surface = ex.create_image_surface(mode, size[0], size[1])
        cmd_exec = ex.for_surface(surface, bot_left=bbox.min_point,
                                  top_right=bbox.max_point, bg_color=bg_color)
        cmd_exec.execute(self.cmd_stream)
        return cmd_exec.surface

    def save(self, file_name, mode=None, size=None, resolution=None,
             executor=None, bg_color=None):
        '''Save the window to file, computing the visible area automatically.

        The user should provide either the resolution or the size in pixels.
        The visible region will be automatically determined by computing the
        bounding box of the window. Use BBox() to set explicitly the bounding
        box and override the automatic computation.
        '''
        cairo_surface = self.draw(mode=mode, size=size, resolution=resolution,
                                  executor=executor, bg_color=bg_color)
        cairo_surface.write_to_png(file_name)

    def draw_view(self, target_surface, view):
        '''Run draw_full_view() when view is None and draw_zoomed_view()
        when view is not None.
        '''
        # Rendering a bitmap window is not yet supported.
        if self.cmd_executor is not None:
            raise ValueError('Cannot render a window with a custom executor')

        bb = BBox(self)
        b_min = bb.min_point
        b_max = bb.max_point
        wx = target_surface.get_width()
        wy = target_surface.get_height()

        if view is None:
            old_size = bb.max_point - bb.min_point
            r = min(wx/float(old_size.x), wy/float(old_size.y))
            size = Point(wx/r, wy/r)
            origin = b_min - (size - old_size)*0.5
        else:
            size = view.size
            origin = view.origin

        ce = CairoCmdExecutor.for_surface(target_surface, bot_left=origin,
                                          top_right=origin + size,
                                          bg_color=Color.grey)
        tmp = Window(ce)
        tmp.Rectangle(b_min, b_max, Color.white)
        tmp.take(Color.black, self)
        return View(bb, origin, size)

    def draw_full_view(self, target_surface):
        '''Method used by the GUI to render a full view of the window.

        Args:
           target_surface (cairo.ImageSurface): where the view is rendered.

        Returns:
           A View() object containing details about the rendered view.
           This is used to allow zooming in/out etc.
        '''
        return self.draw_view(target_surface, None)

    def draw_zoomed_view(self, target_surface, view):
        '''Method used by the GUI to render a zoomed view of the window.

        Args:
           target_surface (cairo.ImageSurface): where the view is rendered.
           view (lib2d.View): the view to render.

        Returns:
           A View() object containing details about the rendered view.
        '''
        return self.draw_view(target_surface, view)


@combination(Window, BBox)
def fn(window, bbox):
    bbox.take(window.cmd_stream)

@combination(BBox, Window, 'BBox')
def fn(bbox, window):
    if bbox:
        cmd = Cmd(Cmd.set_bbox, bbox.min_point, bbox.max_point)
        window.take(CmdStream(cmd))

@combination(Pattern, Window)
@combination(Stroke, Window)
@combination(Fill, Window)
def item_at_window(item, window):
    window.take(CmdStream(item))

@combination(Style, Window)
def style_at_window(style, window):
    s = Style(style)
    s.make_default = True
    s.stroke_style.make_default = True
    window.take(CmdStream(s))

@combination(StrokeStyle, Window)
def stroke_style_at_window(stroke_style, window):
    ss = StrokeStyle(stroke_style)
    ss.make_default = True
    window.take(CmdStream(ss))

@combination(Hot, Window, 'Hot')
def fn(hot, window):
    window.hot_points[hot.name] = hot.point


@combination(CmdStream, Window)
def fn(cmd_stream, window):
    window.cmd_stream(cmd_stream)
    window._consume_cmds()

@combination(Window, Window)
def fn(child, parent):
    cmd_exec = parent.cmd_executor
    if cmd_exec is not None:
        cmd_exec.execute(child.cmd_stream)
    else:
        parent.cmd_stream.take(child.cmd_stream)
