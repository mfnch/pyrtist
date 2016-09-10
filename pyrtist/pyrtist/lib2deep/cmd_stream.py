from ..lib2d.base import combination
from ..lib2d import cmd_stream as twod

from .core_types import Z


class Cmd(twod.CmdBase):
    names = ('set_bbox', 'transfer', 'src_draw', 'on_step', 'on_plane',
             'on_sphere')
Cmd.register_commands()


class CmdArgFilter(twod.CmdArgFilter):
    def __init__(self, filter_scalar, filter_point, filter_z=None):
        super(CmdArgFilter, self).__init__(filter_scalar, filter_point)
        self.filter_z = filter_z

    def __call__(self, arg):
        if isinstance(arg, Z):
            return (self.filter_z(arg) if self.filter_z is not None
                    else arg)
        return super(CmdArgFilter, self).__call__(arg)


class CmdStream(twod.CmdStreamBase):
    pass


@combination(Cmd, CmdStream)
def fn(cmd, cmd_stream):
    cmd_stream.cmds.append(cmd)

@combination(type(None), CmdStream)
def fn(none, cmd_stream):
    # Just ignore None objects.
    pass

@combination(CmdStream, CmdStream)
def fn(child, parent):
    parent.add(child)
