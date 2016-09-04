__all__ = ('CmdBase', 'Cmd', 'CmdStream', 'CmdArgFilter')

import math

from .base import *
from .core_types import *


class CmdBase(object):
    '''Base class to represent a graphics command.'''

    names = ()

    @classmethod
    def register_commands(cls):
        '''Create the command ID enumeration under the class.'''
        for cmd_nr, name in enumerate(cls.names):
            setattr(cls, name, cmd_nr)

    def __init__(self, *args):
        self.args = tuple(args)

    def __repr__(self):
        if len(self.args) < 1:
            return 'Cmd()'
        args = iter(self.args)
        cmd_nr = args.next()
        arg_strs = ['Cmd.{}'.format(self.names[cmd_nr])
                    if 0 <= cmd_nr < len(self.names) else '???']
        arg_strs.extend(str(arg) for arg in args)
        return 'Cmd({})'.format(', '.join(arg_strs))

    def get_name(self):
        return self.names[self.args[0]]

    def get_args(self):
        return self.args[1:]

    def filter(self, arg_filter):
        '''Return a new command where arguments are filtered through the given
        function.
        '''
        args = iter(self.args)
        new_args = [args.next()]
        for arg in args:
            new_args.append(arg_filter(arg))
        return self.__class__(*new_args)


class CmdStreamBase(Taker):
    def __init__(self, *args):
        self.cmds = []
        super(CmdStreamBase, self).__init__(*args)

    def __iter__(self):
        return iter(self.cmds)

    def __repr__(self):
        return repr(self.cmds)

    def __getitem__(self, index):
        return self.cmds[index]

    def add(self, cmd_stream, arg_filter=None):
        if arg_filter is None:
            self.cmds += cmd_stream.cmds
        else:
            for cmd in cmd_stream:
                self.cmds.append(cmd.filter(arg_filter))


class Cmd(CmdBase):
    names = ('move_to', 'line_to', 'ext_arc_to', 'close_path',
             'set_line_width', 'set_line_join', 'set_line_cap',
             'set_source_rgb', 'set_source_rgba',
             'stroke', 'fill', 'fill_preserve', 'save', 'restore',
             'set_bbox')
Cmd.register_commands()


class CmdArgFilter(object):
    def __init__(self, filter_scalar, filter_point):
        self.filter_scalar = filter_scalar
        self.filter_point = filter_point

    @staticmethod
    def from_matrix(mx):
        # Remember: the determinant tells us how volumes are transformed.
        scale_factor = math.sqrt(abs(mx.det()))
        filter_point = lambda p: mx.apply(p)
        filter_scalar = lambda s: Scalar(scale_factor*s)
        return CmdArgFilter(filter_scalar, filter_point)

    def __call__(self, arg):
        if isinstance(arg, Point):
            return self.filter_point(arg)
        elif isinstance(arg, Scalar):
            return self.filter_scalar(arg)
        return arg


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