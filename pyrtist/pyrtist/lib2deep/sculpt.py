'''Sculpt object: allows sculpting one object on top of another one.'''

__all__ = ('Sculpt',)

from ..lib2d.base import combination
from .primitive import Primitive
from .cmd_stream import Cmd


class Sculpt(Primitive):
    def __init__(self, *args):
        super(Sculpt, self).__init__()
        self.primitives = []
        self.take(*args)

    def build_depth_cmd(self):
        cmds = []
        if len(self.primitives) < 1:
            return cmds

        operands = [self.primitives[0]]
        if len(self.primitives) == 2:
            operands.append(self.primitives[1])
        elif len(self.primitives) > 2:
            operands.append(Merge(self.primitives[1:]))

        for operand in operands:
            cmds.append(Cmd(Cmd.image_new))
            cmds.extend(operand.build_image_cmd())
            cmds.append(Cmd(Cmd.depth_new))
            cmds.extend(operand.build_depth_cmd())

        if len(operands) == 2:
            cmds.append(Cmd(Cmd.sculpt))
        return cmds

@combination(Primitive, Sculpt)
def primitive_at_sculpt(primitive, sculpt):
    sculpt.primitives.append(primitive)
