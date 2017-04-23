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

__all__ = ('Color', 'Grey', 'Stroke', 'FillRule', 'Fill', 'Style',
           'Border', 'StrokeStyle', 'Join', 'Cap', 'Dash', 'Font',
           'ParagraphFormat')

import numbers
import colorsys

from .base import *
from .core_types import *
from .path import Path
from .cmd_stream import Cmd, CmdStream
from .pattern import Pattern

### Color #####################################################################
class Color(Pattern):
    @staticmethod
    def from_hsva(h=0.0, s=0.0, v=0.0, a=1.0):
        '''Create a Color from hue, saturation, value and alpha components.'''
        r, g, b = colorsys.hsv_to_rgb(h, s, v)
        return Color(r, g, b, a)

    def __init__(self, r=0.0, g=0.0, b=0.0, a=1.0):
        super(Color, self).__init__()
        if not isinstance(r, numbers.Number):
            comps = tuple(r)
            n = len(comps)
            if n == 4: r, g, b, a = comps
            elif n == 3: r, g, b = comps
            elif n == 2: r, g = comps
            elif n == 1: r = comps[0]
            else:
                raise ValueError('Invalid argument for Color: {}'
                                 .format(repr(r)))
        self.r = r
        self.b = b
        self.g = g
        self.a = a

    def _get_cmds(self):
        return [Cmd(Cmd.set_source_rgba, self.r, self.g, self.b, self.a)]

    def __repr__(self):
        return 'Color({}, {}, {}, {})'.format(self.r, self.g, self.b, self.a)

    def __iter__(self):
        return iter((self.r, self.g, self.b, self.a))

    def get_hsva(self):
        '''Get the hue, saturation, value and alpha components for this color.
        '''
        h, s, v = colorsys.rgb_to_hsv(self.r, self.g, self.b)
        return (h, s, v, self.a)

    def darken(self, amount):
        '''Return a color which is darker than this color by the given amount.
        '''
        h, s, v, a = self.get_hsva()
        v = v*amount
        return Color.from_hsva(h, s, v, a)

Color.none = Color(a=0.0)
Color.black = Color()
Color.grey = Color(0.5, 0.5, 0.5)
Color.white = Color(1.0, 1.0, 1.0)
Color.red = Color(r=1.0)
Color.green = Color(g=1.0)
Color.blue = Color(b=1.0)
Color.yellow = Color(r=1.0, g=1.0)
Color.magenta = Color(r=1.0, b=1.0)
Color.cyan = Color(g=1.0, b=1.0)

def Grey(value, a=1.0):
    'Return a grey color with the given intensity and alpha value.'
    return Color(r=value, g=value, b=value, a=a)

### StyleBase #################################################################

class StyleBase(Taker):
    '''This is the base class for Border and Style. It defines an object with
    some attributes that can optonally be set. Unset attributes have are
    internally set to None. A StyleBase instance can be used to set another
    StyleBase instance. In this case, only the set attributes are propagated
    to the target, while unset attributes are ignored.
    '''

    attrs = ()

    def __init__(self, *args):
        for attr_name in self.attrs:
            setattr(self, attr_name, None)
        super(StyleBase, self).__init__(*args)

    def set_from(self, stroke_style):
        for attr_name in self.attrs:
            attr_value = getattr(stroke_style, attr_name)
            if attr_value is not None:
                if isinstance(attr_value, StyleBase):
                    parent_attr_value = getattr(self, attr_name)
                    if parent_attr_value is None:
                        parent_attr_value = type(attr_value)()
                        setattr(self, attr_name, parent_attr_value)
                    parent_attr_value.set_from(attr_value)
                else:
                    setattr(self, attr_name, attr_value)

@combination(StyleBase, StyleBase)
def style_base(child, parent):
    parent.set_from(child)


### StrokeStyle & co. #########################################################

Join = create_enum('Join', 'Enumeration of join styles for lines',
                   'miter', 'round', 'bevel')

@combination(Join, CmdStream)
def fn(join, cmd_stream):
    cmd_stream.take(Cmd(Cmd.set_line_join, join))

Cap = create_enum('Cap', 'Enumeration of cap styles for lines',
                  'butt', 'round', 'square')

