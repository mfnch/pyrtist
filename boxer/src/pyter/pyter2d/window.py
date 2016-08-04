from base import Taker, combination
from cmd_stream import Cmd, CmdStream
from style import Color, Stroke
from cairo_cmd_exec import CairoCmdExecutor
from bbox import BBox
from point import Point

class Window(Taker):
    '''Window is the object which receives the drawing commands.

    It can either send these commands to a drawer or record them and send them
    later via the Save method.
    '''

    def __init__(self, cmd_executor=None):
        '''Create a new Window.'''

        self.cmd_stream = CmdStream()
        self.cmd_executor = cmd_executor
        super(Window, self).__init__()

    def _consume_cmds(self):
        cmd_exec = self.cmd_executor
        if cmd_exec is not None:
            self.cmd_stream = cmd_exec.execute(self.cmd_stream)

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

class Save(object):
    def __init__(self, file_name, size=None, resolution=None,
                 origin=None, mode=None):
        self.file_name = file_name
        self.size = size
        self.resolution = resolution
        self.origin = origin
        self.mode = mode

@combination(CmdStream, Window)
def fn(cmd_stream, window):
    window.cmd_stream(cmd_stream)
    window._consume_cmds()

@combination(Save, Window, 'Save')
def fn(save, window):
    if window.cmd_executor is not None:
        window.cmd_executor.save(save.file_name)
        return
    origin, size = (save.origin, save.size)
    if size is None:
        bbox = BBox(window)
        origin = bbox.min_point
        size = bbox.max_point - bbox.min_point
    cmd_exec = CairoCmdExecutor(origin=origin, size=size,
                                mode=save.mode, resolution=save.resolution)
    cmd_exec.execute(window.cmd_stream)
    cmd_exec.save(save.file_name)

@combination(Window, Window)
def fn(child, parent):
    if child.cmd_executor is None:
        parent.cmd_stream(child.cmd_stream)
