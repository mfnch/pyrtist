from base import *
from window import Window

__all__ = ('Put', 'Near')

class Near(object):
    def __init__(self, reference_point, target_point, strength=1.0):
        self.reference_point = reference_point
        self.target_point = target_point
        self.strength = strength

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
