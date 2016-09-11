__all__ = ('Put', 'SimplePut', 'Near')

from .base import *
from .core_types import *
from .cmd_stream import *
from .window import Window
from .transform import Transform
from .auto_transform import Constraint as Near, AutoTransform


class SimplePut(object):
    def __init__(self, window, matrix=None):
        self.window = window
        self.matrix = matrix

    def get_window(self):
        if self.window is None:
            raise ValueError('No window was given to SimplePut')
        return self.window

@combination(SimplePut, Window, 'SimplePut')
def fn(simple_put, window):
    flt = CmdArgFilter.from_matrix(simple_put.matrix or Matrix())
    window.cmd_stream.add(simple_put.window.cmd_stream, flt)


class Put(Taker):
    def __init__(self, *args):
        self.window = None
        self.transform = Transform()
        self.auto_transform = AutoTransform()
        self.constraints = []
        super(Put, self).__init__(*args)

@combination(Window, Put)
def window_at_put(window, put):
    put.window = window

@combination(str, Put)
def str_at_put(transform_str, put):
    put.auto_transform = AutoTransform.from_string(transform_str)

@combination(Transform, Put)
def transform_at_put(transform, put):
    put.transform = transform

@combination(Near, Put)
def near_at_put(near, put):
    put.constraints.append(near)

@combination(Put, Window, 'Put')
def put_at_window(put, window):
    resolved_constraints = []
    for c in put.constraints:
        src, dst, weight = (c.src, Point(c.dst), float(c.weight))
        if not isinstance(src, Point):
            reference_point = put.window.get(src)
            if reference_point is None:
                raise ValueError('Cannot find reference point {}'
                                 .format(repr(src)))
            src = reference_point
        resolved_constraints.append(Near(Point(src), dst, weight))

    t = put.auto_transform.calculate(put.transform, resolved_constraints)
    mx = t.get_matrix()
    flt = CmdArgFilter.from_matrix(mx)
    window.cmd_stream.add(put.window.cmd_stream, flt)
