__all__ = ('BBox',)

import math

from .base import Taker, combination
from .core_types import Point
from .cmd_stream import CmdStream


def range_of_sin(angle_begin, angle_end):
    yb = math.sin(angle_begin)
    ye = math.sin(angle_end)
    y_min = min(yb, ye)
    y_max = max(yb, ye)

    ab = angle_begin/(2.0*math.pi)
    ae = angle_end/(2.0*math.pi)
    # Minima are at 3/2 pi + 2 k pi, for integer k.
    if math.floor(ab - 0.75) != math.floor(ae - 0.75):
        y_min = -1.0
    # Maxima are at 1/2 pi + 2 k pi, for integer k.
    if math.floor(ab - 0.25) != math.floor(ae - 0.25):
        y_max = 1.0

    # If this interval contains a minimum, then this is our y_min.
    return [y_min, y_max]

def range_of_cos(angle_begin, angle_end):
    return range_of_sin(angle_begin + 0.5*math.pi, angle_end + 0.5*math.pi)


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
        if direction < 0:
            start_ang, end_ang = (end_ang, start_ang)
        if start_ang > end_ang:
            d = 2.0*math.pi
            end_ang += math.ceil((start_ang - end_ang)/d)*d
        x0, x1 = range_of_cos(start_ang, end_ang)
        y0, y1 = range_of_sin(start_ang, end_ang)
        u = one_zero - center
        v = zero_one - center
        w = self.current_width
        if self.current_width is not None:
            u += Point(math.copysign(w, u.x), math.copysign(w, u.x))
            v += Point(math.copysign(w, v.x), math.copysign(w, v.x))
        self.bbox.take(center + x0*u + y0*v,
                       center + x1*u + y1*v)

@combination(CmdStream, BBox)
def cmd_stream_at_bbox(cmd_stream, bbox):
    bbox_exec = BBoxExecutor(bbox)
    bbox_exec.execute(cmd_stream)
