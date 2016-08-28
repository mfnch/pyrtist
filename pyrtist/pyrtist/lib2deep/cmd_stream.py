from ..lib2d.base import combination
from ..lib2d.cmd_stream import CmdBase, CmdStreamBase


class Cmd(CmdBase):
    names = ('set_bbox', 'draw_sphere')
Cmd.register_commands()


class CmdStream(CmdStreamBase):
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
