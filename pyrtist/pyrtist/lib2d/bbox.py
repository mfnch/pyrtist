__all__ = ('BBox',)

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
def fn(point, bbox):
    point = Point(point)
    if bbox.min_point is None:
        bbox.min_point = bbox.max_point = point
    else:
        bbox.min_point = Point(min(bbox.min_point.x, point.x),
                               min(bbox.min_point.y, point.y))
        bbox.max_point = Point(max(bbox.max_point.x, point.x),
                               max(bbox.max_point.y, point.y))


class BBoxExecutor(object):
    def __init__(self, bbox):
        self.bbox = bbox

    def execute(self, cmds):
        for cmd in cmds:
            method_name = 'cmd_{}'.format(cmd.get_name())
            method = getattr(self, method_name, self.generic_cmd_executor)
            method(*cmd.get_args())

    def generic_cmd_executor(self, *args):
        for arg in args:
            if isinstance(arg, Point):
                self.bbox.take(arg)

    def cmd_set_bbox(self, p1, p2):
        self.bbox.reset()
        self.bbox.take(p1, p2)

@combination(CmdStream, BBox)
def cmd_stream_at_bbox(cmd_stream, bbox):
    bbox_exec = BBoxExecutor(bbox)
    bbox_exec.execute(cmd_stream)
