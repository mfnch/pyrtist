__all__ = ('CmdExecutor',)

import cairo

from .. import deepsurface
from .core_types import Point


class CmdExecutor(object):
    def __init__(self, width, height):
        self.origin = Point()
        self.vector_transform = Point.vx()
        self.deep_surface = ds = deepsurface.DeepSurface(width, height)
        self.depth_buffer = ds.get_depth_buffer()
        ib = ds.get_image_buffer()
        self.image_buffer = \
          cairo.ImageSurface.create_for_data(ib.get_data(),
                                             cairo.FORMAT_ARGB32,
                                             width, height, width*4)

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

    def cmd_draw_sphere(self, center_2d, one_zero, zero_one,
                        z_start, z_end, window):
        x, y = center_2d
        radius = 0.5*(one_zero.norm() + zero_one.norm())
        z_scale = (z_end - z_start)/float(radius)
        self.depth_buffer.draw_sphere(int(x), int(y), int(z_start),
                                      int(radius), z_scale)
        self.deep_surface.transfer()

    def save(self, base_name):
        self.deep_surface.save_to_files(base_name + '-real.png',
                                        base_name + '-norm.png')
