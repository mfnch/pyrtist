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

__all__ = ('CairoCmdExecutor',)

import math
import numbers
import cairo
from collections import namedtuple

from .core_types import *
from .cmd_stream import CmdStream
from .style import Cap, Join, FillRule, Font, ParagraphFormat
from .text_formatter import TextFormatter

def ext_arc_to(ctx, ctr, a, b, angle_begin, angle_end, direction):
    prev_mx = ctx.get_matrix()
    mx = cairo.Matrix(xx=a.x - ctr.x, yx=a.y - ctr.y, x0=ctr.x,
                      xy=b.x - ctr.x, yy=b.y - ctr.y, y0=ctr.y)
    ctx.transform(mx)
    arc = (ctx.arc_negative if direction < 0 else ctx.arc)
    arc(0.0, 0.0, 1.0,            # center x and y, radius.
        angle_begin, angle_end)   # angle begin and end.
    ctx.set_matrix(prev_mx)

def ext_joinarc_to(ctx, a, b, c):
    prev_mx = ctx.get_matrix()
    mx_xx = b.x - c.x
    mx_yx = b.y - c.y
    mx = cairo.Matrix(xx=mx_xx,     yx=mx_yx,     x0=a.x - mx_xx,
                      xy=b.x - a.x, yy=b.y - a.y, y0=a.y - mx_yx)
    ctx.transform(mx)
    ctx.arc(0.0, 0.0, 1.0,            # center x and y, radius.
            0.0, 0.5*math.pi)         # angle begin and end.
    ctx.set_matrix(prev_mx)


