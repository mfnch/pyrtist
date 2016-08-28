__all__ = ('DeepWindow',)

from ..lib2d import BBox
from ..lib2d.base import Taker, combination
from .cmd_stream import Cmd, CmdStream


class DeepWindow(Taker):
    def __init__(self, cmd_executor=None):
        self.cmd_stream = CmdStream()
        self.cmd_executor = cmd_executor
        super(DeepWindow, self).__init__()

    def _consume_cmds(self):
        cmd_exec = self.cmd_executor
        if cmd_exec is not None:
            self.cmd_stream = cmd_exec.execute(self.cmd_stream)


@combination(CmdStream, DeepWindow)
def cmd_stream_at_deep_window(cmd_stream, deep_window):
    deep_window.cmd_stream(cmd_stream)
    deep_window._consume_cmds()

@combination(BBox, DeepWindow, 'BBox')
def bbox_at_deep_window(bbox, deep_window):
    pass
