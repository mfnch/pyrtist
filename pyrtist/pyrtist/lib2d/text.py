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

__all__ = ('Text',)

from .base import *
from .core_types import *
from .axes import Axes
from .primitive import Primitive
from .style import Font, ParagraphFormat
from .cmd_stream import CmdStream, Cmd


class Text(Primitive):
    def __init__(self, *args):
        self.string = ''
        self.axes = None
        self.offset = Offset(0.5, 0.5)
        self.paragraph_format = None
        super(Text, self).__init__(*args)

    def build_path(self):
        cmds = CmdStream()
        if self.axes is None or len(self.string) == 0:
            return cmds.cmds
        cmds.take(self.style.font)
        ax = self.axes
        size = 1.0
        if self.style.font is not None and self.style.font.size is not None:
            size = self.style.font.size
        ax.scale(size)
        cmds.take(Cmd(Cmd.text_path, ax.origin, ax.one_zero, ax.zero_one,
                      tuple(self.offset), self.paragraph_format, self.string))
        return cmds.cmds


@combination(Font, Text)
def font_at_text(font, text):
    text.style.take(font)

@combination(Point, Text)
def point_at_text(point, text):
    text.axes = Axes(point)

@combination(str, Text)
def str_at_text(string, text):
    text.string += string

@combination(Offset, Text)
def offset_at_text(offset, text):
    text.offset = offset

@combination(ParagraphFormat, Text)
def par_fmt_at_text(par_fmt, text):
    text.paragraph_format = par_fmt
