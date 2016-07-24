from base import Taker, combination
from cmd_stream import CmdStream

class Path(Taker):
    def __init__(self, *args):
        self.cmd_stream = CmdStream()
        super(Path, self).__init__(*args)

@combination(Path, Path)
def fn(child, parent):
    parent.cmd_stream(child.cmd_stream)

@combination(Path, CmdStream)
def fn(path, cmd_stream):
    cmd_stream(path.cmd_stream)
