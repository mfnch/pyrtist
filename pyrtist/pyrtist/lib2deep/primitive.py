__all__ = ('Primitive',)

from ..lib2d import Window, Color
from ..lib2d.base import Taker, combination
from .cmd_stream import CmdStream
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
