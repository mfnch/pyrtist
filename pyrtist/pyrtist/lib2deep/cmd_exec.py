__all__ = ('CmdExecutor',)

import cairo

from ..lib2d import CairoCmdExecutor, Window, BBox, Axes
from .core_types import Point3, Point, Z
from .deep_surface import DeepSurface
from .cmd_stream import Cmd


class CmdExecutor(object):
    draw_cmd_names = set(Cmd.draw_cmd_names)

    @staticmethod
    def create_surface(width, height):
        return DeepSurface(width, height)

    @staticmethod
    def for_surface(deep_surface, top_left=None, bot_right=None,
                    top_right=None, bot_left=None, z_origin=0.0,
                    z_scale=None):
        if ((top_left is None) != (bot_right is None) or
            (top_right is None) != (bot_left is None)):
            raise TypeError('Only opposing corners should be set')

        if bot_left is None:
            if top_left is None:
                bot_left = Point(0, 0)
                top_right = Point(deep_surface.get_width(),
                                  deep_surface.get_height())
            else:
                top_left, bot_right = Point(top_left), Point(bot_right)
        else:
            if top_left is not None:
                raise TypeError('Only opposing corners should be set')
            bot_left, top_right = Point(bot_left), Point(top_right)

        if top_left is None:
            top_left = Point(bot_left.x, top_right.y)
            bot_right = Point(top_right.x, bot_left.y)

        diag = bot_right - top_left
        scale = Point(deep_surface.get_width()/diag.x,
                      deep_surface.get_height()/diag.y)
        z_scale = z_scale or (abs(scale.x) + abs(scale.y))*0.5
        return CmdExecutor(deep_surface, top_left, scale, z_origin, z_scale)

    def __init__(self, deep_surface, origin, scale, z_origin=0.0, z_scale=1.0):
        self.deep_surface = ds = deep_surface
        self.origin = Point(origin)
        self.vector_transform = Point(scale)
        self.z_origin = z_origin
        self.z_scale = z_scale

        # Stacks of auxiliary depth buffers.
        self.aux_depth_buffers = []

        width = ds.get_width()
        height = ds.get_height()
        self.image_buffer = ib = ds.create_image_buffer()
        cairo_surface = \
          cairo.ImageSurface.create_for_data(ib.get_data(),
                                             cairo.FORMAT_ARGB32,
                                             width, height, width*4)
        target = CairoCmdExecutor(cairo_surface, origin, scale)
        self.src_image = Window(target)
        self.src_bbox = BBox()

    def execute(self, cmds):
        '''Execute all the commands in the given list.'''
        for cmd in cmds:
            cmd_name = cmd.get_name()
            if cmd_name in self.draw_cmd_names:
                # Drawing command.
                method_name = 'draw_{}'.format(cmd.get_name())
                method = getattr(self, method_name, None)
                if method is not None:
                    bbox = self.get_clip_region()
                    if bbox and len(self.aux_depth_buffers) > 0:
                        clip_start, clip_end = bbox
                        current_depth_buffer = self.aux_depth_buffers[-1]
                        method(current_depth_buffer, clip_start, clip_end,
                               *self.transform_args(cmd.get_args()))
                        continue
            else:
                method_name = 'cmd_{}'.format(cmd_name)
                method = getattr(self, method_name, None)
                if method is not None:
                    method(*self.transform_args(cmd.get_args()))
                    continue
            print('Unknown method {}'.format(method_name))

    def transform_args(self, args):
        origin = self.origin
        vtx = self.vector_transform
        out = []
        for arg in args:
            if isinstance(arg, Point):
                arg = arg - origin
                arg.x *= vtx.x
                arg.y *= vtx.y
            elif isinstance(arg, Z):
                arg = Z((arg - self.z_origin)*self.z_scale)
            elif isinstance(arg, Point3):
                arg = arg - Point3(origin, self.z_origin)
                arg.x *= vtx.x
                arg.y *= vtx.y
                arg.z *= self.z_scale
            out.append(arg)
        return out

    def get_clip_region(self):
        '''Return the bounding box of whatever has been drawn to the src window
        since the last transfer.
        '''
        return BBox(*self.transform_args(p for p in self.src_bbox))

    def save(self, real_file_name, depth_file_name):
        self.deep_surface.save_to_files(real_file_name, depth_file_name)

    def cmd_set_bbox(self, *args):
        pass

    def cmd_src_draw(self, window):
        self.src_bbox.take(window)
        self.src_image.take(window)

    def cmd_depth_new(self):
        '''Add a new depth buffer to the stack.'''
        self.aux_depth_buffers.append(self.deep_surface.take_depth_buffer())

    def cmd_depth_merge(self):
        '''Merge the topmost two depth buffers.'''
        pass

    def cmd_depth_sculpt(self):
        '''Sculpt the topmost depth buffer on top of the previous one.'''
        pass

    def cmd_transfer(self):
        current_depth_buffer = self.aux_depth_buffers.pop()
        self.deep_surface.transfer(current_depth_buffer, self.image_buffer)
        current_depth_buffer.clear()
        self.image_buffer.clear()
        self.src_bbox = BBox()
        self.deep_surface.give_depth_buffer(current_depth_buffer)

    def draw_on_step(self, dst, clip_start, clip_end,
                     start, z_start, end, z_end):
        args = (tuple(clip_start) + tuple(clip_end) +
                (start.x, start.y, z_start, end.x, end.y, z_end))
        dst.draw_step(*map(float, args))

    def draw_on_plane(self, dst, clip_start, clip_end, p1, p2, p3):
        mx = Axes(p1.xy, p2.xy, p3.xy).get_matrix()
        mx.invert()
        args = (tuple(clip_start) + tuple(clip_end) +
                mx.get_entries() + (p1.z, p2.z, p3.z))
        dst.draw_plane(*map(float, args))

    def draw_on_sphere(self, dst, clip_start, clip_end,
                       center_2d, one_zero, zero_one, z_start, z_end):
        mx = Axes(center_2d, one_zero, zero_one).get_matrix()
        mx.invert()
        args = map(float, (tuple(clip_start) + tuple(clip_end) +
                           mx.get_entries() + (z_start, z_end)))
        dst.draw_sphere(*args)

    def draw_on_cylinder(self, dst, clip_start, clip_end,
                         start_point, start_edge, end_point, end_edge):
        start_delta = start_edge - start_point
        end_delta = end_edge - end_point
        if start_delta.xy.norm() < end_delta.xy.norm():
            start_point, end_point = (end_point, start_point)
            start_edge, end_edge = (end_edge, start_edge)
            start_delta, end_delta = (end_delta, start_delta)
        if start_delta.xy.norm() == 0.0:
            return

        start_radius_z = start_delta.z
        ref = start_delta.xy
        end_radius_coeff = end_delta.xy.dot(ref)/ref.norm2()
        end_radius_z = end_delta.z
        z_of_axis = 0.5*(start_point.z + end_point.z)

        mx = Axes(start_point.xy, end_point.xy, start_edge.xy).get_matrix()
        mx.invert()

        args = (tuple(clip_start) + tuple(clip_end) + mx.get_entries() +
                (end_radius_coeff, z_of_axis, start_radius_z, end_radius_z))
        dst.draw_cylinder(*map(float, args))

    def draw_on_circular(self, dst, clip_start, clip_end,
                         start_point, end_point, start_edge, radius_fn):
        translation_z = start_point.z
        scale_z = start_edge.z - start_point.z

        mx = Axes(start_point.xy, end_point.xy, start_edge.xy).get_matrix()
        mx.invert()

        float_args = map(float, (tuple(clip_start) + tuple(clip_end) +
                                 mx.get_entries() + (scale_z, translation_z)))
        args = list(float_args) + [radius_fn]
        dst.draw_circular(*args)
