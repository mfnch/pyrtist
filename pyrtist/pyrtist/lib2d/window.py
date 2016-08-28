__all__ = ('Hot', 'Window')

import numbers

from base import Taker, combination
from cmd_stream import Cmd, CmdStream
from style import Color, Stroke
from cairo_cmd_exec import CairoCmdExecutor
from bbox import BBox
from core_types import Point


class Hot(object):
    def __init__(self, name, point):
        self.name = name
        self.point = point


class Window(Taker):
    '''Window is the object which receives the drawing commands.

    It can either send these commands to a drawer or record them and send them
    later via the Save method.
    '''

    default_executor = CairoCmdExecutor
    default_mode = 'rgb24'
    default_bg_color = Color.white
    default_resolution = 10.0

    def __init__(self, cmd_executor=None):
        '''Create a new Window.'''

        self.cmd_stream = CmdStream()
        self.cmd_executor = cmd_executor
        self.hot_points = {}
        super(Window, self).__init__()

    def __getitem__(self, name):
        return self.hot_points[name]

    def get(self, name, default=None):
        return self.hot_points.get(name, default)

    def _consume_cmds(self):
        cmd_exec = self.cmd_executor
        if cmd_exec is not None:
            self.cmd_stream = cmd_exec.execute(self.cmd_stream)

    def save(self, file_name, mode=None, size=None, resolution=None,
             executor=None, bg_color=None):
        '''Save the window to file, computing the visible area automatically.

        The user should provide either the resolution or the size in pixels.
        The visible region will be automatically determined by computing the
        bounding box of the window. Use BBox() to set explicitly the bounding
        box and override the automatic computation.
        '''
        extra_args = (mode, size, resolution, executor, bg_color)
        if self.cmd_executor is not None:
            if any(arg is not None for arg in extra_args):
                raise ValueError('No arguments should be given when saving '
                                 'a window which has a target')
            self.cmd_executor.save(file_name)
            return

        if size is not None and resolution is not None:
            raise ValueError('Only one of the arguments size, resolution '
                             'should be given to the save method')

        # Compute the bouding box.
        bbox = BBox(window)

        if size is None:
            if resolution is not None:
                resolution = (Point((resolution, resolution))
                              if isinstance(resolution, numbers.Number)
                              else Point(resolution))
                if resolution.x <= 0 or resolution.y <= 0:
                    raise ValueError('Negative resolution')
            else:
                resolution = self.default_resolution

            view = bbox.max_point - bbox.min_point
            size = (int(view.x*resolution.x), int(view.y*resolution.y))
        else:
            size = (size)

        ex = executor or self.default_executor
        mode = mode or self.default_mode
        bg = bg_color or self.default_bg_color
        surface = ex.create_image_surface(mode, size[0], size[1])
        cmd_exec = ex.for_surface(surface, bot_left=bbox.min_point,
                                  top_right=bbox.max_point, bg_color=bg)
        cmd_exec.execute(self.cmd_stream)
        cmd_exec.save(file_name)


@combination(Window, BBox)
def fn(window, bbox):
    for cmd in window.cmd_stream:
        args = cmd.get_args()
        for arg in args:
            if isinstance(arg, Point):
                bbox(arg)

@combination(BBox, Window, 'BBox')
def fn(bbox, window):
    if bbox:
        window(CmdStream(Cmd(Cmd.set_bbox, bbox.min_point, bbox.max_point)))

@combination(Color, Window, 'Color')
def fn(color, window):
    window(CmdStream(color))

@combination(Stroke, Window)
def fn(stroke, window):
    window.take(CmdStream(stroke))

@combination(Hot, Window, 'Hot')
def fn(hot, window):
    window.hot_points[hot.name] = hot.point


@combination(CmdStream, Window)
def fn(cmd_stream, window):
    window.cmd_stream(cmd_stream)
    window._consume_cmds()

@combination(Window, Window)
def fn(child, parent):
    cmd_exec = parent.cmd_executor
    if cmd_exec is not None:
        cmd_exec.execute(child.cmd_stream)
    else:
        parent.cmd_stream.take(child.cmd_stream)
