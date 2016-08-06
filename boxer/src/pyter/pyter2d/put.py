from base import *
from base_types import *
from cmd_stream import *
from window import Window
from transform import Transform
from auto_transform import Constraint as Near, AutoTransform

__all__ = ('Put', 'SimplePut', 'Near')


class SimplePut(Taker):
    def __init__(self, *args):
        self.window = None
        self.matrix = Matrix()
        super(SimplePut, self).__init__(*args)

@combination(Matrix, SimplePut)
def fn(matrix, simple_put):
    simple_put.matrix = matrix

@combination(Window, SimplePut)
def fn(window, simple_put):
    simple_put.window = window

@combination(SimplePut, Window, 'SimplePut')
def fn(simple_put, window):
    flt = CmdArgFilter.from_matrix(simple_put.matrix)
    window.cmd_stream.add(simple_put.window.cmd_stream, flt)


class Put(Taker):
    def __init__(self, *args):
        self.window = None
        self.transform = Transform()
        self.auto_transform = AutoTransform()
        self.constraints = []
        super(Put, self).__init__(*args)

@combination(Window, Put)
def fn(window, put):
    put.window = window

@combination(str, Put)
def fn(transform_str, put):
    put.auto_transform = AutoTransform.from_string(transform_str)

@combination(Transform, Put)
def fn(transform, put):
    put.transform = transform

@combination(Near, Put)
def fn(near, put):
    put.constraints.append(near)

@combination(Put, Window, 'Put')
def fn(put, window):
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
