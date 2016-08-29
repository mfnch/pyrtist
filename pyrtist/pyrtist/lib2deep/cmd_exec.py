__all__ = ('CmdExecutor',)

import cairo

from ..lib2d import CairoCmdExecutor, Window
from .. import deepsurface
from .core_types import Point


class CmdExecutor(object):
    @staticmethod
    def create_surface(width, height):
        return deepsurface.DeepSurface(width, height)

    @staticmethod
    def for_surface(deep_surface, top_left=None, bot_right=None,
                    top_right=None, bot_left=None, bg_color=None):
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
        return CmdExecutor(deep_surface, top_left, scale)

    def __init__(self, deep_surface, origin, scale):
        self.deep_surface = ds = deep_surface
        self.origin = Point(origin)
        self.vector_transform = Point(scale)
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
            out.append(arg)
        return out

    def cmd_set_bbox(self, *args):
        pass

    def cmd_transfer(self):
        self.deep_surface.transfer()

    def cmd_src_draw(self, window):
        self.src_image.take(window)

    def cmd_on_sphere(self, center_2d, one_zero, zero_one, z_start, z_end):
        x, y = center_2d
        radius = 0.5*((one_zero - center_2d).norm() +
                      (zero_one - center_2d).norm())
        z_scale = (z_end - z_start)/float(radius)
        z_scale = 1.0
        self.depth_buffer.draw_sphere(int(x), int(y), int(z_start),
                                      int(radius), z_scale)

    def save(self, base_name):
        self.deep_surface.save_to_files(base_name + '-real.png',
                                        base_name + '-norm.png')
