__all__ = ('SimplePut',)

from ..lib2d.base import Taker, combination
from ..lib2d import SimplePut
from .core_types import DeepMatrix
from .deep_window import DeepWindow
from .cmd_stream import DeepCmdArgFilter


@combination(SimplePut, DeepWindow, 'SimplePut')
def fn(simple_put, deep_window):
    flt = DeepCmdArgFilter.from_matrix(simple_put.matrix or DeepMatrix())
    deep_window.cmd_stream.add(simple_put.get_window().cmd_stream, flt)
