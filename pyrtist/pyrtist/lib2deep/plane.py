'''Plane object: allows drawing on a plane.'''

__all__ = ('Plane',)

from ..lib2d import Window
from ..lib2d.base import Taker, combination
from .deep_window import DeepWindow

class Plane(Taker):
    def __init__(self, *args):
        self.point = None
        self.normal = None
        super(Plane, self).__init__(*args)

@combination(Window, Plane)
def windot_at_plane(window, plane):
    plane.window = window

@combination(Plane, DeepWindow)
def plane_at_deep_window(plane, deep_window):
    pass