@combination(Cap, CmdStream)
def fn(cap, cmd_stream):
    cmd_stream.take(Cmd(Cmd.set_line_cap, cap))

doc = ('If the current line join style (see `Join`) is set to `Join.miter`, '
'the miter limit (a real number) is used to decide whether the lines should '
'be joined with a bevel instead of a miter. This is determined by dividing '
'the miter by the line width. If the result is greater than the miter limit, '
'then the style is converted to a `Join.bevel`.')
MiterLimit = alias('MiterLimit', float, __doc__=doc)

doc = ('Whether this style should be made the default style. '
 'This is mostly used internally. It allows to control the scope of validity '
 'of a Style object. For example, ``Poly(..., Style(MakeDefault.no, ...))`` '
 'traces a polygon using the provided style. The style is used only for '
 'tracing this polygon and does not affect the default style. '
 'Alternatively, ``Poly(..., Style(MakeDefault.yes, ...))`` traces the polygon '
 'using the provided style, which will also become the default style. '
 'Following ``Poly`` will use this style, unless another style is explicitly '
 'provided.')
MakeDefault = create_enum('MakeDefault', doc, 'no', 'yes')

class Dash(Taker):
    '''Object specifying the dash lengths.'''
    def __init__(self, *args):
        super(Dash, self).__init__()
        self.offset = 0.0
        self.dashes = []
        self.take(*args)

@combination(Offset, Dash)
def offset_at_dash(offset, dash):
    dash.offset = offset.x

@combination(int, Dash)
@combination(float, Dash)
def scalar_at_dash(scalar, dash):
    dash.dashes.append(scalar)

@combination(Dash, CmdStream)
def dash_at_cmd_stream(dash, cmd_stream):
    if dash.dashes:
        scalars = map(Scalar, [dash.offset] + dash.dashes)
        cmd_stream.take(Cmd(Cmd.set_dash, *scalars))

class StrokeStyle(StyleBase):
    attrs = ('pattern', 'width', 'dash', 'miter_limit', 'join', 'cap',
             'make_default')

# Border is just another name for StrokeStyle.
Border = StrokeStyle

@combination(Pattern, StrokeStyle)
def pattern_at_stroke_style(pattern, stroke_style):
    stroke_style.pattern = pattern

@combination(int, StrokeStyle)
@combination(float, StrokeStyle)
def fn(width, stroke_style):
    stroke_style.width = width

@combination(Join, StrokeStyle)
def fn(join, stroke_style):
    stroke_style.join = join

@combination(Cap, StrokeStyle)
def fn(cap, stroke_style):
    stroke_style.cap = cap

@combination(Dash, StrokeStyle)
def dash_at_stroke_style(dash, stroke_style):
    stroke_style.dash = dash

@combination(StrokeStyle, CmdStream)
def stroke_style_at_cmd_stream(stroke_style, cmd_stream):
    preserve = (stroke_style.make_default != True)
    if preserve:
        cmd_stream.take(Cmd(Cmd.save))
    cmd_stream.take(stroke_style.pattern)
    if stroke_style.width is not None:
        cmd_stream.take(Cmd(Cmd.set_line_width, Scalar(stroke_style.width)))
    cmd_stream.take(stroke_style.dash,
                    stroke_style.join,
                    stroke_style.cap,
                    stroke_style.miter_limit,
                    Cmd(Cmd.stroke))
    if preserve:
        cmd_stream.take(Cmd(Cmd.restore))

### Style #####################################################################

class Style(StyleBase):
    attrs = ('pattern', 'fill_rule', 'stroke_style', 'font', 'make_default')


@combination(StrokeStyle, Style)
def fn(stroke_style, style):
    if style.stroke_style is None:
        style.stroke_style = StrokeStyle()
    style.stroke_style.take(stroke_style)

@combination(Pattern, Style)
def pattern_at_style(pattern, style):
    style.pattern = pattern

@combination(Style, CmdStream)
def style_at_cmd_stream(style, cmd_stream):
    preserve = (style.make_default != True)
    if preserve:
        cmd_stream.take(Cmd(Cmd.save))

    cmd_stream.take(style.pattern)
    cmd_stream.take(style.fill_rule)
    if style.stroke_style is None:
        cmd_stream.take(Cmd(Cmd.fill))
    else:
        cmd_stream.take(Cmd(Cmd.fill_preserve),
                        style.stroke_style)

    if preserve:
        cmd_stream.take(Cmd(Cmd.restore))

