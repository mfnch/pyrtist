__all__ = ('Primitive',)

from .core_types import Point
from .style import Color, StrokeStyle, Style
from .path import Path
from .base import Taker, combination
from .cmd_stream import CmdStream, Cmd
from .window import Window


class Primitive(Taker):
    def __init__(self, *args):
        super(Primitive, self).__init__()
        self.style = Style()
        self.take(*args)

    def build_path(self):
        return []

@combination(Color, Primitive)
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