class CairoTextFormatter(TextFormatter):
    StackFrame = namedtuple('Stack', ['cur_pos', 'cur_mx'])

    def __init__(self, context, paragraph_fmt):
        super(CairoTextFormatter, self).__init__()
        self.context = context
        self.stack = []
        self.fmt = paragraph_fmt or ParagraphFormat.default

    def cmd_draw(self):
        self.context.text_path(self.pop_text())

    def cmd_save(self):
        frame = self.StackFrame(cur_pos=self.context.get_current_point(),
                                cur_mx=self.context.get_matrix())
        self.stack.append(frame)

    def cmd_restore(self):
        frame = self.stack.pop()
        self.context.set_matrix(frame.cur_mx)
        _, y = frame.cur_pos
        x, _ = self.context.get_current_point()
        self.context.move_to(x, y)

    def change_fmt(self, translation, scale):
        x, y = self.context.get_current_point()
        mx = cairo.Matrix(xx=scale, xy=0.0,
                          yx=0.0, yy=scale,
                          x0=x, y0=y + translation)
        self.context.transform(mx)
        self.context.move_to(0.0, 0.0)

    def cmd_superscript(self):
        self.change_fmt(self.fmt.superscript_pos, self.fmt.superscript_scale)

    def cmd_subscript(self):
        self.change_fmt(self.fmt.subscript_pos, self.fmt.subscript_scale)

    def cmd_newline(self):
        self.context.translate(0.0, -self.fmt.line_spacing)
        self.context.move_to(0.0, 0.0)


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

    slants = {Font.Slant.normal: cairo.FONT_SLANT_NORMAL,
              Font.Slant.italic: cairo.FONT_SLANT_ITALIC,
              Font.Slant.oblique: cairo.FONT_SLANT_OBLIQUE,
              None: cairo.FONT_SLANT_NORMAL}

    weights = {Font.Weight.normal: cairo.FONT_WEIGHT_NORMAL,
               Font.Weight.bold: cairo.FONT_WEIGHT_BOLD,
               None: cairo.FONT_WEIGHT_NORMAL}

    pattern_extends = {'none': cairo.EXTEND_NONE,
                       'repeat': cairo.EXTEND_REPEAT,
                       'reflect': cairo.EXTEND_REFLECT,
                       'pad': cairo.EXTEND_PAD}

    pattern_filters = {'fast': cairo.FILTER_FAST,
                       'good': cairo.FILTER_GOOD,
                       'best': cairo.FILTER_BEST,
                       'nearest': cairo.FILTER_NEAREST,
                       'bilinear': cairo.FILTER_BILINEAR,
                       'gaussian': cairo.FILTER_GAUSSIAN}


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
        self.pattern = None
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

        self.cmd_set_fill_rule(FillRule.even_odd)
        self.cmd_set_font(None, None, None)
        self.cmd_set_line_width(0.25*self.scalar_transform)

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

    def cmd_ext_arc_to(self, ctr, a, b, angle_begin, angle_end, direction):
        ext_arc_to(self.context, ctr, a, b, angle_begin, angle_end, direction)

    def cmd_ext_joinarc_to(self, a, b, c):
        ext_joinarc_to(self.context, a, b, c)

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

    def cmd_set_dash(self, offset, *dashes):
        self.context.set_dash(dashes, offset)

    def cmd_set_source_rgba(self, r, g, b, a):
        self.context.set_source_rgba(r, g, b ,a)

    def cmd_set_fill_rule(self, fill_rule):
        if fill_rule == FillRule.winding:
            self.context.set_fill_rule(cairo.FILL_RULE_WINDING)
        else:
            assert fill_rule is FillRule.even_odd, \
              'Unknown fill rule'
            self.context.set_fill_rule(cairo.FILL_RULE_EVEN_ODD)

    def cmd_pattern_set_extend(self, extend):
        if self.pattern is None:
            return
        cairo_extend = self.pattern_extends.get(extend)
        if cairo_extend is None:
            raise ValueError('Unknown pattern extend mode {}'.format(extend))
        self.pattern.set_extend(cairo_extend)

    def cmd_pattern_set_filter(self, flt):
        if self.pattern is None:
            return
        cairo_filter = self.pattern_filters.get(flt)
        if cairo_filter is None:
            raise ValueError('Unknown pattern filter {}'.format(flt))
        self.pattern.set_filter(cairo_filter)

    def cmd_pattern_set_source(self):
        if self.pattern is None:
            return
        self.context.set_source(self.pattern)

    def cmd_pattern_create_image(self, origin, v10, v01, offset, surface):
        lx = float(surface.get_width())
        ly = float(surface.get_height())
        mx = cairo.Matrix(xx=(v10.x - origin.x)/lx, yx=(v10.y - origin.y)/lx,
                          xy=(origin.x - v01.x)/ly, yy=(origin.y - v01.y)/ly,
                          x0=v01.x, y0=v01.y)
        mx.invert()
        mx = mx.multiply(cairo.Matrix(x0=offset[0]*lx, y0=-offset[1]*ly))
        pattern = cairo.SurfacePattern(surface)
        pattern.set_matrix(mx)
        self.pattern = pattern

    def cmd_pattern_create_linear(self, p_start, p_stop):
        self.pattern = cairo.LinearGradient(p_start.x, p_start.y,
                                            p_stop.x, p_stop.y)

    def cmd_pattern_create_radial(self, ctr1, refx, refy, ctr2,
                                  refr1, refr2):
        xaxis = refx - ctr1
        yaxis = refy - ctr1
        mx = cairo.Matrix(xx=xaxis.x, yx=xaxis.y,
                          xy=yaxis.x, yy=yaxis.y,
                          x0=ctr1.x, y0=ctr1.y)
        mx.invert()
        c2x, c2y = mx.transform_point(ctr2.x, ctr2.y)
        unity = xaxis.norm()
        r1 = (refr1 - ctr1).norm()/unity
        r2 = (refr2 - ctr2).norm()/unity
        pattern = cairo.RadialGradient(0.0, 0.0, r1, c2x, c2y, r2)
        pattern.set_matrix(mx)
        self.pattern = pattern

    def cmd_pattern_add_color_stop_rgba(self, offset, color):
        if self.pattern is None:
            return
        self.pattern.add_color_stop_rgba(offset, *color)

    def cmd_set_font(self, name, weight, slant):
        name = name or 'sans'
        cairo_weight = self.weights.get(weight)
        cairo_slant = self.slants.get(slant)
        self.context.select_font_face(name, cairo_slant, cairo_weight)

    def cmd_text_path(self, origin, v10, v01, offset, par_fmt, text):
        # Matrix for the reference system identified by origin, v10, v01.
        mx = cairo.Matrix(xx=(v10.x - origin.x), xy=(v01.x - origin.x),
                          yx=(v10.y - origin.y), yy=(v01.y - origin.y),
                          x0=origin.x, y0=origin.y)

        # Compute the text path in a separate context, to avoid polluting
        # the current context.
        new_context = cairo.Context(self.context.get_target())

        prev_mx = None
        try:
            # Save the matrix (so it can be resored later) and change
            # coordinates.
            prev_mx = self.context.get_matrix()
            self.context.transform(mx)

            # Copy settings from self.context.
            new_context.set_matrix(self.context.get_matrix())
            new_context.move_to(0.0, 0.0)

            new_context.set_font_face(self.context.get_font_face())
            font_mx = cairo.Matrix(xx=1.0, yx=0.0,
                                   xy=0.0, yy=-1.0,
                                   x0=0.0, y0=0.0)
            new_context.set_font_matrix(font_mx)

            # Draw the path.
            fmt = CairoTextFormatter(new_context, par_fmt)
            fmt.run(text)

            # Obtain the path extent in these coordinates.
            x1, y1, x2, y2 = new_context.fill_extents()
            text_path = new_context.copy_path()

            # Position the text path correctly in the main context.
            self.context.translate(-x1 - (x2 - x1)*offset[0],
                                   -y1 - (y2 - y1)*offset[1])
            self.context.append_path(text_path)
        finally:
            if prev_mx is not None:
                self.context.set_matrix(prev_mx)

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
