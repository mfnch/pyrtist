'''Merge object: allows merging surfaces.'''

__all__ = ('Merge',)

from ..lib2d.base import combination
from ..lib2d import Window
from .primitive import Primitive
from .cmd_stream import Cmd


class Merge(Primitive):
    def __init__(self, *args):
        super(Merge, self).__init__()
        self.primitives = []
        self.take(*args)

    def get_window(self, *args):
        w = Window()
        for p in self.primitives:
            w.take(p.get_window())
        return w

    def build_cmds(self):
        cmds = []
        w = ([Cmd(Cmd.image_draw, self.window)]
             if self.window is not None else None)
        for p in self.primitives:
            do_merge = (len(cmds) > 0)
            cmds.extend(p.build_cmds())
            if do_merge:
                cmds.append(Cmd(Cmd.merge))
        return cmds

@combination(Primitive, Merge)
def primitive_at_merge(primitive, merge):
    merge.primitives.append(primitive)
