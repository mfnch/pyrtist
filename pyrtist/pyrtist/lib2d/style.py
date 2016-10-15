__all__ = ('Color', 'Grey', 'Stroke', 'Style',
           'Border', 'StrokeStyle', 'Join', 'Cap')

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
                setattr(self, attr_name, attr_value)

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

class StrokeStyle(StyleBase):
    attrs = ('pattern', 'width', 'dash', 'miter_limit', 'join', 'cap',
             'make_default')

# Border is just another name for StrokeStyle.
Border = StrokeStyle

@combination(StrokeStyle, StrokeStyle)
def fn(child, parent):
    parent.set_from(child)

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

@combination(StrokeStyle, CmdStream)
def fn(stroke_style, cmd_stream):
    preserve = (stroke_style.make_default != True)
    if preserve:
        cmd_stream.take(Cmd(Cmd.save))
    cmd_stream.take(stroke_style.pattern)
    if stroke_style.width is not None:
        cmd_stream.take(Cmd(Cmd.set_line_width, Scalar(stroke_style.width)))
    cmd_stream.take(stroke_style.join,
                    stroke_style.cap,
                    stroke_style.miter_limit,
                    Cmd(Cmd.stroke))
    if preserve:
        cmd_stream.take(Cmd(Cmd.restore))

### Style #####################################################################

class Style(StyleBase):
    attrs = ('pattern', 'stroke_style', 'make_default')

@combination(Style, Style)
def fn(child, parent):
    parent.set_from(child)

@combination(StrokeStyle, Style)
def fn(stroke_style, style):
    if style.stroke_style is None:
        style.stroke_style = StrokeStyle()
    style.stroke_style.take(stroke_style)

@combination(Pattern, Style)
def pattern_at_style(pattern, style):
    style.pattern = pattern

@combination(Style, CmdStream)
def fn(style, cmd_stream):
    preserve = (style.make_default != True)
    if preserve:
        cmd_stream.take(Cmd(Cmd.save))

    cmd_stream.take(style.pattern)
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
    stroke.path(path)

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
