import numbers
import cairo

from base import Scalar
from cmd_stream import CmdStream
from point import Point
from style import Cap, Join

def ext_arc_to(ctx, ctr, a, b, angle_begin, angle_end):
    prev_mx = ctx.get_matrix()
    mx = cairo.Matrix(xx=a.x - ctr.x, yx=a.y - ctr.y, x0=ctr.x,
                      xy=b.x - ctr.x, yy=b.y - ctr.y, y0=ctr.y)
    ctx.transform(mx)
    ctx.arc(0.0, 0.0, 1.0,            # center x and y, radius.
            angle_begin, angle_end);  # angle begin and end.
    ctx.set_matrix(prev_mx)

class CairoCmdExecutor(object):
    default_origin = (0.0, 0.0)
    default_mode = cairo.FORMAT_RGB24
    default_resolution = 10.0

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

    def __init__(self, size=None, resolution=None, origin=None, mode=None):
        if size is None:
            raise ValueError('size missing')

        orig_mode = mode
        mode = self.formats.get(orig_mode, self.default_mode)
        if mode is None:
            raise ValueError('Unknown mode {}'.format(orig_mode))

        # Ensure origin and size are Point objects.
        size = Point(size)
        origin = Point(self.default_origin if origin is None else origin)

        self.mode = mode
        self.size = size
        resolution = (self.default_resolution if resolution is None
                      else resolution)
        resolution = \
          (Point((resolution, resolution))
           if isinstance(resolution, numbers.Number)
           else Point(resolution))

        right_bottom = origin + size
        num_x = int(abs(size.x*resolution.x))
        num_y = int(abs(size.y*resolution.y))

        self.surface = surface = cairo.ImageSurface(mode, num_x, num_y)
        self.context = context = cairo.Context(surface)

        context.save()
        context.rectangle(0, 0, num_x, num_y)
        context.set_source_rgb(1.0, 1.0, 1.0)
        context.fill()
        context.restore()

        # Flip the y axis.
        resolution.y = -resolution.y;
        origin.y += size.y
        self.vector_transform = resolution
        self.scalar_transform = 0.5*(abs(resolution.x) + abs(resolution.y))
        self.origin = origin

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
