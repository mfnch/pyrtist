'''Merge object: allows merging surfaces.'''

__all__ = ('Merge',)

from ..lib2d.base import combination
from .primitive import Primitive
from .cmd_stream import Cmd


class Merge(Primitive):
    def __init__(self, *args):
        super(Merge, self).__init__()
        self.primitives = []
        self.take(*args)

    def build_depth_cmd(self):
        cmds = []
        for p in self.primitives:
            if len(cmds) > 0:
                cmds.append(Cmd(Cmd.merge))
            cmds.append(Cmd(Cmd.image_new))
            cmds.extend(p.build_image_cmd())
            cmds.append(Cmd(Cmd.depth_new))
            cmds.extend(p.build_depth_cmd())
        cmds.append(Cmd(Cmd.merge))
        return cmds

@combination(Primitive, Merge)
def primitive_at_merge(primitive, merge):
    merge.primitives.append(primitive)
