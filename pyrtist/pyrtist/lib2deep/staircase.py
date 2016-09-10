'''Staircase object: allow drawing on a staircase (piecewise planar surface).
'''

__all__ = ('Staircase',)

from ..lib2d import Window, Point, Axes, BBox
from ..lib2d.base import Taker, combination, RejectException
from .core_types import Z
from .profile import Profile
from .cmd_stream import Cmd, CmdStream
from .deep_window import DeepWindow

class Staircase(Taker):
    def __init__(self, *args):
        self.axes = None
        self.profile = None
        self.window = None
        super(Staircase, self).__init__(*args)

@combination(Profile, Staircase)
def profile_at_staircase(profile, staircase):
    if staircase.window is not None:
        raise RejectException('Staircase already got a Profile')
    staircase.profile = profile

@combination(Window, Staircase)
def window_at_staircase(window, staircase):
    if staircase.window is not None:
        raise RejectException('Staircase already got a Window')
    staircase.window = window

@combination(Axes, Staircase)
def window_at_staircase(axes, staircase):
    if staircase.axes is not None:
        raise RejectException('Staircase already got an Axes')
    staircase.axes = axes

@combination(Staircase, CmdStream)
def staircase_at_cmd_stream(staircase, cmd_stream):
    if not staircase.profile.is_valid():
        raise ValueError('Invalid Profile object')

    # Draw the window to the source, which also defines the bounding box.
    cmd_stream.take(Cmd(Cmd.src_draw, staircase.window))

    # Create a list of points in coordinate system of the profile object.
    profile = staircase.profile
    mx_transf = profile.axes.get_matrix().get_inverse()
    ps_transf = [mx_transf.apply(p) for p in profile.points]
    ps_transf.sort(key=lambda p: p.x)

    # Now, for every point in the profile object, find its projection into the
    # x-axis of the staircase reference system. Also find the z offset for the
    # same point.
    mx_real = (staircase.axes or profile.axes).get_matrix()
    proj_and_z_offs = [(mx_real.apply(Point(p_transf.x, 0)),
                        p_transf.y)
                       for p_transf in ps_transf]
    for i in range(1, len(proj_and_z_offs)):
        start, z_start = proj_and_z_offs[i - 1]
        end, z_end = proj_and_z_offs[i]
        cmd_stream.take(Cmd(Cmd.on_step, start, Z(z_start), end, Z(z_end)))

    # Transfer to the dst buffer.
    cmd_stream.take(Cmd(Cmd.transfer))

@combination(Staircase, DeepWindow)
def staircase_at_deep_window(staircase, deep_window):
    deep_window.take(CmdStream(staircase))