### Stroke ####################################################################

class Stroke(Taker):
    def __init__(self, *args):
        super(Stroke, self).__init__()
        self.path = Path()
        self.stroke_style = StrokeStyle()
        self.take(*args)

@combination(Path, Stroke)
def path_at_stroke(path, stroke):
    stroke.path.take(path)

@combination(int, Stroke)
@combination(float, Stroke)
def scalar_at_stroke(scalar, stroke):
    stroke.stroke_style.take(scalar)

@combination(Color, Stroke)
def color_at_stroke(scalar, stroke):
    stroke.stroke_style.take(scalar)

@combination(StrokeStyle, Stroke)
def fn(stroke_style, stroke):
    stroke.stroke_style.take(stroke_style)

@combination(Stroke, CmdStream)
def fn(stroke, cmd_stream):
    cmd_stream.take(stroke.path, stroke.stroke_style)

### Fill Rule #################################################################

FillRule = create_enum('FillRule', 'Rule determining how paths are filled',
                       'winding', 'even_odd')

@combination(FillRule, Style)
def fill_rule_at_style(fill_rule, style):
    style.fill_rule = fill_rule

@combination(FillRule, CmdStream)
def fill_rule_at_cmd_stream(fill_rule, cmd_stream):
    cmd_stream.take(Cmd(Cmd.set_fill_rule, fill_rule))


### Fill ######################################################################


class Fill(Taker):
    def __init__(self, *args):
        super(Fill, self).__init__()
        self.path = Path()
        self.style = Style()
        self.take(*args)

@combination(Color, Fill)
def color_at_fill(color, fill):
    fill.style.take(color)

@combination(Path, Fill)
def path_at_fill(path, fill):
    fill.path.take(path)

@combination(Fill, CmdStream)
def fill_at_cmd_stream(fill, cmd_stream):
    cmd_stream.take(fill.path, fill.style)


### Font ######################################################################

class Font(StyleBase):
    attrs = ('name', 'size', 'slant', 'weight')

    Slant = create_enum('Slant', 'Enumeration of font slant styles',
                        'normal', 'italic', 'oblique')

    Weight = create_enum('Weight', 'Enumeration of font weight styles',
                         'normal', 'bold')

    italic = Slant.italic
    oblique = Slant.oblique
    bold = Weight.bold

    def __init__(self, *args):
        super(Font, self).__init__()
        self.name = None
        self.size = None
        self.slant = None
        self.weight = None
        self.take(*args)


@combination(int, Font)
@combination(float, Font)
def scalar_at_font(scalar, font):
    font.size = scalar

@combination(str, Font)
def string_at_font(string, font):
    font.name = string

@combination(Font.Slant, Font)
def slant_at_font(slant, font):
    font.slant = slant

@combination(Font.Weight, Font)
def weight_at_font(weight, font):
    font.weight = weight

@combination(Font, Style)
def font_at_style(font, style):
    if style.font is None:
        style.font = Font()
    style.font.take(font)

@combination(Font, CmdStream)
def font_at_cmd_stream(font, cmd_stream):
    cmd_stream.take(Cmd(Cmd.set_font, font.name,
                        font.weight, font.slant))


### ParagraphFormat ###########################################################

class ParagraphFormat(object):
    attrs = ('line_spacing','subscript_scale', 'subscript_pos',
             'superscript_scale', 'superscript_pos')

    def __init__(self, line_spacing=1.0,
                 subscript_scale=0.5, subscript_pos=-0.1,
                 superscript_scale=0.5, superscript_pos=0.5):
        self.line_spacing = line_spacing
        self.subscript_scale = subscript_scale
        self.subscript_pos = subscript_pos
        self.superscript_scale = superscript_scale
        self.superscript_pos = superscript_pos

    def __repr__(self):
        args = ', '.join('{}={}'.format(attr, getattr(self, attr))
                         for attr in self.attrs)
        return 'ParagraphFormat({})'.format(args)


# The default paragraph format is assumed to be immutable. The user should not
# change it.
ParagraphFormat.default = ParagraphFormat()
