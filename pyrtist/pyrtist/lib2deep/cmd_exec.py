__all__ = ('CmdExecutor',)

import cairo

from ..lib2d import CairoCmdExecutor, Window, BBox, Axes
from .. import deepsurface
from .core_types import Point3, Point, Z


class CmdExecutor(object):
    @staticmethod
    def create_surface(width, height):
        return deepsurface.DeepSurface(width, height)

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
        self.depth_buffer = ds.get_src_depth_buffer()
        width = ds.get_width()
        height = ds.get_height()
        ib = ds.get_src_image_buffer()
        cairo_surface = \
          cairo.ImageSurface.create_for_data(ib.get_data(),
                                             cairo.FORMAT_ARGB32,
                                             width, height, width*4)
        target = CairoCmdExecutor(cairo_surface, origin, scale)
        self.src_image = Window(target)
        self.src_bbox = BBox()

    def execute(self, cmds):
        for cmd in cmds:
            method_name = 'cmd_{}'.format(cmd.get_name())
            method = getattr(self, method_name, None)
            if method is not None:
                method(*self.transform_args(cmd.get_args()))
            else:
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

    def cmd_set_bbox(self, *args):
        pass

    def cmd_transfer(self):
        self.deep_surface.transfer()
        self.src_bbox = BBox()

    def cmd_src_draw(self, window):
        self.src_bbox.take(window)
        self.src_image.take(window)

    def cmd_on_step(self, start, z_start, end, z_end):
        bbox = self.get_clip_region()
        if not bbox:
            return
        clip_start, clip_end = bbox
        args = (clip_start.x, clip_start.y, clip_end.x, clip_end.y,
                start.x, start.y, z_start, end.x, end.y, z_end)
        self.depth_buffer.draw_step(*map(float, args))

    def cmd_on_plane(self, p1, p2, p3):
        bbox = self.get_clip_region()
        if not bbox:
            return
        clip_start, clip_end = bbox

        mx = Axes(p1.xy, p2.xy, p3.xy).get_matrix()
        mx.invert()

        args = ((clip_start.x, clip_start.y, clip_end.x, clip_end.y) +
                mx.get_entries() + (p1.z, p2.z, p3.z))
        self.depth_buffer.draw_plane(*map(float, args))

    def cmd_on_sphere(self, center_2d, one_zero, zero_one, z_start, z_end):
        x, y = center_2d
        radius = 0.5*((one_zero - center_2d).norm() +
                      (zero_one - center_2d).norm())
        z_scale = (z_end - z_start)/float(radius)
        z_scale = 1.0
        self.depth_buffer.draw_sphere(int(x), int(y), int(z_start),
                                      int(radius), z_scale)

    def cmd_on_cylinder(self, start_point, start_edge, end_point, end_edge):
        bbox = self.get_clip_region()
        if not bbox:
            return
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

        clip_start, clip_end = map(tuple, bbox)
        args = (clip_start + clip_end + mx.get_entries() +
                (end_radius_coeff, z_of_axis, start_radius_z, end_radius_z))
        self.depth_buffer.draw_cylinder(*map(float, args))

    def cmd_on_lathe(self, start_point, end_point, start_edge, radius_fn):
        bbox = self.get_clip_region()
        if not bbox:
            return

        translation_z = start_point.z
        scale_z = start_edge.z - start_point.z

        mx = Axes(start_point.xy, end_point.xy, start_edge.xy).get_matrix()
        mx.invert()

        clip_start, clip_end = map(tuple, bbox)
        float_args = map(float, (clip_start + clip_end + mx.get_entries() +
                                 (translation_z, scale_z)))
        args = list(float_args) + [radius_fn]
        self.depth_buffer.draw_lathe(*args)

    def save(self, real_file_name, depth_file_name):
        self.deep_surface.save_to_files(real_file_name, depth_file_name)
