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

__all__ = ('DeepWindow',)

import os

from ..lib2d import Point, Color, BBox, Window, Hot, View
from ..lib2d.window import WindowBase
from ..lib2d.base import combination
from .core_types import Point3
from .cmd_stream import Cmd, CmdStream
from .cmd_exec import CmdExecutor


class DeepWindowRenderer(object):
    def __init__(self, deep_window, mode):
        self.deep_window = deep_window
        self.mode = mode

    def draw_view(self, target_surface, view):
        dw = self.deep_window
        if dw.cmd_executor is not None:
            raise ValueError('Cannot render a window with a custom executor')

        bb = BBox(dw)
        b_min = bb.min_point
        b_max = bb.max_point

        wx = target_surface.get_width()
        wy = target_surface.get_height()
        deep_surface = CmdExecutor.create_surface(wx, wy)

        if view is None:
            old_size = bb.max_point - bb.min_point
            r = min(wx/float(old_size.x), wy/float(old_size.y))
            size = Point(wx/r, wy/r)
            origin = b_min - (size - old_size)*0.5
        else:
            origin = view.origin
            size = view.size

        ce = CmdExecutor.for_surface(deep_surface, bot_left=origin,
                                     top_right=origin + size)
        tmp = DeepWindow(ce)
        #tmp.Rectangle(b_min, b_max, Color.white)
        tmp.take(dw)

        if self.mode == 'depth':
            db = deep_surface.get_dst_depth_buffer()
            db = db.compute_normals()
        else:
            db = deep_surface.get_dst_image_buffer()

        src_data = db.get_data()
        dst_data = target_surface.get_data()
        dst_data[:] = src_data[:]
        return View(bb, origin, size)

    def draw_full_view(self, target_surface):
        return self.draw_view(target_surface, None)
    draw_full_view.__doc__ = Window.draw_full_view.__doc__

    def draw_zoomed_view(self, target_surface, view):
        return self.draw_view(target_surface, view)
    draw_zoomed_view.__doc__ = Window.draw_zoomed_view.__doc__


class DeepWindow(WindowBase):
    def __init__(self, cmd_executor=None):
        super(DeepWindow, self).__init__(CmdStream(), cmd_executor)

    def depth(self):
        '''Return the normals view so that it can be rendered in the GUI.'''
        return DeepWindowRenderer(self, 'depth')

    def draw_full_view(self, target_surface):
        return DeepWindowRenderer(self, 'real').draw_full_view(target_surface)
    draw_full_view.__doc__ = Window.draw_full_view.__doc__

    def draw_zoomed_view(self, target_surface, view):
        renderer = DeepWindowRenderer(self, 'real')
        return renderer.draw_zoomed_view(target_surface, view)
    draw_zoomed_view.__doc__ = Window.draw_zoomed_view.__doc__

    def save(self, real_file_name, depth_file_name=None, resolution=None):
        '''Save the real and depth parts of the image, computing the visible
        area automatically.
        '''
        # Construct the depth file name, if not given.
        if depth_file_name is None:
            lhs, rhs = os.path.splitext(real_file_name)
            depth_file_name = lhs + '-depth' + rhs

        # Get the z component of the resolution.
        z_resolution = None
        if isinstance(resolution, Point3):
            z_resolution = resolution.z
            resolution = resolution.get_xy()

        bbox, _, size, _ = \
          self._check_for_save(None, None, resolution, None, None)
        surface = CmdExecutor.create_surface(size[0], size[1])
        cmd_exec = CmdExecutor.for_surface(surface, bot_left=bbox.min_point,
                                           top_right=bbox.max_point,
                                           z_scale=z_resolution)
        cmd_exec.execute(self.cmd_stream)
        cmd_exec.save(real_file_name, depth_file_name)


@combination(CmdStream, DeepWindow)
def cmd_stream_at_deep_window(cmd_stream, deep_window):
    deep_window.cmd_stream(cmd_stream)
    deep_window._consume_cmds()

@combination(DeepWindow, BBox)
def deep_window_at_bbox(deep_window, bbox):
    for cmd in deep_window.cmd_stream:
        args = cmd.get_args()
        for arg in args:
            if isinstance(arg, (Point, Window)):
                bbox.take(arg)

@combination(DeepWindow, DeepWindow)
def deep_window_at_deep_window(child, parent):
    cmd_exec = parent.cmd_executor
    if cmd_exec is not None:
        cmd_exec.execute(child.cmd_stream)
    else:
        parent.cmd_stream.take(child.cmd_stream)

@combination(BBox, DeepWindow, 'BBox')
def bbox_at_deep_window(bbox, deep_window):
    if not bbox:
        return
    deep_window(CmdStream(Cmd(Cmd.set_bbox, bbox.min_point, bbox.max_point)))

@combination(Hot, DeepWindow, 'Hot')
def hot_at_deep_window(hot, deep_window):
    deep_window.hot_points[hot.name] = hot.point
