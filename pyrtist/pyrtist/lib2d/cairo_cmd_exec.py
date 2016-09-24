__all__ = ('CairoCmdExecutor',)

import numbers
import cairo

from .core_types import *
from .cmd_stream import CmdStream
from .style import Cap, Join

def ext_arc_to(ctx, ctr, a, b, angle_begin, angle_end):
    prev_mx = ctx.get_matrix()
    mx = cairo.Matrix(xx=a.x - ctr.x, yx=a.y - ctr.y, x0=ctr.x,
                      xy=b.x - ctr.x, yy=b.y - ctr.y, y0=ctr.y)
    ctx.transform(mx)
    ctx.arc(0.0, 0.0, 1.0,            # center x and y, radius.
            angle_begin, angle_end);  # angle begin and end.
    ctx.set_matrix(prev_mx)

class CairoCmdExecutor(object):
    formats = {'a1': cairo.FORMAT_A1,
               'a8': cairo.FORMAT_A8,
               'rgb24': cairo.FORMAT_RGB24,
               'argb32': cairo.FORMAT_ARGB32}

    caps = {Cap.butt: cairo.LINE_CAP_BUTT,
            Cap.round: cairo.LINE_CAP_ROUND,
            Cap.square: cairo.LINE_CAP_SQUARE}

    joins = {Join.miter: cairo.LINE_JOIN_MITER,
             Join.round: cairo.LINE_JOIN_ROUND,
             Join.bevel: cairo.LINE_JOIN_BEVEL}

    @classmethod
    def create_image_surface(cls, mode, width, height):
        '''Create a new image surface.'''
        cairo_fmt = cls.formats.get(mode, None)
        if cairo_fmt is None:
            raise ValueError('Invalid format {}'.format(mode))
        return cairo.ImageSurface(cairo_fmt, width, height)

    @staticmethod
    def for_surface(cairo_surface, top_left=None, bot_right=None,
                    top_right=None, bot_left=None, bg_color=None):
        '''Create a CairoCmdExecutor from a given cairo surface and the
        coordinates of two opposite corners.

        The user can either provide top_left, bot_right or top_right, bot_left.
        These optional arguments should contain Point objects (or tuples)
        specifying the coordinates of the corresponding corner. A reference
        system will be set up to honor the user request. If no corner
        coordinates are specified, then this function assumes bot_left=(0, 0)
        and top_right=(width, height).

        bg_color, if provided, is used to set the background with a uniform
        color.
        '''
        if ((top_left is None) != (bot_right is None) or
            (top_right is None) != (bot_left is None)):
            raise TypeError('Only opposing corners should be set')

        if bot_left is None:
            if top_left is None:
                bot_left = Point(0, 0)
                top_right = Point(cairo_surface.get_width(),
                                  cairo_surface.get_height())
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
        scale = Point(cairo_surface.get_width()/diag.x,
                      cairo_surface.get_height()/diag.y)
        return CairoCmdExecutor(cairo_surface, top_left, scale,
                                bg_color=bg_color)

    def __init__(self, surface, origin, resolution, bg_color=None):
        context = cairo.Context(surface)
        self.surface = surface
        self.context = context
        self.vector_transform = resolution
        self.scalar_transform = 0.5*(abs(resolution.x) + abs(resolution.y))
        self.origin = origin
        self.size = (surface.get_width(), surface.get_height())
        if bg_color is not None:
            context.save()
            context.rectangle(0, 0, *self.size)
            context.set_source_rgba(*bg_color)
            context.fill()
            context.restore()

    def execute(self, cmds):
        for cmd in cmds:
            method_name = 'cmd_{}'.format(cmd.get_name())
            method = getattr(self, method_name, None)
            if method is not None:
                method(*self.transform_args(cmd.get_args()))
            else:
                print('Unknown method {}'.format(method_name))
        return CmdStream()

    def transform_args(self, args):
        origin = self.origin
        vtx = self.vector_transform
        stx = self.scalar_transform
        out = []
        for arg in args:
            if isinstance(arg, Point):
                arg = arg - origin
                arg.x *= vtx.x
                arg.y *= vtx.y
            elif isinstance(arg, Scalar):
                arg = Scalar(arg*stx)
            out.append(arg)
        return out

    def save(self, file_name):
        self.surface.write_to_png(file_name)

    def cmd_move_to(self, p):
        self.context.move_to(p.x, p.y)

    def cmd_line_to(self, p):
        self.context.line_to(p.x, p.y)

    def cmd_curve_to(self, p_out, p_in, p):
        self.context.curve_to(p_out.x, p_out.y, p_in.x, p_in.y, p.x, p.y)

    def cmd_ext_arc_to(self, ctr, a, b, angle_begin, angle_end):
        ext_arc_to(self.context, ctr, a, b, angle_begin, angle_end)

    def cmd_close_path(self):
        self.context.close_path()

    def cmd_set_line_width(self, width):
        self.context.set_line_width(float(width))

    def cmd_set_line_join(self, join):
        cairo_join = self.joins.get(join)
        if cairo_join is None:
            raise ValueError('Unknown join style {}'.format(join))
        self.context.set_line_join(cairo_join)

    def cmd_set_line_cap(self, cap):
        cairo_cap = self.caps.get(cap)
        if cairo_cap is None:
            raise ValueError('Unknown cap style {}'.format(cap))
        self.context.set_line_cap(cairo_cap)

    def cmd_set_source_rgba(self, r, g, b, a):
        self.context.set_source_rgba(r, g, b ,a)

    def cmd_stroke(self):
        self.context.stroke()

    def cmd_fill(self):
        self.context.fill()

    def cmd_fill_preserve(self):
        self.context.fill_preserve()

    def cmd_save(self):
        self.context.save()

    def cmd_restore(self):
        self.context.restore()

    def cmd_set_bbox(self, *args):
        pass
