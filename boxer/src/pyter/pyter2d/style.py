from base import Taker, combination
from path import Path
from window import Window
from cmd_stream import Cmd, CmdStream

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

@combination(Color, Window, 'Color')
def fn(color, window):
    window(CmdStream(color))

Color.none = Color(a=0.0)
Color.black = Color()
Color.grey = Color(r=0.5, b=0.5, g=0.5)
Color.white = Color(r=1.0, b=1.0, g=1.0)
Color.red = Color(r=1.0)
Color.green = Color(g=1.0)
Color.blue = Color(b=1.0)

class Style(Taker):
    def __init__(self, *args):
        self.color = None
        self.preserve = None
        super(Style, self).__init__(*args)

@combination(Color, Style)
def fn(color, style):
    style.color = color

class Stroke(Taker):
    def __init__(self, *args):
        self.path = Path()
        self.style_cmd = CmdStream()
        super(Stroke, self).__init__(*args)

@combination(Path, Stroke)
def fn(path, stroke):
    stroke.path(path)

@combination(Style, Stroke)
def fn(style, stroke):
    cmds = stroke.style_cmd
    cmds(Cmd(Cmd.stroke))

@combination(Stroke, Window)
def fn(stroke, window):
    window.take(stroke.path.cmd_stream,
                stroke.style_cmd)

@combination(Style, CmdStream)
def fn(style, cmd_stream):
    preserve = (style.preserve != False)
    if preserve:
        cmd_stream(Cmd(Cmd.save))
    if style.color is not None:
        cmd_stream(style.color)
    cmd_stream(Cmd(Cmd.fill))
    if preserve:
        cmd_stream(Cmd(Cmd.restore))
