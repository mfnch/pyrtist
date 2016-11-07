__all__ = ('Image', 'Extend', 'Filter')

import cairo

from .base import Taker, combination
from .core_types import Point, Offset, create_enum
from .axes import Axes
from .window import Window
from .cmd_stream import Cmd, CmdStream
from .pattern import Pattern
from .poly import Poly


doc = '''Extend mode to be used for drawing outside the pattern area.

Available options are:
- Extend.none: pixels outside of the source pattern are fully transparent,
- Extend.repeat: the pattern is tiled by repeating,
- Extend.reflect: the pattern is tiled by reflecting at the edges,
- Extend.pad: pixels outside of the pattern copy the closest pixel from the
  source.
'''
Extend = create_enum('Extend', doc,
                     'unset', 'none', 'repeat', 'reflect', 'pad')

@combination(Extend, CmdStream)
def extend_at_cmd_stream(extend, cmd_stream):
    if extend is not Extend.unset:
        cmd_stream.take(Cmd(Cmd.pattern_set_extend, extend.name))


doc = '''Filtering to  be applied when reading pixel values from patterns.

Available values are:
- Filter.fast: a high-performance filter, with quality similar to
  Filter.nearest,
- Filter.good: a reasonable-performance filter, with quality similar to
  Filter.bilinear,
- Filter.best: the highest-quality  available, performance may not be
  suitable for interactive use,
- Filter.nearest: nearest-neighbor filtering,
- Filter.bilinear: linear interpolation in two dimensions
- Filter.gaussian: this filter value is currently unimplemented, and should
  not be used in current code.
'''
Filter = create_enum('Filter', doc,
                     'unset', 'fast', 'good', 'best', 'nearest', 'bilinear',
                     'gaussian')

@combination(Filter, CmdStream)
def filter_at_cmd_stream(flt, cmd_stream):
    if flt is not Filter.unset:
        cmd_stream.take(Cmd(Cmd.pattern_set_filter, flt.name))


class Image(Pattern, Taker):
    def __init__(self, *args):
        self.pattern_filter = None
        self.pattern_extend = None
        self.surface = None
        self.file_name = None
        self.axes = Axes()
        self.offset = Point()
        super(Image, self).__init__(*args)

    def _get_cmds(self):
        # Pass offset as a tuple to avoid having it transformed by a
        # CmdArgFilter.
        surface = self.surface
        if surface is None:
            if self.file_name is None:
                raise ValueError('Image lacks a file name or a cairo surface')
            surface = cairo.ImageSurface.create_from_png(self.file_name)
        offset = tuple(self.offset)
        cmds = CmdStream()
        cmds.take(Cmd(Cmd.pattern_create_image,
                      self.axes.origin, self.axes.one_zero, self.axes.zero_one,
                      offset, surface),
                  self.pattern_filter,
                  self.pattern_extend,
                  Cmd(Cmd.pattern_set_source))
        return cmds.cmds


@combination(Filter, Image)
def filter_at_image(flt, image):
    image.pattern_filter = flt

@combination(Extend, Image)
def extend_at_image(extend, image):
    image.pattern_extend = extend

@combination(str, Image)
def str_at_image(s, image):
    image.file_name = s

@combination(cairo.ImageSurface, Image)
def surface_at_image(surface, image):
    image.surface = surface

@combination(Axes, Image)
def axes_at_image(axes, image):
    image.axes = axes

@combination(Point, Image)
@combination(int, Image)
@combination(float, Image)
def axes_arg_at_image(scalar, image):
    image.axes.take(scalar)

@combination(Offset, Image)
def offset_at_image(offset, image):
    image.offset = offset

@combination(Image, Window)
def image_at_window(image, window):
    origin = image.axes.origin
    x_one = image.axes.one_zero - origin
    y_one = image.axes.zero_one - origin
    mxmy = origin - image.offset.x*x_one - image.offset.y*y_one
    window.take(Poly(mxmy, mxmy + x_one, mxmy + x_one + y_one,
                     mxmy + y_one, image))
