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
