__all__ = ('Primitive',)

from .core_types import Point
from .style import Stroke, Fill, StrokeStyle, Style
from .pattern import Pattern
from .path import Path
from .base import Taker, combination
from .cmd_stream import CmdStream, Cmd
from .window import Window
from .bbox import BBox


class Primitive(Taker):
    def __init__(self, *args):
        super(Primitive, self).__init__()
        self.style = Style()
        self.take(*args)

    def build_path(self):
        return []

@combination(Pattern, Primitive)
@combination(StrokeStyle, Primitive)
@combination(Style, Primitive)
def style_at_primitive(style, primitive):
    primitive.style.take(style)

@combination(Primitive, Path)
def primitive_at_path(primitive, path):
    path.cmd_stream.take(*primitive.build_path())

@combination(Primitive, CmdStream)
def primitive_at_cmd_stream(primitive, cmd_stream):
    cmd_stream.take(Path(primitive), primitive.style)

@combination(Primitive, Window)
def primitive_at_window(primitive, window):
    window.take(CmdStream(primitive))

@combination(Primitive, Stroke)
def primitive_at_stroke(primitive, stroke):
    stroke.take(Path(primitive))

@combination(Primitive, Fill)
def primitive_at_fill(primitive, fill):
    fill.take(Path(primitive))

@combination(Primitive, BBox)
def primitive_at_bbox(primitive, bbox):
    bbox.take(Window(primitive))
