__all__ = ('SimplePut',)

from ..lib2d.base import Taker, combination
from ..lib2d import SimplePut, Put, Near
from .core_types import Point, Point3, DeepMatrix
from .deep_window import DeepWindow
from .cmd_stream import DeepCmdArgFilter


@combination(SimplePut, DeepWindow, 'SimplePut')
def simple_put_at_deep_window(simple_put, deep_window):
    flt = DeepCmdArgFilter.from_matrix(simple_put.matrix or DeepMatrix())
    deep_window.cmd_stream.add(simple_put.get_window().cmd_stream, flt)

@combination(DeepWindow, Put)
def deep_window_at_put(deep_window, put):
    put.window = deep_window

@combination(Put, DeepWindow, 'Put')
def put_at_deep_window(put, deep_window):
    xy_constraints = []
    z_constraints = []
    for c in put.constraints:
        src, dst, weight = (c.src, Point3(c.dst), float(c.weight))
        if not isinstance(src, (Point, Point3)):
            reference_point = put.window.get(src)
            if reference_point is None:
                raise ValueError('Cannot find reference point {}'
                                 .format(repr(src)))
            src = reference_point
        src = Point3(src)
        xy_constraints.append(Near(src.get_xy(), dst.get_xy(), weight))
        z_constraints.append((src.z, dst.z, weight))

    # Calculate the xy part of the matrix.
    t = put.auto_transform.calculate(put.transform, xy_constraints)
    mx = t.get_matrix()
    flt = DeepCmdArgFilter.from_matrix(mx)
    deep_window.cmd_stream.add(put.window.cmd_stream, flt)
