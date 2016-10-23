'''Sculpt object: allows sculpting one object on top of another one.'''

__all__ = ('Sculpt',)

from ..lib2d import Window
from ..lib2d.base import combination
from .primitive import Primitive
from .cmd_stream import Cmd
from .merge import Merge


class Sculpt(Primitive):
    def __init__(self, *args):
        super(Sculpt, self).__init__()
        self.primitives = []
        self.take(*args)

    def get_window(self, *args):
        return Window()

    def build_cmds(self):
        cmds = []
        if len(self.primitives) < 1:
            return [Cmd(Cmd.image_new), Cmd(Cmd.depth_new)]

        operands = [self.primitives[0]]
        if len(self.primitives) == 2:
            operands.append(self.primitives[1])
        elif len(self.primitives) > 2:
            operands.append(Merge(*self.primitives[1:]))

        for operand in operands:
            cmds.extend(operand.build_cmds())

        if len(operands) == 2:
            cmds.append(Cmd(Cmd.sculpt))
        return cmds

@combination(Primitive, Sculpt)
def primitive_at_sculpt(primitive, sculpt):
    sculpt.primitives.append(primitive)
