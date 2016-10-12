__all__ = ('Primitive',)

from ..lib2d import Window, Color
from ..lib2d.base import Taker, combination
from .cmd_stream import Cmd, CmdStream
from .deep_window import DeepWindow


class Primitive(Taker):
    def __init__(self, *args):
        self.window = None
        self.extra_args = []
        super(Primitive, self).__init__(*args)

    def get_window(self):
        '''Get the Window object associated to this primitive or build a
        default profile if none was provided by the user.
        '''
        if self.window is not None:
            return self.window
        w = Window()
        w.take(self.get_profile(*self.extra_args))
        return w

    def build_depth_cmd(self):
        '''Return a list of commands that can be used to construct the depth
        buffer for this primitive.
        '''
        return []

    def build_image_cmd(self):
        '''Return a list of commands which can be used to construct the image
        buffer for this primitive.
        '''
        return [Cmd(Cmd.image_draw, self.get_window())]

    def get_profile(self, extra_args):
        raise NotImplementedError('Primitive profile not implemented')

@combination(Window, Primitive)
def window_at_primitive(window, primitive):
    if primitive.window is not None:
        raise TypeError('{} takes only one Window'
                        .format(window.__class__.__name__))
    primitive.window = window

@combination(Color, Primitive)
def color_at_primitive(extra_arg, primitive):
    primitive.extra_args.append(extra_arg)

@combination(Primitive, DeepWindow)
def primitive_at_deep_window(primitive, deep_window):
    deep_window.take(CmdStream(primitive))

@combination(Primitive, CmdStream)
def primitive_at_cmd_stream(primitive, cmd_stream):
    cmds = [Cmd(Cmd.image_new)]
    cmds.extend(primitive.build_image_cmd())
    cmds.append(Cmd(Cmd.depth_new))
    cmds.extend(primitive.build_depth_cmd())
    cmds.append(Cmd(Cmd.transfer))
    cmd_stream.take(*cmds)
