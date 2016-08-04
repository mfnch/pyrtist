__all__ = ('Color', 'Stroke', 'Style', 'Border', 'StrokeStyle', 'Join', 'Cap')

from base import Scalar, Taker, combination, enum, alias
from path import Path
from cmd_stream import Cmd, CmdStream

### Color #####################################################################
class Color(object):
    def __init__(self, value=None, r=0.0, g=0.0, b=0.0, a=1.0):
        if value is not None:
            if len(value) == 3:
                r, g, b = value
            else:
                assert(len(value) == 4)
                r, g, b, a = value
        self.r = r
        self.b = b
        self.g = g
        self.a = a

    def __repr__(self):
        return 'Color(({}, {}, {}))'.format(self.r, self.g, self.b)

@combination(Color, CmdStream)
def fn(color, cmd_stream):
    cmd_stream(Cmd(Cmd.set_source_rgba, color.r, color.g, color.b, color.a))

Color.none = Color(a=0.0)
Color.black = Color()
Color.grey = Color(r=0.5, b=0.5, g=0.5)
Color.white = Color(r=1.0, b=1.0, g=1.0)
Color.red = Color(r=1.0)
Color.green = Color(g=1.0)
Color.blue = Color(b=1.0)

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

Join = enum('Join', 'Enumeration of join styles for lines',
            miter=0, round=1, bevel=2)

@combination(Join, CmdStream)
def fn(join, cmd_stream):
    cmd_stream.take(Cmd(Cmd.set_line_join, join))

Cap = enum('Cap', 'Enumeration of cap styles for lines',
           butt=0, round=1, square=2)

@combination(Cap, CmdStream)
def fn(cap, cmd_stream):
    cmd_stream.take(Cmd(Cmd.set_line_cap, cap))

doc = ('If the current line join style (see `Join`) is set to `Join.miter`, '
'the miter limit (a real number) is used to decide whether the lines should '
'be joined with a bevel instead of a miter. This is determined by dividing '
'the miter by the line width. If the result is greater than the miter limit, '
'then the style is converted to a `Join.bevel`.')
MiterLimit = alias('MiterLimit', float, __doc__=doc)

doc = 'Whether this style should be made the default style. '
'This is mostly used internally. It allows to control the scope of validity '
'of a Style object. For example, ``Poly(..., Style(MakeDefault.no, ...))`` '
'traces a polygon using the provided style. The style is used only for '
'tracing this polygon and does not affect the default style. '
'Alternatively, ``Poly(..., Style(MakeDefault.yes, ...))`` traces the polygon '
'using the provided style, which will also become the default style. '
'Following ``Poly`` will use this style, unless another style is explicitly '
'provided.'
MakeDefault = enum('MakeDefault', doc, no=False, yes=True)

class StrokeStyle(StyleBase):
    attrs = ('pattern', 'width', 'dash', 'miter_limit', 'join', 'cap',
             'make_default')

# Border is just another name for StrokeStyle.
Border = StrokeStyle

@combination(StrokeStyle, StrokeStyle)
def fn(child, parent):
    parent.set_from(child)

@combination(Color, StrokeStyle)
def fn(color, stroke_style):
    stroke_style.pattern = color

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

@combination(Color, Style)
def fn(color, style):
    style.pattern = color

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
        self.path = Path()
        self.stroke_style = StrokeStyle()
        super(Stroke, self).__init__(*args)

@combination(Path, Stroke)
def fn(path, stroke):
    stroke.path(path)

@combination(StrokeStyle, Stroke)
def fn(stroke_style, stroke):
    stroke.stroke_style.take(stroke_style)

@combination(Stroke, CmdStream)
def fn(stroke, cmd_stream):
    cmd_stream.take(stroke.path, stroke.stroke_style)
