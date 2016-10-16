__all__ = ('BBox',)

import math

from .base import Taker, combination
from .core_types import Point
from .cmd_stream import CmdStream

class BBox(Taker):
    def __init__(self, *args):
        self.min_point = None
        self.max_point = None
        super(BBox, self).__init__(*args)

    def __str__(self):
        return 'BBox({}, {})'.format(self.min_point, self.max_point)

    def __nonzero__(self):
        return (self.min_point is not None)

    def __iter__(self):
        return iter(filter(None, (self.min_point, self.max_point)))

    def reset(self):
        '''Reset the bounding box so that it is the same as BBox().'''
        self.min_point = None
        self.max_point = None

@combination(tuple, BBox)
@combination(Point, BBox)
def point_at_bbox(point, bbox):
    point = Point(point)
    if bbox.min_point is None:
        bbox.min_point = bbox.max_point = point
    else:
        bbox.min_point = Point(min(bbox.min_point.x, point.x),
                               min(bbox.min_point.y, point.y))
        bbox.max_point = Point(max(bbox.max_point.x, point.x),
                               max(bbox.max_point.y, point.y))

@combination(BBox, BBox)
def bbox_at_bbox(child, parent):
    parent.take(child.min_point, child.max_point)


class BBoxExecutor(object):
    def __init__(self, bbox):
        self.bbox = bbox
        self.current_width = None

    def execute(self, cmds):
        for cmd in cmds:
            method_name = 'cmd_{}'.format(cmd.get_name())
            method = getattr(self, method_name, self.generic_cmd_executor)
            method(*cmd.get_args())

    def generic_cmd_executor(self, *args):
        for arg in args:
            if isinstance(arg, Point):
                w = self.current_width
                if w is None:
                    self.bbox.take(arg)
                else:
                    self.bbox.take(arg + Point(w, w))
                    self.bbox.take(arg - Point(w, w))

    def cmd_set_line_width(self, width):
        self.current_width = abs(width)

    def cmd_set_bbox(self, p1, p2):
        self.bbox.reset()
        self.bbox.take(p1, p2)

    def cmd_ext_arc_to(self, center, one_zero, zero_one,
                       start_ang, end_ang, direction):
        u = one_zero - center
        v = zero_one - center
        w = self.current_width
        if self.current_width is not None:
            u += Point(math.copysign(w, u.x), math.copysign(w, u.x))
            v += Point(math.copysign(w, v.x), math.copysign(w, v.x))
        self.bbox.take(center + u + v,
                       center + u - v,
                       center - u - v,
                       center - u + v)

@combination(CmdStream, BBox)
def cmd_stream_at_bbox(cmd_stream, bbox):
    bbox_exec = BBoxExecutor(bbox)
    bbox_exec.execute(cmd_stream)
