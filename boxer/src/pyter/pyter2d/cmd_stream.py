from base import Taker, combination

class Cmd(object):
    names = ('move_to', 'line_to', 'ext_arc_to', 'close_path',
             'set_source_rgb', 'set_source_rgba',
             'stroke', 'fill', 'save', 'restore',
             'set_bbox')

    def __init__(self, *args):
        self.args = tuple(args)

    def __repr__(self):
        if len(self.args) < 1:
            return 'Cmd()'
        args = iter(self.args)
        cmd_nr = args.next()
        arg_strs = ['Cmd.{}'.format(Cmd.names[cmd_nr])
                    if 0 <= cmd_nr < len(Cmd.names) else '???']
        for arg in args:
            arg_strs.append(str(arg))
        return 'Cmd({})'.format(', '.join(arg_strs))

    def get_name(self):
        return Cmd.names[self.args[0]]

    def get_args(self):
        return self.args[1:]

for cmd_nr, name in enumerate(Cmd.names):
    setattr(Cmd, name, cmd_nr)

class CmdStream(Taker):
    def __init__(self, *args):
        self.cmds = []
        super(CmdStream, self).__init__(*args)

    def __iter__(self):
        return iter(self.cmds)

    def __repr__(self):
        return repr(self.cmds)


@combination(Cmd, CmdStream)
def fn(cmd, cmd_stream):
    cmd_stream.cmds.append(cmd)

@combination(CmdStream, CmdStream)
def fn(child, parent):
    parent.cmds += child.cmds
