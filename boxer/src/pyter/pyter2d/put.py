from base import *
from base_types import *
from cmd_stream import *
from window import Window

__all__ = ('Put', 'SimplePut', 'Near')

class Near(object):
    def __init__(self, reference_point, target_point, strength=1.0):
        self.reference_point = reference_point
        self.target_point = target_point
        self.strength = strength


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
        self.auto_transforms = ''
        self.constraints = []
        super(Put, self).__init__(*args)

@combination(Window, Put)
def fn(window, put):
    put.window = window

@combination(str, Put)
def fn(transform_str, put):
    put.auto_transforms = transform_str

@combination(Near, Put)
def fn(near, put):
    put.constraints.append(near)

@combination(Put, Window, 'Put')
def fn(put, window):
    print('Not implemented yet')
