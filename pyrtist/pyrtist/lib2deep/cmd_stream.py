from ..lib2d.base import combination
from ..lib2d import cmd_stream as twod
from ..lib2d import Window, Scalar

from .core_types import Point3, Z, DeepMatrix


class Cmd(twod.CmdBase):
    # Drawing commands.
    draw_cmd_names = ('on_step', 'on_plane', 'on_sphere', 'on_cylinder',
                      'on_circular', 'on_crescent')
    # All commands.
    names = draw_cmd_names + \
      ('set_bbox', 'depth_new', 'merge', 'sculpt',
       'image_new', 'image_draw', 'transfer')
Cmd.register_commands()


class DeepCmdArgFilter(twod.CmdArgFilter):
    @classmethod
    def from_matrix(cls, mx):
        if not isinstance(mx, DeepMatrix):
            mx = DeepMatrix(mx)
        mx2 = mx.get_xy()
        # Remember: the determinant tells us how volumes are transformed.
        scale_factor = abs(mx.det3())**(1.0/3.0)
        filter_point = lambda p: mx2.apply(p)
        filter_scalar = lambda s: Scalar(scale_factor*s)
        filters = {Point3: lambda p: mx.apply(p),
                   Z: lambda z: Z(mx.apply_z(z))}
        return cls(filter_scalar, filter_point, filters)

    def __init__(self, filter_scalar, filter_point, filters=None):
        super(DeepCmdArgFilter, self).__init__(filter_scalar, filter_point)
        self.filters = filters or {}

    def __call__(self, arg):
        flt_arg = self.filters.get(arg.__class__)
        if flt_arg is not None:
            return flt_arg(arg)
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
