from ..lib2d.base import combination
from ..lib2d import cmd_stream as twod
from ..lib2d import Window, Scalar

from .core_types import Z, DeepMatrix


class Cmd(twod.CmdBase):
    names = ('set_bbox', 'transfer', 'src_draw', 'on_step', 'on_plane',
             'on_sphere')
Cmd.register_commands()


class DeepCmdArgFilter(twod.CmdArgFilter):
    @classmethod
    def from_matrix(cls, mx):
        if not isinstance(mx, DeepMatrix):
            mx = DeepMatrix(mx)
        mx2 = mx.get_xy()
        # Remember: the determinant tells us how volumes are transformed.
        scale_factor = abs(mx.det3())**(1.0/3.0)
        filter_z = lambda z: Z(mx.apply_z(z))
        filter_point = lambda p: mx2.apply(p)
        filter_scalar = lambda s: Scalar(scale_factor*s)
        return cls(filter_scalar, filter_point, filter_z)

    def __init__(self, filter_scalar, filter_point, filter_z=None):
        super(DeepCmdArgFilter, self).__init__(filter_scalar, filter_point)
        self.filter_z = filter_z or (lambda z: z)

    def __call__(self, arg):
        if isinstance(arg, Z):
            return self.filter_z(arg)
        if isinstance(arg, Window):
            w = Window()
            w.cmd_stream.add(arg.cmd_stream, self)
            return w
        return super(DeepCmdArgFilter, self).__call__(arg)


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
