__all__ = ('Image',)

from .base import Taker, combination
from .core_types import Point, Offset
from .axes import Axes
from .window import Window
from .cmd_stream import Cmd
from .pattern import Pattern
from .poly import Poly

class Image(Pattern, Taker):
    def __init__(self, *args):
        self.file_name = None
        self.axes = Axes()
        self.offset = Point()
        super(Image, self).__init__(*args)

    def _get_cmds(self):
        # Pass offset as a tuple to avoid having it transformed by a
        # CmdArgFilter.
        offset = tuple(self.offset)
        return [Cmd(Cmd.pattern_create_image,
                    self.axes.origin, self.axes.one_zero, self.axes.zero_one,
                    offset, self.file_name)]


@combination(str, Image)
def str_at_image(s, image):
    image.file_name = s

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
