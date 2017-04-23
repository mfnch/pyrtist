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

__all__ = ('Text', 'Texts')

import copy

from .base import *
from .core_types import *
from .style import Style, StrokeStyle, Font, ParagraphFormat
from .pattern import Pattern
from .axes import Axes
from .primitive import Primitive
from .cmd_stream import CmdStream, Cmd
from .window import Window


class Text(Primitive):
    def __init__(self, *args):
        self.string = ''
        self.axes = None
        self.offset = Offset(0.5, 0.5)
        self.paragraph_format = None
        super(Text, self).__init__(*args)

    def new_similar(self):
        '''Return a new Text object with the same style as this one.'''
        new = Text()
        new.style.take(self.style)
        new.offset = Offset(self.offset)
        new.paragraph_format = copy.deepcopy(self.paragraph_format)
        return new

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


class Texts(Taker):
    def __init__(self, *args):
        super(Texts, self).__init__()
        self._tip = Text()
        self._tail = []
        self.take(*args)

    @property
    def texts(self):
        for item in self._tail:
            yield item
        yield self._tip


@combination(Pattern, Texts)
@combination(StrokeStyle, Texts)
@combination(Style, Texts)
@combination(Font, Texts)
@combination(str, Texts)
@combination(Offset, Texts)
def item_at_texts(item, texts):
    texts._tip.take(item)

@combination(Point, Texts)
def point_at_texts(point, texts):
    point_comes_first = not texts._tip.string

    if point_comes_first:
        # This typically occurs when the Point is the first argument given to
        # Texts().
        texts._tip.take(point)
        return

    # In all the other cases, Point marks either the end of the current Text
    # object or the beginning of the next one. Whether we are in the former or
    # latter case is decided by looking at whether the tip has already a
    # position.
    tip_has_position_already = (texts._tip.axes is not None)

    # In both cases, we will have to create a new tip.
    new_tip = texts._tip.new_similar()

    # If the tip already has a position, then it marks the beginning of a new
    # Text and we assign the point the new tip. Otherwise, we assign it to the
    # old tip.
    if tip_has_position_already:
        new_tip.take(point)
    else:
        texts._tip.take(point)

    # Adjust the Texts object.
    texts._tail.append(texts._tip)
    texts._tip = new_tip


@combination(Texts, CmdStream)
def texts_at_cmd_stream(texts, cmd_stream):
    for text in texts.texts:
        cmd_stream.take(text)

@combination(Texts, Window)
def texts_at_window(primitive, window):
    window.take(CmdStream(primitive))
